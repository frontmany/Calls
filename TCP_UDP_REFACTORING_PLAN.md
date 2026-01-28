# План рефакторинга: TCP для контроля, UDP для медиа

## 1. Текущая архитектура (кратко)

### 1.1. Сервер
- **Один UDP‑сокет** на порту (например 8081). Весь трафик — управляющие пакеты и медиа (голос, камера, экран).
- **Идентификация пользователей**: `asio::ip::udp::endpoint` (IP:port). При AUTHORIZATION создаётся `User` с этим endpoint.
- **PingController**: пинг всех известных endpoint’ов раз в 500 ms, проверка раз в 1 s. После 4 подряд пропущенных pong → «connection down», иначе при успешном pong после down → «connection restored».
- **TaskManager**: повторная отправка управляющих пакетов (интервал 1500 ms, 3–5 попыток). Успех — по приходу CONFIRMATION(uid) от клиента. Используется для:
  - AUTHORIZATION_RESULT, RECONNECT_RESULT;
  - CONNECTION_DOWN_WITH_USER, CONNECTION_RESTORED_WITH_USER;
  - USER_LOGOUT (партнёрам);
  - CALL_DECLINE и т.п.
- **Контроль vs медиа**:
  - Контроль: JSON‑пакеты (AUTHORIZATION, LOGOUT, RECONNECT, CALL_*, CONFIRMATION, …), доставка через Task + CONFIRMATION.
  - Медиа: VOICE, CAMERA, SCREEN — бинарные, ретрансляция через `MediaRelayService` по endpoint партнёра.

### 1.2. Клиент (Core)
- **Один UDP‑сокет**: `connect` к серверу (host:port), `bind(0)` для своего порта. Вся связь с сервером по UDP.
- **PingController**: клиент шлёт ping, ждёт pong; при серии неудач → «connection down». Дополнительно участвует в сценарии CONNECTION_DOWN_WITH_USER / RECONNECT.
- **TaskManager**: та же схема — отправка пакета с UID, повторы до прихода ответа (или CONFIRMATION), затем `completeTask` / `failTask`.
- **PacketProcessor**: разбор входящих пакетов, `onConfirmation` → `completeTask(uid)`, вызовы сервисов (Authorization, Call, MediaSharing и т.д.).

### 1.3. Референс: serverUpdater / updater (TCP)
- **serverUpdater**: `asio::ip::tcp::acceptor`, на каждое подключение — `Connection` (handshake, `PacketsReceiver` / `PacketsSender`, `SafeQueue`). Keepalive, `onDisconnected`.
- **updater (клиент)**: `ConnectionResolver` (connect + handshake), `PacketReceiver` / `PacketSender`, `NetworkController`. Ошибки → `onNetworkError`.
- **Формат пакетов**: header (type, size) + body. Без фрагментации.

---

## 2. Целевая архитектура

### 2.1. Разделение по протоколам
- **TCP**: только **управляющие** пакеты (логин, звонки, экран/камера on/off, отключения и т.д.).
- **UDP**: только **медиа** — VOICE, CAMERA, SCREEN.

### 2.2. Что это даёт
- Гарантии доставки и порядок для контроля — за счёт TCP, без Task‑ретраев и CONFIRMATION‑цикла.
- Обрыв TCP = сразу «connection down», без отдельного pinger’а для контроля.
- Упрощение: убрать PingController и TaskManager для контроля, меньше кода и состояний.
- Медиа остаётся на UDP: низкая задержка, допустимые потери.

---

## 3. Типы пакетов по каналам

### 3.1. Только TCP (контроль)
- **Приём**: AUTHORIZATION, LOGOUT, RECONNECT, GET_USER_INFO, CONFIRMATION,  
  CALLING_BEGIN, CALLING_END, CALL_ACCEPT, CALL_DECLINE, CALL_END,  
  SCREEN_SHARING_BEGIN, SCREEN_SHARING_END, CAMERA_SHARING_BEGIN, CAMERA_SHARING_END.
- **Отправка**: AUTHORIZATION_RESULT, RECONNECT_RESULT, GET_USER_INFO_RESULT,  
  CONNECTION_DOWN_WITH_USER, CONNECTION_RESTORED_WITH_USER, USER_LOGOUT,  
  а также редирект CONFIRMATION, CALLING_*, CALL_ACCEPT/DECLINE/END, SCREEN_*_BEGIN/END, CAMERA_*_BEGIN/END.

### 3.2. Только UDP (медиа)
- **Приём/ретрансляция**: VOICE, CAMERA, SCREEN.

---

## 4. Идентификация пользователей и привязка каналов

### 4.1. Сервер
- **User** хранит:
  - `nicknameHash`, `token`, `publicKey`, прочее как сейчас.
  - **TCP**: ссылка на «управляющее» соединение (`Connection` или `ConnectionPtr`). Все контрольные ответы и редиректы — через него.
  - **UDP**: `asio::ip::udp::endpoint` для медиа. Нужен для `MediaRelayService` (приём медиа от пользователя и отправка партнёру).
- При **AUTHORIZATION** / **RECONNECT** по TCP:
  - Создаём/обновляем `User`, привязываем текущее TCP‑соединение.
  - **Вариант 1 (принят)**: клиент передаёт в JSON только **`udpPort`**. Сервер строит UDP endpoint как `(remote IP TCP‑соединения, udpPort)` и сохраняет в `User`. Один и тот же IP для TCP и UDP предполагается (типично за NAT).

### 4.2. Клиент (Core)
- **TCP**: одно постоянное соединение к серверу (host, tcpPort). Только контроль.
- **UDP**: отдельный сокет для медиа. `bind(0)`. Поднимается вместе с TCP (или сразу после успешного connect). Адрес/порт сервера: host + `udpPort` (см. ниже).
- При авторизации/reconnect клиент передаёт в JSON **`udpPort`** — порт своего UDP‑сокета. Сервер использует пару (IP TCP‑соединения, `udpPort`) как UDP endpoint для медиа.

### 4.3. Порты
- Вариант **А**: один порт для обоих — TCP listen и UDP bind на одном и том же (например 8081). Клиент подключается по TCP и шлёт медиа по UDP на тот же порт.
- Вариант **Б**: два порта (например 8081 TCP, 8082 UDP). Конфиг: `tcpPort`, `udpPort`.
- Рекомендация: **вариант Б** для ясности и независимой настройки файрволов; при необходимости потом можно свести к одному порту.

---

## 5. Референс из serverUpdater / updater и доработки

### 5.1. Что взять за основу
- **Сервер**: структура `Connection` + `NetworkController` (acceptor, очередь `OwnedPacket`, обработка по типу пакета). Handshake, `PacketsReceiver` / `PacketsSender`, `SafeQueue` для исходящих.
- **Клиент**: `ConnectionResolver` (connect + handshake), `PacketReceiver` / `PacketSender`, работа через `NetworkController` (connect/disconnect, очередь пакетов).
- **Формат**: header (type, size) + body, без chunk’ов, как в serverUpdater/updater.

### 5.2. Улучшение обработки ошибок (относительно текущего updater/serverUpdater)
- **Единообразная обработка `error_code`**:
  - `operation_aborted`: при shutdown не считать ошибкой, не вызывать `onDisconnect`/`onError` лишний раз.
  - `connection_reset`, `broken_pipe`, `eof`: явно считать обрывом, логировать и вызывать `onDisconnect` / `onConnectionDown`.
- **Логирование**: все сетевые ошибки с `ec.message()` и по возможности `ec.value()`, уровень WARN/ERROR в зависимости от типа.
- **Сервер**: при любой ошибке read/write в `PacketsReceiver` / `PacketsSender` — закрыть соединение, удалить из множества клиентов, вызвать `onDisconnected(connection)`. Не оставлять «висячих» соединений.
- **Клиент**: при ошибках в resolver, handshake, receive, send — вызывать `onNetworkError` / `onConnectionDown`, закрывать сокет, переводить в состояние «не подключён».
- **Таймауты**: при необходимости — таймаут на connect и на handshake (через `async_wait` + `cancel`), чтобы не висеть при «молчащем» сервере.
- **Graceful shutdown**: при `stop()` — `cancel` операций, `shutdown` + `close` сокетов, дождаться завершения обработчиков перед выходом.

---

## 6. План работ по шагам

### Этап 1: Инфраструктура TCP на сервере

1. **Добавить TCP‑слой** (рядом с существующим UDP):
   - Сделать абстракцию «управляющее соединение» (аналог `Connection` в serverUpdater): handshake, приём/отправка только контрольных пакетов.
   - Использовать формат пакетов header+body (type, size) как в serverUpdater. Отдельные классы `TcpPacketsReceiver` / `TcpPacketsSender` или переиспользовать обобщённые имена в namespace `server::network`.
   - `TcpNetworkController` (или расширить текущий): `tcp::acceptor` на `tcpPort`, при accept — создание `Connection`, `SafeQueue<OwnedPacket>`, цикл обработки очереди и вызов существующих обработчиков (но уже по TCP).

2. **Интеграция с User**:
   - В `User` добавить хранение `ConnectionPtr` (или ссылки на соединение) и `udp::endpoint` для медиа.
   - При AUTHORIZATION / RECONNECT по TCP — обновлять оба. При `onDisconnected` — считать пользователя отключённым (аналог «connection down»).

3. **Отключить PingController для контроля** (шаг можно сделать после переноса всех контрольных пакетов на TCP):
   - Оставить PingController только если решите сохранить опциональный UDP‑keepalive для медиа (ниже не предполагаем). Иначе — удалить PingController и связанные коллбэки.

### Этап 2: Разделение приёма пакетов на сервере

4. **Маршрутизация входящих**:
   - **TCP**: все типы из §3.1. Парсинг JSON там, где нужно. Вызов `handleAuthorization`, `handleLogout`, `handleReconnect`, `handleConfirmation`, `handleStartOutgoingCall`, … как сейчас, но с контекстом TCP‑соединения (и при необходимости endpoint из `User`) вместо UDP endpoint.
   - **UDP**: только VOICE, CAMERA, SCREEN. Как сейчас — `handleVoice` / `handleScreen` / `handleCamera` → `MediaRelayService::relayMedia`. Источник — по UDP endpoint; пользователь по‑прежнему определяется через `findUserByEndpoint` (UDP endpoint в `User`).

5. **Исходящие контрольные пакеты**:
   - Везде, где сейчас `m_packetSender.send(..., endpoint)` или `m_taskManager.createTask(..., sendPacketTask, ...)` для контрольных типов:
     - Отправлять по **TCP** через привязанное к `User` соединение.
     - Убрать создание Task для этих отправок. Отправка один раз; при ошибке write — закрыть соединение, `onDisconnected` → обработка «connection down».
   - Оставить `TaskManager` только если что‑то ещё будет использовать (например, отдельные фоновые задачи); для контроля — убрать.

6. **Удалить CONFIRMATION‑ретраи**:
   - Сервер больше не ретраит отправку контрольных пакетов до CONFIRMATION. Достаточно успешного write в TCP.
   - Решить, нужен ли CONFIRMATION в приложении: если нет — убрать. Если да (например, для явного ack) — оставить только приём CONFIRMATION и, при необходимости, pending actions, но **без** ретраев.

### Этап 3: UDP только для медиа на сервере

7. **Очистить UDP‑код**:
   - Убрать приём по UDP любых контрольных типов (в т.ч. CONFIRMATION, AUTHORIZATION, …).
   - Убрать отправку по UDP любых контрольных типов. Ping/pong убрать вместе с PingController.
   - Оставить в UDP только приём и ретрансляцию VOICE, CAMERA, SCREEN через `MediaRelayService`.

8. **Связка User ↔ UDP**:
   - UDP endpoint пользователя задаётся только из AUTHORIZATION / RECONNECT (в JSON). Важно: до авторизации по TCP медиа от этого пользователя не принимаются (или не ассоциируются с `User`).

### Этап 4: Клиент (Core)

9. **Два канала**:
   - **TCP**: аналог updater — `ConnectionResolver`, `PacketReceiver`, `PacketSender`, единый `NetworkController` (или `TcpControlChannel`). Connect при `start(host, port)`, один сокет на всю сессию.
   - **UDP**: отдельный сокет для медиа. Инициализация при `start` (host, udpPort). Отправка VOICE/CAMERA/SCREEN только через UDP.

10. **Конфиг и порты**:
    - При двух портах: `host`, `tcpPort`, `udpPort`. UI/конфиг передаёт оба.

11. **Авторизация / Reconnect**:
    - В AUTHORIZATION и RECONNECT по TCP добавлять в JSON свой UDP endpoint (адрес и порт UDP‑сокета). Сервер сохраняет в `User`.

12. **Упрощение Core**:
    - Убрать PingController (контроль жизни — по TCP).
    - Убрать TaskManager для контрольных операций: отправка запроса один раз по TCP; при ответе — обработка, при обрыве TCP — `onConnectionDown`.
    - Сохранить при необходимости простой «ожидающий ответа» слой (по `uid` или типу запроса) без ретраев — только матчинг request/response.

13. **Сервисы (Authorization, Call, MediaSharing, …)**:
    - Перевести на отправку контрольных пакетов через TCP (через общий `NetworkController`/`TcpControlChannel`).
    - Медиа (sendScreen, sendCamera, голос) — только через UDP.
    - Убрать `createAndStartTask`-подобные ретраи для authorize, logout, reconnect, call-операций и т.д.

14. **PacketProcessor**:
    - Вход от TCP: все контрольные типы, включая CONFIRMATION (если остаётся), AUTHORIZATION_RESULT, RECONNECT_RESULT, USER_LOGOUT, ….
    - Вход от UDP: только VOICE, CAMERA, SCREEN. Остальное игнорировать.
    - Убрать `completeTask`/`hasTask` для старых сценариев; при необходимости оставить только для локального матчинга request/response без ретраев.

### Этап 5: Финальная очистка и обработка ошибок

15. **Удалить неиспользуемое**:
    - PingController (и server, и client).
    - TaskManager для контроля (и server, и client). Task/taskManager — удалить полностью, если больше нигде не используются.
    - Контрольную логику, завязанную на UDP (connection down/restored по ping).

16. **Обработка ошибок** (в духе §5.2):
    - Везде явно обрабатывать `operation_aborted`, `connection_reset`, `eof`, `broken_pipe`.
    - Единый подход к логированию и к вызову `onDisconnect` / `onConnectionDown` / `onNetworkError`.
    - Проверить graceful shutdown и отмену операций при `stop()`.

17. **Тесты и ручные проверки**:
    - Авторизация, logout, reconnect по TCP.
    - Звонки (calling begin/end, accept/decline, end) только по TCP.
    - Включение/выключение экрана и камеры по TCP.
    - Медиа (голос, экран, камера) только по UDP, ретрансляция через сервер.
    - Обрыв TCP → «connection down» без задержек, восстановление после переподключения.

---

## 7. Риски и смягчение

| Риск | Смягчение |
|------|-----------|
| Разный порядок доставки TCP vs UDP | Медиа не влияет на порядок контрольных сообщений. Важно не начинать медиа-поток до успешного контрольного «start» по TCP. |
| NAT/firewall по-разному пропускают TCP и UDP | Два порта (TCP/UDP) упрощают настройку. Документировать требования к фаерволу. |
| Сложность отладки двух каналов | Чёткое разделение в логах: «[TCP] …» / «[UDP] …», отдельные счётчики пакетов при необходимости. |

---

## 8. Порядок внедрения (рекомендуемый)

1. Реализовать TCP‑сервер (Connection, handshake, receive/send) и **не** переводить пока обработчики — только принять подключение и эхо‑тест пакетов.
2. Добавить UDP endpoint в AUTHORIZATION/RECONNECT и в `User`; оставить весь контроль пока на UDP.
3. Перенести обработку **одного** контрольного сценария на TCP (например, только AUTHORIZATION / AUTHORIZATION_RESULT), убедиться, что клиент может работать через TCP для него.
4. Постепенно переносить остальные контрольные сценарии на TCP, параллельно убирать для них TaskManager и PingController.
5. Оставить в UDP только медиа, удалить контрольный код из UDP.
6. Рефакторинг Core: два канала, отключение ping/task для контроля, обновление сервисов и PacketProcessor.
7. Финальная очистка, улучшение обработки ошибок, тесты.

---

## 9. Итог

- **Контроль** — только TCP: надёжная доставка, обрыв = немедленный «connection down», без pinger’а и ретраев по Task.
- **Медиа** — только UDP: голос, камера, экран через существующий `MediaRelayService`.
- За основу взять TCP‑реализацию serverUpdater/updater, унифицировать и усилить обработку ошибок.
- Рефакторинг делать по этапам: сначала TCP‑инфраструктура и привязка к `User`, затем перенос контроля, затем очистка UDP и клиента.

После выполнения плана код управления соединениями и контролем упростится, а мониторинг отключений будет опираться на TCP вместо отдельного pinger’а.

---

## 10. Выполнено (фаза 2 — Core)

- **Crypto**: добавлен `scramble` (как на сервере) для TCP handshake.
- **jsonType**: добавлен `UDP_PORT`.
- **PacketFactory**: `getAuthorizationPacket` и `getReconnectPacket` принимают `udpPort`, добавляют его в JSON.
- **TCP-слой в core**: `tcp_packet.h`, `TcpPacketsReceiver`, `TcpPacketsSender`, `TcpControlClient` (connect, handshake, приём/отправка). Формат пакетов совпадает с сервером (header type+bodySize + body).
- **NetworkController**: только UDP для медиа. Удалены PingController, пинг/понг. `init(host, udpPort, onReceive, onConnectionDown, onConnectionRestored)`. Добавлен `getLocalUdpPort()`.
- **Core**: `start(host, tcpPort, udpPort)`. Запуск UDP и TCP, подключение по TCP. Connection down только при обрыве TCP; `failAll` pending при down.
- **PendingRequests**: замена TaskManager для контроля. `add`/`complete`/`fail`/`failAll`. Без ретраев.
- **Сервисы**: Authorization, Call, MediaSharing переведены на `PendingRequests` + `TcpControlClient::send`. Контроль только по TCP; медиа (voice, screen, camera) — по UDP.
- **PacketProcessor**: приём контроля по TCP, медиа по UDP. Удалены CONFIRMATION и `sendConfirmation`. Обработчики вызывают `m_pendingRequests.complete(uid, json)` там, где раньше был `completeTask`.
- **ConfigManager / UI / Python**: два порта — `getTcpPort()` / `getUdpPort()`, конфиг `tcpPort` / `udpPort`, дефолты 8081 / 8082. `Client::start(host, tcpPort, udpPort)`. Python `init(host, tcp_port, udp_port, event_listener)`.
- **Reconnect**: при connection down выполняется `disconnect` + `connect` по TCP, затем отправка RECONNECT с `udpPort`. Ожидание ответа через pending.

---

## 11. Финальная очистка (после фазы 2)

- **Удалены неиспользуемые файлы**:
  - **Client (core)**: `pingController.cpp` / `pingController.h`, `taskManager.h`, `task.h` — контроль по TCP, PendingRequests вместо TaskManager.
  - **Server**: `pingController.cpp` / `pingController.h`, `taskManager.h`, `task.h`, `confirmationKey.h` — контроль по TCP, без пингера и тасков.
- **Server**: убрана обработка `CONFIRMATION` в `onTcpControlPacket` (клиент больше не шлёт). Удалён мёртвый перегруз `NetworkController::send(uint32_t type, endpoint)` (пустые пакеты не используются).
- **Конфиг**: `getPort` / `m_port` оставлены — используются для порта **updater** в UI, не для core.
</think>
