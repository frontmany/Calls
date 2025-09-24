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
    // ������� ������ ��� ���������� �������� �������
    QTimer* testTimer = new QTimer(this);
    int callCounter = 0;

    connect(testTimer, &QTimer::timeout, this, [this, callCounter]() mutable {
        // mutable ��������� �������� ����������� �����
        int currentCounter = callCounter + 1;
        QString testNickname = QString("test_user_%1").arg(currentCounter);
        m_mainMenuWidget->addIncomingCall(testNickname);

        qDebug() << "Added test call from:" << testNickname;

        // ����������� ��������� ����� (�� ��������)
        callCounter++;
    });

    // ��� ����� ������� callCounter ������ ������

    // ��������� ������: ��������� ������ ������ 5 ������
    testTimer->start(5000);
    */



    // ����� ����� ����� �������� ������ ������
    //m_mainMenuWidget->addIncomingCall("qwerty");
    //m_mainMenuWidget->addIncomingCall("asdf");
}

void MainWindow::setupUI() {
    // ������� ����������� ������
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    // ������� ������� layout
    m_mainLayout = new QHBoxLayout(m_centralWidget);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // ������� stacked layout ��� ������������ ����� ���������
    m_stackedLayout = new QStackedLayout();
    m_mainLayout->addLayout(m_stackedLayout);

    // ������� ������ �����������
    m_authorizationWidget = new AuthorizationWidget(this);
    m_stackedLayout->addWidget(m_authorizationWidget);

    // ������� ������ �������� ����
    m_mainMenuWidget = new MainMenuWidget(this);
    m_stackedLayout->addWidget(m_mainMenuWidget);

    // ������������� ������ ����������� �� ���������
    switchToAuthorizationWidget();
}

void MainWindow::switchToAuthorizationWidget() {
    m_stackedLayout->setCurrentWidget(m_authorizationWidget);
    setWindowTitle("Authorization - Callifornia");
}

void MainWindow::switchToMainMenuWidget() {
    m_stackedLayout->setCurrentWidget(m_mainMenuWidget);
    setWindowTitle("Callifornia");

    // ������������� ������� (���� ��������)
    m_mainMenuWidget->setNickname("TestUser");
}

void MainWindow::onAuthorizationResult(calls::Result authorizationResult) {
    if (authorizationResult == calls::Result::SUCCESS) {
        m_mainMenuWidget->setNickname(QString::fromStdString(calls::getNickname()));
        switchToMainMenuWidget();

        // ������������� �������� ������� �� calls �������
        std::string nickname = calls::getNickname();
        if (!nickname.empty()) {
            m_mainMenuWidget->setNickname(QString::fromStdString(nickname));
        }
    }
    else {
        // ���������� ������ � ������� �����������
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
        m_authorizationWidget->reset(); // ������������ ������ ��� ��������� �������
    }
}

void MainWindow::onCreateCallResult(calls::Result createCallResult) {
    // ��������� ���������� �������� ������
    // ����� ���������� ����������� � ������� ����
}

void MainWindow::onIncomingCall(const std::string& friendNickName) {
    // ��������� ��������� ������
    // ��������� �������� ����� � ������� ����
    m_mainMenuWidget->addIncomingCall(QString::fromStdString(friendNickName));
}

void MainWindow::onIncomingCallExpired(const std::string& friendNickName) {
    // ��������� ��������� ������� ��������� ������
    // ������� �� ������ �������� �������
    m_mainMenuWidget->removeIncomingCall(QString::fromStdString(friendNickName));
}

void MainWindow::onCallHangUp() {
    // ��������� ���������� ������
    // ������� ������ �������� �������
    m_mainMenuWidget->clearIncomingCalls();
}

void MainWindow::onNetworkError() {
    // ��������� ������� ������
    // ����� �������� ��������� �� ������ � ������������� �� �����������
    switchToAuthorizationWidget();
    m_authorizationWidget->setErrorMessage("Network error occurred");
}