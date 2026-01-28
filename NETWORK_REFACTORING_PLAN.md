# План рефакторинга сетевого слоя (TCP/UDP)

## Цели

- Отдельные namespace `tcp` и `udp`; в коде не использовать префиксы tcp/udp — различие только через namespace.
- Вложенные namespace: `core::network::tcp`, `core::network::udp` (клиент); `server::network::tcp`, `server::network::udp` (сервер).
- Папки `network/tcp/` и `network/udp/`; убрать префиксы `tcp_` из имён файлов.
- Унифицировать названия фасадных классов: оба канала — **Controller** (например, `ControlController` для TCP, `MediaController` для UDP).
- Избегать сокращений в именах классов и переменных; допустимы только при явном обосновании.
- **Именование: не использовать snake_case.** В коде — только camelCase (классы, функции, переменные, файлы). Исключение: **приватные члены** — `m_camelCase` (например, `m_controlController`, `m_mediaController`).
- Упорядочить структуру и по возможности улучшить логику.

---

## 1. Namespace и структура папок

### 1.1 Клиент (core)

**Было:** `core::network` — всё в одном namespace, файлы в `network/` без разделения.

**Станет:**
- `core::network::tcp` — всё, что относится к TCP (управление, контрольные пакеты).
- `core::network::udp` — всё, что относится к UDP (медиа: голос, камера, экран).

**Папки (имена файлов — camelCase, не snake_case):**
```
client/core/src/network/
├── tcp/
│   ├── controlController.h   (было tcp_control_client)
│   ├── controlController.cpp
│   ├── packet.h              (было tcp_packet, только для TCP)
│   ├── packetsReceiver.h     (было tcp_packets_receiver)
│   ├── packetsReceiver.cpp
│   ├── packetsSender.h       (было tcp_packets_sender)
│   └── packetsSender.cpp
├── udp/
│   ├── mediaController.h     (было networkController)
│   ├── mediaController.cpp
│   ├── packet.h              (общий UDP‑пакет; был packet.h в network/)
│   ├── packetReceiver.h      (было packetReceiver)
│   ├── packetReceiver.cpp
│   ├── packetSender.h        (было packetSender)
│   └── packetSender.cpp
└── (при необходимости common/ или общие вещи в network/)
```

- Файлы только в `tcp/` и `udp/`, без префиксов `tcp_`/`udp_` в именах. Имена файлов — **camelCase**.
- Внутри `tcp/` — `namespace core::network::tcp`; внутри `udp/` — `namespace core::network::udp`.

### 1.2 Сервер

**Аналогично:**
- `server::network::tcp` — TCP (acceptor, соединения, контроль).
- `server::network::udp` — UDP (медиа).

**Папки (camelCase для файлов):**
```
server/src/network/
├── tcp/
│   ├── controlController.h   (было tcp_control_controller)
│   ├── controlController.cpp
│   ├── connection.h          (было tcp_connection)
│   ├── connection.cpp
│   ├── packet.h              (было tcp_packet)
│   ├── packetsReceiver.h
│   ├── packetsReceiver.cpp
│   ├── packetsSender.h
│   └── packetsSender.cpp
└── udp/
    ├── mediaController.h     (было networkController)
    ├── mediaController.cpp
    ├── packetReceiver.h
    ├── packetReceiver.cpp
    ├── packetSender.h
    └── packetSender.cpp
```

---

## 2. Унификация фасадных классов (Controller)

| Роль | Было | Станет | Namespace |
|------|------|--------|-----------|
| Клиент TCP | `TcpControlClient` | `ControlController` | `core::network::tcp` |
| Клиент UDP | `NetworkController` | `MediaController` | `core::network::udp` |
| Сервер TCP | `TcpControlController` | `ControlController` | `server::network::tcp` |
| Сервер UDP | `NetworkController` | `MediaController` | `server::network::udp` |

Клиентский TCP по сути «подключается» (client), серверный — «принимает» (acceptor), но оба фасада называем `ControlController` для единообразия.

- Оба канала — **Controller**; различие по смыслу: **Control** (управление) vs **Media** (медиа).
- В коде не использовать суффиксы/префиксы `Tcp`/`Udp` — только `core::network::tcp::ControlController`, `core::network::udp::MediaController` и т.п.

**Переименование членов в Core и сервере (приватные — m_camelCase):**
- `m_tcpControl` → `m_controlController` (тип из `core::network::tcp`).
- `m_networkController` → `m_mediaController` (тип из `core::network::udp`).
- Параметры/локальные переменные: `controlController`, `mediaController` и т.д., без `tcp`/`udp` в имени.

---

## 3. Внутренние классы TCP (клиент)

| Было | Станет | Файл |
|------|--------|------|
| `TcpControlClient` | `ControlController` | `tcp/controlController.h` |
| `TcpPacket` | `Packet` | `tcp/packet.h` |
| `TcpPacketHeader` | `PacketHeader` | `tcp/packet.h` |
| `TcpPacketsReceiver` | `PacketsReceiver` | `tcp/packetsReceiver.h` |
| `TcpPacketsSender` | `PacketsSender` | `tcp/packetsSender.h` |

Всё в `core::network::tcp`. Внутри namespace — короткие имена (`Packet`, `PacketsReceiver` и т.д.); снаружи — `core::network::tcp::Packet` и т.п.

---

## 4. Внутренние классы UDP (клиент)

| Было | Станет | Файл |
|------|--------|------|
| `NetworkController` | `MediaController` | `udp/mediaController.h` |
| `Packet` (packet.h) | `Packet` | `udp/packet.h` |
| `PacketReceiver` | `PacketReceiver` | `udp/packetReceiver.h` |
| `PacketSender` | `PacketSender` | `udp/packetSender.h` |

В `core::network::udp`. Имена уже без префиксов; при необходимости можно уточнить (например, `MediaPacket`), но если конфликта с `tcp::Packet` нет — достаточно `udp::Packet`.

---

## 5. Сервер: TCP

| Было | Станет | Файл |
|------|--------|------|
| `TcpControlController` | `ControlController` | `tcp/controlController.h` |
| `TcpConnection` | `Connection` | `tcp/connection.h` |
| `TcpConnectionPtr` | `ConnectionPtr` | `tcp/connection.h` |
| `TcpPacket` | `Packet` | `tcp/packet.h` |
| `OwnedTcpPacket` | `OwnedPacket` | `tcp/packet.h` |
| `TcpPacketsReceiver` | `PacketsReceiver` | `tcp/packetsReceiver.h` |
| `TcpPacketsSender` | `PacketsSender` | `tcp/packetsSender.h` |

Всё в `server::network::tcp`.

---

## 6. Сервер: UDP

| Было | Станет | Файл |
|------|--------|------|
| `NetworkController` | `MediaController` | `udp/mediaController.h` |
| `PacketReceiver` | `PacketReceiver` | `udp/packetReceiver.h` |
| `PacketSender` | `PacketSender` | `udp/packetSender.h` |

В `server::network::udp`.

---

## 7. Синтаксис namespace

Использовать вложенные namespace в одну строку:

```cpp
namespace core::network::tcp {
    class ControlController { ... };
    struct Packet { ... };
}

namespace core::network::udp {
    class MediaController { ... };
}
```

Аналогично для `server::network::tcp` и `server::network::udp`.

---

## 8. Именование: camelCase, без snake_case, приватные m_camelCase

- **snake_case не использовать** — ни в коде, ни в именах файлов.
- **Классы, функции, параметры, локальные переменные, файлы:** camelCase. Примеры: `ControlController`, `getLocalMediaPort`, `controlController`, `packetReceiver.h`.
- **Приватные члены:** единственное исключение по форме — префикс `m_` + camelCase: `m_controlController`, `m_mediaController`, `m_packetsReceiver` и т.д.
- Классы: `ControlController`, `MediaController`, `PacketsReceiver`, `PacketsSender`, `Connection`, `Packet`, `PacketHeader`, `PacketReceiver`, `PacketSender`.
- Методы/параметры: `initialize` вместо `init`, `getLocalMediaPort` вместо `getLocalUdpPort` (если убираем udp из API), `onConnectionDown` и т.д.
- Переменные: `controlController`, `mediaController`, `receiver`, `sender`, `connection`, `packet` — без `tcp`/`udp` в имени.

**Допустимые исключения (сокращения):**
- `id` для идентификатора, `uid` если устоялось в протоколе.
- `Packet`, `Port` — кратко и понятно.

---

## 9. Обновление зависимых компонентов

- **Core:** `Client`, `PacketProcessor`, `AuthorizationService`, `CallService`, `MediaSharingService` — перейти на `core::network::tcp::ControlController` и `core::network::udp::MediaController`; обновить инклюды (`network/tcp/controlController.h`, `network/udp/mediaController.h` и т.д., camelCase).
- **Config / UI:** упоминания «tcp»/«udp» только в конфиге (порты, хосты) и логах; в коде — только типы из `tcp`/`udp` namespace.
- **Сервер:** `Server`, обработчики пакетов, репозиторий пользователей — перейти на `server::network::tcp::*` и `server::network::udp::*`.
- **CMake:** обновить пути к файлам при переносе в `network/tcp/` и `network/udp/` (если явно перечислены; при `GLOB_RECURSE` достаточно структуры каталогов).

---

## 10. Логика и улучшения

- **Handshake / scramble:** оставить в `utilities::crypto` или в `core::network::tcp` (и при необходимости в `server::network::tcp`); не дублировать.
- **Byte order:** общие хелперы `hostToNetwork64` / `networkToHost64` вынести в `utilities` (или `network/common`) и использовать в TCP‑handshake и пакетах.
- **Резолв (IP vs hostname):** логика «сначала IP, иначе resolver» — в одном месте (например, `tcp::resolve` или общий `network::resolve`), переиспользовать в клиенте и при необходимости в updater.
- **Очереди, таймеры:** по возможности общие типы (`SafeQueue`, таймеры) не тащить в `tcp`/`udp` лишний раз; оставить в `utilities` или существующих модулях.
- **Логи:** без префиксов `[TCP]`/`[UDP]` в коде — вынести в общий логгер с контекстом (например, «control» vs «media») или оставить только в сообщении, если нужно.

---

## 11. Порядок выполнения

1. **Подготовка**
   - Создать `client/core/src/network/tcp/` и `udp/`.
   - Создать `server/src/network/tcp/` и `udp/`.

2. **Клиент: TCP**
   - Ввести `core::network::tcp`, перенести и переименовать файлы (camelCase, без префикса `tcp_`).
   - Переименовать классы: `ControlController`, `Packet`, `PacketsReceiver`, `PacketsSender`.
   - Обновить инклюды и все использования в core.

3. **Клиент: UDP**
   - Ввести `core::network::udp`, перенести `NetworkController` → `MediaController` в `udp/`, разнести `PacketReceiver`/`PacketSender` и т.д.
   - Обновить core и все ссылки.

4. **Клиент: унификация**
   - Заменить `m_tcpControl`/`m_networkController` на `m_controlController`/`m_mediaController`.
   - Обновить сервисы, `PacketProcessor`, `Client`.

5. **Сервер: TCP**
   - Аналогично клиенту: `server::network::tcp`, перенос файлов, переименование в `ControlController`, `Connection`, `Packet`, `PacketsReceiver`/`PacketsSender`, `OwnedPacket`.

6. **Сервер: UDP**
   - `server::network::udp`, `MediaController`, `PacketReceiver`/`PacketSender`.

7. **Чистка**
   - Удалить старые файлы из `network/` (не в подпапках).
   - Убрать оставшиеся префиксы `tcp`/`udp` в идентификаторах.
   - Убедиться, что нигде не осталось snake_case (только camelCase и m_camelCase для приватных членов).
   - Проверить CMake, сборку, тесты.

8. **Улучшения логики**
   - Вынести общий resolve, byte-order, при необходимости — общие части handshake.
   - Унифицировать логирование и обработку ошибок в `tcp` и `udp`.

---

## 12. Риски и откат

- Массовые переименования и переносы: делать по шагам (сначала TCP, потом UDP), после каждого шага — сборка и прогон сценариев (логин, звонок, медиа, логаут).
- Сохранить совместимость протокола (форматы пакетов, handshake) — менять только пути, namespace и имена классов/файлов.
- При проблемах — откат по коммитам по каждому подшагу (например, «клиент TCP», «клиент UDP», «сервер TCP», «сервер UDP»).

После выполнения плана структура будет четко разделена по `tcp`/`udp`, имена унифицированы (Controller), префиксы убраны из кода, а логика при необходимости слегка упорядочена без смены протокола.
