# План улучшения архитектуры Core (Client)

## Анализ текущей архитектуры

### Выявленные проблемы

#### 1. God Object Anti-pattern в классе `Client`
Класс `Client` содержит слишком много ответственностей:
- Управление сетью (NetworkController)
- Управление аудио (AudioEngine)
- Управление состоянием (ClientStateManager)
- Обработка всех операций (authorize, logout, calls, screen sharing, camera sharing)
- Шифрование медиа-данных (дублируется в нескольких местах)
- Управление задачами (TaskManager)
- Обработка ~20+ callback методов для результатов операций
- Логика переподключения

**Проблема**: Нарушение Single Responsibility Principle, сложность тестирования, высокая связанность.

#### 2. Дублирование логики шифрования медиа-данных
Логика шифрования/дешифрования медиа-данных дублируется в:
- `Client::sendScreen()` - шифрование экрана
- `Client::sendCamera()` - шифрование камеры
- `Client::onInputVoice()` - шифрование голоса
- `PacketProcessor::onVoice()` - дешифрование голоса
- `PacketProcessor::onScreen()` - дешифрование экрана
- `PacketProcessor::onCamera()` - дешифрование камеры

**Проблема**: Нарушение DRY принципа, сложность поддержки, риск ошибок при изменении.

#### 3. Отсутствие сервисного слоя
Бизнес-логика смешана с координацией в классе `Client`:
- Валидация операций
- Создание пакетов
- Управление задачами
- Обработка результатов
- Управление состоянием

**Проблема**: Сложность тестирования бизнес-логики, нарушение разделения ответственностей.

#### 4. Прямые зависимости от деталей реализации
`Client` напрямую зависит от:
- `NetworkController` (детали сетевого уровня)
- `AudioEngine` (детали аудио-уровня)
- `ClientStateManager` (детали управления состоянием)
- `KeyManager` (детали криптографии)

**Проблема**: Нарушение Dependency Inversion Principle, сложность замены реализаций.

#### 5. Множество callback-методов
В `Client` классе около 20+ callback методов:
- `onAuthorizeCompleted/Failed`
- `onLogoutCompleted/Failed`
- `onStartOutgoingCallCompleted/Failed`
- `onStopOutgoingCallCompleted/Failed`
- `onAcceptCallCompleted/Failed/...`
- `onDeclineCallCompleted/Failed`
- `onEndCallCompleted/Failed`
- `onStartScreenSharingCompleted/Failed`
- `onStopScreenSharingCompleted/Failed`
- `onStartCameraSharingCompleted/Failed`
- `onStopCameraSharingCompleted/Failed`
- И другие...

**Проблема**: Сложность поддержки, дублирование паттернов обработки.

#### 6. Отсутствие интерфейсов для зависимостей
Нет абстракций для:
- Сетевого взаимодействия
- Аудио-обработки
- Управления состоянием
- Шифрования

**Проблема**: Невозможность мокирования для тестирования, жесткая связанность.

## Предлагаемые улучшения

### 1. Введение сервисного слоя

#### 1.1. MediaEncryptionService
**Ответственность**: Шифрование и дешифрование медиа-данных (voice, screen, camera)

**Интерфейс**:
```cpp
class IMediaEncryptionService {
public:
    virtual ~IMediaEncryptionService() = default;
    virtual std::vector<unsigned char> encryptMedia(
        const unsigned char* data, 
        int size, 
        const CryptoPP::SecByteBlock& key) = 0;
    virtual std::vector<unsigned char> decryptMedia(
        const unsigned char* data, 
        int size, 
        const CryptoPP::SecByteBlock& key) = 0;
};
```

**Преимущества**:
- Устранение дублирования логики шифрования
- Единая точка для изменений алгоритма шифрования
- Легкое тестирование

#### 1.2. CallService
**Ответственность**: Бизнес-логика управления звонками

**Интерфейс**:
```cpp
class ICallService {
public:
    virtual ~ICallService() = default;
    virtual std::error_code startOutgoingCall(const std::string& nickname) = 0;
    virtual std::error_code stopOutgoingCall() = 0;
    virtual std::error_code acceptCall(const std::string& nickname) = 0;
    virtual std::error_code declineCall(const std::string& nickname) = 0;
    virtual std::error_code endCall() = 0;
};
```

**Преимущества**:
- Инкапсуляция бизнес-логики звонков
- Упрощение тестирования
- Разделение ответственностей

#### 1.3. AuthorizationService
**Ответственность**: Бизнес-логика авторизации и переподключения

**Интерфейс**:
```cpp
class IAuthorizationService {
public:
    virtual ~IAuthorizationService() = default;
    virtual std::error_code authorize(const std::string& nickname) = 0;
    virtual std::error_code logout() = 0;
    virtual void handleReconnect() = 0;
};
```

#### 1.4. MediaSharingService
**Ответственность**: Бизнес-логика screen/camera sharing

**Интерфейс**:
```cpp
class IMediaSharingService {
public:
    virtual ~IMediaSharingService() = default;
    virtual std::error_code startScreenSharing() = 0;
    virtual std::error_code stopScreenSharing() = 0;
    virtual std::error_code sendScreen(const std::vector<unsigned char>& data) = 0;
    virtual std::error_code startCameraSharing() = 0;
    virtual std::error_code stopCameraSharing() = 0;
    virtual std::error_code sendCamera(const std::vector<unsigned char>& data) = 0;
};
```

### 2. Введение интерфейсов для зависимостей

#### 2.1. INetworkService
**Ответственность**: Абстракция сетевого взаимодействия

```cpp
class INetworkService {
public:
    virtual ~INetworkService() = default;
    virtual bool send(const std::vector<unsigned char>& data, uint32_t type) = 0;
    virtual bool isConnected() const = 0;
};
```

#### 2.2. IAudioService
**Ответственность**: Абстракция аудио-обработки

```cpp
class IAudioService {
public:
    virtual ~IAudioService() = default;
    virtual void muteMicrophone(bool isMute) = 0;
    virtual void muteSpeaker(bool isMute) = 0;
    virtual void setInputVolume(int volume) = 0;
    virtual void setOutputVolume(int volume) = 0;
    // ... другие методы
};
```

#### 2.3. IStateRepository
**Ответственность**: Абстракция управления состоянием

```cpp
class IStateRepository {
public:
    virtual ~IStateRepository() = default;
    virtual bool isAuthorized() const = 0;
    virtual bool isActiveCall() const = 0;
    virtual const Call& getActiveCall() const = 0;
    // ... другие методы состояния
};
```

### 3. Рефакторинг класса Client

#### 3.1. Упрощение Client до координатора
`Client` должен стать тонким координатором, который:
- Делегирует бизнес-логику сервисам
- Координирует взаимодействие между сервисами
- Предоставляет публичный API

#### 3.2. Устранение callback-методов
Вместо множества callback-методов использовать:
- Command Pattern для операций
- Result Pattern для обработки результатов
- Event-driven подход через EventListener

### 4. Структура после рефакторинга

```
Client (координатор)
├── Services (бизнес-логика)
│   ├── CallService
│   ├── AuthorizationService
│   ├── MediaSharingService
│   └── MediaEncryptionService
├── Interfaces (абстракции)
│   ├── INetworkService
│   ├── IAudioService
│   └── IStateRepository
└── Infrastructure (реализации)
    ├── NetworkController (реализует INetworkService)
    ├── AudioEngine (реализует IAudioService)
    └── ClientStateManager (реализует IStateRepository)
```

## План реализации

### Этап 1: Создание интерфейсов
1. Создать `IMediaEncryptionService` и реализацию
2. Создать `INetworkService`, `IAudioService`, `IStateRepository`
3. Создать адаптеры для существующих классов

### Этап 2: Создание сервисов
1. Создать `MediaEncryptionService` - устранить дублирование шифрования
2. Создать `CallService` - вынести логику звонков
3. Создать `AuthorizationService` - вынести логику авторизации
4. Создать `MediaSharingService` - вынести логику sharing

### Этап 3: Рефакторинг Client
1. Заменить прямые вызовы на использование сервисов
2. Упростить callback-методы
3. Убрать дублирование логики

### Этап 4: Тестирование
1. Написать unit-тесты для сервисов
2. Написать интеграционные тесты
3. Проверить все сценарии использования

## Ожидаемые результаты

1. **Улучшение тестируемости**: Сервисы можно тестировать изолированно
2. **Снижение связанности**: Зависимости через интерфейсы
3. **Устранение дублирования**: Единая логика шифрования
4. **Упрощение Client**: Тонкий координатор вместо God Object
5. **Легкость расширения**: Новые функции через новые сервисы
