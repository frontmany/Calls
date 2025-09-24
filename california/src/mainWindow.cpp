#include "mainWindow.h"
#include <QStackedLayout>
#include <QHBoxLayout>
#include <QFontDatabase>

#include "AuthorizationWidget.h"
#include "MainMenuWidget.h"

MainWindow::MainWindow(QWidget* parent, const std::string& host)
    : QMainWindow(parent) {

    calls::init(host,
        [this](calls::Result authorizationResult) { onAuthorizationResult(authorizationResult); },
        [this](calls::Result createCallResult) { onCreateCallResult(createCallResult); },
        [this](const std::string& friendNickName) { onIncomingCall(friendNickName); },
        [this](const std::string& friendNickName) { onIncomingCallExpired(friendNickName); },
        [this]() { onCallHangUp(); },
        [this]() { onNetworkError(); }
    );

    if (QFontDatabase::addApplicationFont(":/resources/Pacifico-Regular.ttf") == -1) {
        qWarning() << "Font load error:";
    }

    if (QFontDatabase::addApplicationFont(":/resources/Outfit-VariableFont_wght.ttf") == -1) {
        qWarning() << "Font load error:";
    }

    setupUI();



    switchToMainMenuWidget();
    m_mainMenuWidget->setNickname("feder");
    m_mainMenuWidget->setStatus(calls::ClientStatus::FREE);

    /*
    // Создаем таймер для добавления тестовых звонков
    QTimer* testTimer = new QTimer(this);
    int callCounter = 0;

    connect(testTimer, &QTimer::timeout, this, [this, callCounter]() mutable {
        // mutable позволяет изменять захваченную копию
        int currentCounter = callCounter + 1;
        QString testNickname = QString("test_user_%1").arg(currentCounter);
        m_mainMenuWidget->addIncomingCall(testNickname);

        qDebug() << "Added test call from:" << testNickname;

        // Увеличиваем локальную копию (не оригинал)
        callCounter++;
    });

    // Или лучше сделать callCounter членом класса

    // Запускаем таймер: добавляем звонок каждые 5 секунд
    testTimer->start(5000);
    */



    // Также можно сразу добавить первый звонок
    //m_mainMenuWidget->addIncomingCall("qwerty");
    //m_mainMenuWidget->addIncomingCall("asdf");
}

void MainWindow::setupUI() {
    // Создаем центральный виджет
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    // Создаем главный layout
    m_mainLayout = new QHBoxLayout(m_centralWidget);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // Создаем stacked layout для переключения между виджетами
    m_stackedLayout = new QStackedLayout();
    m_mainLayout->addLayout(m_stackedLayout);

    // Создаем виджет авторизации
    m_authorizationWidget = new AuthorizationWidget(this);
    m_stackedLayout->addWidget(m_authorizationWidget);

    // Создаем виджет главного меню
    m_mainMenuWidget = new MainMenuWidget(this);
    m_stackedLayout->addWidget(m_mainMenuWidget);

    // Устанавливаем виджет авторизации по умолчанию
    switchToAuthorizationWidget();
}

void MainWindow::switchToAuthorizationWidget() {
    m_stackedLayout->setCurrentWidget(m_authorizationWidget);
    setWindowTitle("Authorization - Callifornia");
}

void MainWindow::switchToMainMenuWidget() {
    m_stackedLayout->setCurrentWidget(m_mainMenuWidget);
    setWindowTitle("Callifornia");

    // Устанавливаем никнейм (пока заглушка)
    m_mainMenuWidget->setNickname("TestUser");
}

void MainWindow::onAuthorizationResult(calls::Result authorizationResult) {
    if (authorizationResult == calls::Result::SUCCESS) {
        m_mainMenuWidget->setNickname(QString::fromStdString(calls::getNickname()));
        switchToMainMenuWidget();

        // Устанавливаем реальный никнейм из calls клиента
        std::string nickname = calls::getNickname();
        if (!nickname.empty()) {
            m_mainMenuWidget->setNickname(QString::fromStdString(nickname));
        }
    }
    else {
        // Показываем ошибку в виджете авторизации
        QString errorMessage;
        switch (authorizationResult) {
        case calls::Result::FAIL:
            errorMessage = "Authorization failed";
            break;
        case calls::Result::TIMEOUT:
            errorMessage = "Authorization timeout";
            break;
        default:
            errorMessage = "Unknown error";
            break;
        }
        m_authorizationWidget->setErrorMessage(errorMessage);
        m_authorizationWidget->reset(); // Разблокируем кнопку для повторной попытки
    }
}

void MainWindow::onCreateCallResult(calls::Result createCallResult) {
    // Обработка результата создания звонка
    // Можно показывать уведомления в главном меню
}

void MainWindow::onIncomingCall(const std::string& friendNickName) {
    // Обработка входящего звонка
    // Добавляем входящий вызов в главное меню
    m_mainMenuWidget->addIncomingCall(QString::fromStdString(friendNickName));
}

void MainWindow::onIncomingCallExpired(const std::string& friendNickName) {
    // Обработка истечения времени входящего звонка
    // Удаляем из списка входящих вызовов
    m_mainMenuWidget->removeIncomingCall(QString::fromStdString(friendNickName));
}

void MainWindow::onCallHangUp() {
    // Обработка завершения звонка
    // Очищаем список входящих вызовов
    m_mainMenuWidget->clearIncomingCalls();
}

void MainWindow::onNetworkError() {
    // Обработка сетевой ошибки
    // Можно показать сообщение об ошибке и переключиться на авторизацию
    switchToAuthorizationWidget();
    m_authorizationWidget->setErrorMessage("Network error occurred");
}