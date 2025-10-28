#include <iostream>
#include <thread>
#include <chrono>
#include "clientUpdater.h"

using namespace updater;

// Функции обратного вызова
void handleCheckResult(CheckResult result) {
    std::cout << "Check result received: ";

    switch (result) {
    case CheckResult::UPDATE_NOT_NEEDED:
        std::cout << "UPDATE_NOT_NEEDED - Ваша версия актуальна" << std::endl;
        break;
    case CheckResult::REQUIRED_UPDATE:
        std::cout << "REQUIRED_UPDATE - Требуется обязательное обновление" << std::endl;
        break;
    case CheckResult::POSSIBLE_UPDATE:
        std::cout << "POSSIBLE_UPDATE - Доступно опциональное обновление" << std::endl;
        break;
    }
}

void handleUpdateLoaded() {
    std::cout << "Обновление успешно загружено и готово к установке" << std::endl;
}

void handleError() {
    std::cout << "Произошла ошибка при проверке или загрузке обновления" << std::endl;
}

// Функция для симуляции пользовательского интерфейса
void simulateUserInteraction(ClientUpdater& updater) {
    while (true) {
        std::cout << "\n=== Менеджер обновлений ===" << std::endl;
        std::cout << "Текущее состояние: ";

        switch (updater.getState()) {
        case ClientUpdater::State::DISCONNECTED:
            std::cout << "DISCONNECTED" << std::endl;
            break;
        case ClientUpdater::State::AWAITING_SERVER_RESPONSE:
            std::cout << "AWAITING_SERVER_RESPONSE" << std::endl;
            break;
        case ClientUpdater::State::AWAITING_UPDATES_CHECK:
            std::cout << "AWAITING_UPDATES_CHECK" << std::endl;
            break;
        case ClientUpdater::State::AWAITING_START_UPDATE:
            std::cout << "AWAITING_START_UPDATE" << std::endl;
            break;
        }

        std::cout << "1. Проверить обновления" << std::endl;
        std::cout << "2. Установить обновление (Windows)" << std::endl;
        std::cout << "3. Установить обновление (Linux)" << std::endl;
        std::cout << "4. Установить обновление (Mac)" << std::endl;
        std::cout << "5. Отключиться" << std::endl;
        std::cout << "6. Выход" << std::endl;
        std::cout << "Выберите действие: ";

        int choice;
        std::cin >> choice;

        switch (choice) {
        case 1:
            if (updater.getState() != ClientUpdater::State::DISCONNECTED) {
                std::cout << "Проверка обновлений..." << std::endl;
                if (!updater.checkUpdates("1.0.1")) {
                    std::cout << "Не удалось запустить проверку обновлений" << std::endl;
                }
            }
            else {
                std::cout << "Ошибка: клиент отключен. Сначала подключитесь к серверу." << std::endl;
            }
            break;
        case 2:
            if (updater.getState() != ClientUpdater::State::DISCONNECTED) {
                std::cout << "Запуск обновления для Windows..." << std::endl;
                if (!updater.startUpdate(OperationSystemType::WINDOWS)) {
                    std::cout << "Не удалось запустить обновление" << std::endl;
                }
            }
            else {
                std::cout << "Ошибка: клиент отключен. Сначала подключитесь к серверу." << std::endl;
            }
            break;
        case 3:
            if (updater.getState() != ClientUpdater::State::DISCONNECTED) {
                std::cout << "Запуск обновления для Linux..." << std::endl;
                if (!updater.startUpdate(OperationSystemType::LINUX)) {
                    std::cout << "Не удалось запустить обновление" << std::endl;
                }
            }
            else {
                std::cout << "Ошибка: клиент отключен. Сначала подключитесь к серверу." << std::endl;
            }
            break;
        case 4:
            if (updater.getState() != ClientUpdater::State::DISCONNECTED) {
                std::cout << "Запуск обновления для Mac..." << std::endl;
                if (!updater.startUpdate(OperationSystemType::MAC)) {
                    std::cout << "Не удалось запустить обновление" << std::endl;
                }
            }
            else {
                std::cout << "Ошибка: клиент отключен. Сначала подключитесь к серверу." << std::endl;
            }
            break;
        case 5:
            std::cout << "Отключение..." << std::endl;
            updater.disconnect();
            break;
        case 6:
            std::cout << "Выход..." << std::endl;
            return;
        default:
            std::cout << "Неверный выбор!" << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

int main() {
    setlocale(LC_ALL, "ru");

    try {
        ClientUpdater updater(handleCheckResult, handleUpdateLoaded, handleError);

        std::cout << "Подключение к серверу обновлений..." << std::endl;
        updater.connect("192.168.1.44", "8081");

        std::this_thread::sleep_for(std::chrono::seconds(1));

        simulateUserInteraction(updater);

        updater.disconnect();

    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

// Дополнительный пример с автоматической проверкой
void automaticUpdateExample() {
    std::cout << "\n=== Автоматическая проверка обновлений ===" << std::endl;

    ClientUpdater autoUpdater(
        [](CheckResult result) {
            std::cout << "Автоматическая проверка: ";
            if (result == CheckResult::REQUIRED_UPDATE) {
                std::cout << "Обязательное обновление найдено, запускаем установку..." << std::endl;
                // Здесь можно автоматически запустить обновление
            }
            else if (result == CheckResult::POSSIBLE_UPDATE) {
                std::cout << "Доступно опциональное обновление" << std::endl;
            }
            else {
                std::cout << "Обновления не требуются" << std::endl;
            }
        },
        []() {
            std::cout << "Автообновление: Обновление загружено успешно" << std::endl;
        },
        []() {
            std::cout << "Автообновление: Ошибка при обновлении" << std::endl;
        }
    );

    // Подключаемся и проверяем обновления
    autoUpdater.connect("192.168.1.44", "8081");
    std::this_thread::sleep_for(std::chrono::seconds(1));

    if (autoUpdater.getState() != ClientUpdater::State::DISCONNECTED) {
        autoUpdater.checkUpdates("1.0.1");
    }
    else {
        std::cout << "Не удалось подключиться к серверу обновлений" << std::endl;
    }

    // Ждем завершения операций
    std::this_thread::sleep_for(std::chrono::seconds(3));
    autoUpdater.disconnect();
}

// Для компиляции может потребоваться запустить automaticUpdateExample()
// в main() вместо simulateUserInteraction()