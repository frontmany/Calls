#include "mainWindow.h"
#include <QStackedLayout>
#include <QHBoxLayout>
#include <QFontDatabase>
#include <QApplication>
#include <QSoundEffect>
#include <QFileInfo>

#include "AuthorizationWidget.h"
#include "MainMenuWidget.h"
#include "OverlayWidget.h"
#include "CallWidget.h"

MainWindow::MainWindow(QWidget* parent, const std::string& host, const std::string& port)
    : QMainWindow(parent)
{
    setWindowIcon(QIcon(":/resources/callifornia.ico"));

    m_ringtonePlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_ringtonePlayer->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(0.4f);
    m_ringtonePlayer->setSource(QUrl("qrc:/resources/ringtone.mp3"));

    connect(m_ringtonePlayer, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state) {
        if (state == QMediaPlayer::StoppedState || state == QMediaPlayer::PausedState) {
            if (calls::getIncomingCallsCount() != 0) {
                m_ringtonePlayer->play();
            }
        }
    });

    loadFonts();
    setupUI();

    std::unique_ptr<CallsClientHandler> callsClientHandler = std::make_unique<CallsClientHandler>(this);

    bool initialized = calls::init(host, port, std::move(callsClientHandler));
    
    if (initialized) {
        calls::run();
    }
    else {
        QTimer::singleShot(0, this, [this]() {
            showInitializationErrorDialog();
        });
    }
    

    showMaximized();
}

MainWindow::~MainWindow() {
    if (m_ringtonePlayer) {
        m_ringtonePlayer->stop();
    }
}

void MainWindow::playRingtone() {
    if (!m_ringtonePlayer) return;

    if (m_ringtonePlayer->playbackState() != QMediaPlayer::PlayingState) {
        m_ringtonePlayer->setLoops(QMediaPlayer::Infinite);
        m_ringtonePlayer->play();
    }
}

void MainWindow::pauseRingtone() {
    if (!m_ringtonePlayer) return;

    if (m_ringtonePlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_ringtonePlayer->pause();
    }
}

void MainWindow::playSoundEffect(const QString& soundPath) {
    QSoundEffect* effect = new QSoundEffect(this);
    effect->setSource(QUrl::fromLocalFile((soundPath)));
    effect->setVolume(1.0f);
    effect->play();

    connect(effect, &QSoundEffect::playingChanged, this, [effect]() {
        if (!effect->isPlaying()) {
            effect->deleteLater();
        }
    });
}

void MainWindow::loadFonts() {
    if (QFontDatabase::addApplicationFont(":/resources/Pacifico-Regular.ttf") == -1) {
        qWarning() << "Font load error:";
    }

    if (QFontDatabase::addApplicationFont(":/resources/Outfit-VariableFont_wght.ttf") == -1) {
        qWarning() << "Font load error:";
    }
}

void MainWindow::setupUI() {
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    m_mainLayout = new QHBoxLayout(m_centralWidget);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    m_stackedLayout = new QStackedLayout();
    m_mainLayout->addLayout(m_stackedLayout);

    m_authorizationWidget = new AuthorizationWidget(this);
    connect(m_authorizationWidget, &AuthorizationWidget::authorizationButtonClicked, this, &MainWindow::onAuthorizationButtonClicked);
    connect(m_authorizationWidget, &AuthorizationWidget::blurAnimationFinished, this, &MainWindow::onBlurAnimationFinished);

    m_stackedLayout->addWidget(m_authorizationWidget);

    m_mainMenuWidget = new MainMenuWidget(this);
    connect(m_mainMenuWidget, &MainMenuWidget::acceptCallButtonClicked, this, &MainWindow::onAcceptCallButtonClicked);
    connect(m_mainMenuWidget, &MainMenuWidget::declineCallButtonClicked, this, &MainWindow::onDeclineCallButtonClicked);
    connect(m_mainMenuWidget, &MainMenuWidget::createCallButtonClicked, this, &MainWindow::onStartCallingButtonClicked);
    connect(m_mainMenuWidget, &MainMenuWidget::stopCallingButtonClicked, this, &MainWindow::onStopCallingButtonClicked);
    connect(m_mainMenuWidget, &MainMenuWidget::refreshAudioDevicesButtonClicked, this, &MainWindow::onRefreshAudioDevicesButtonClicked);
    connect(m_mainMenuWidget, &MainMenuWidget::inputVolumeChanged, this, &MainWindow::onInputVolumeChanged);
    connect(m_mainMenuWidget, &MainMenuWidget::outputVolumeChanged, this, &MainWindow::onOutputVolumeChanged);
    connect(m_mainMenuWidget, &MainMenuWidget::muteButtonClicked, this, &MainWindow::onMuteButtonClicked);
    m_stackedLayout->addWidget(m_mainMenuWidget);

    m_callWidget = new CallWidget(this);
    connect(m_callWidget, &CallWidget::hangupClicked, this, &MainWindow::onEndCallButtonClicked);
    connect(m_callWidget, &CallWidget::refreshAudioDevicesButtonClicked, this, &MainWindow::onRefreshAudioDevicesButtonClicked);
    connect(m_callWidget, &CallWidget::inputVolumeChanged, this, &MainWindow::onInputVolumeChanged);
    connect(m_callWidget, &CallWidget::outputVolumeChanged, this, &MainWindow::onOutputVolumeChanged);
    connect(m_callWidget, &CallWidget::muteButtonClicked, this, &MainWindow::onMuteButtonClicked);
    connect(m_callWidget, &CallWidget::acceptCallButtonClicked, this, &MainWindow::onAcceptCallButtonClicked);
    connect(m_callWidget, &CallWidget::declineCallButtonClicked, this, &MainWindow::onDeclineCallButtonClicked);
    m_stackedLayout->addWidget(m_callWidget);

    switchToAuthorizationWidget();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (calls::getState() == calls::State::UNAUTHORIZED) {
        event->accept();
    }
    else {
        bool initiated = calls::initiateShutdown();
        if (initiated) {
            event->ignore();
        }
        else {
            calls::forceShutdown();
            event->accept();
        }
    }
}


//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------

void MainWindow::switchToAuthorizationWidget() {
    m_stackedLayout->setCurrentWidget(m_authorizationWidget);
    setWindowTitle("Authorization - Callifornia");
}

void MainWindow::switchToMainMenuWidget() {
    m_stackedLayout->setCurrentWidget(m_mainMenuWidget);
    m_mainMenuWidget->setInputVolume(calls::getInputVolume());
    m_mainMenuWidget->setOutputVolume(calls::getOutputVolume());
    m_mainMenuWidget->setMuted(calls::isMuted());

    setWindowTitle("Callifornia");

    std::string nickname = calls::getNickname();
    if (!nickname.empty()) {
        m_mainMenuWidget->setNickname(QString::fromStdString(nickname));
    }
}

void MainWindow::switchToCallWidget(const QString& friendNickname) {
    m_stackedLayout->setCurrentWidget(m_callWidget);

    m_callWidget->setInputVolume(calls::getInputVolume());
    m_callWidget->setOutputVolume(calls::getOutputVolume());
    m_callWidget->setMuted(calls::isMuted());

    setWindowTitle("Call In Progress - Callifornia");
    m_callWidget->setCallInfo(friendNickname);
}

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------

void MainWindow::onBlurAnimationFinished() {
    if (calls::getState() != calls::State::UNAUTHORIZED) {
        switchToMainMenuWidget();

        m_mainMenuWidget->setState(calls::State::FREE);

        std::string nickname = calls::getNickname();
        if (!nickname.empty()) {
            m_mainMenuWidget->setNickname(QString::fromStdString(nickname));
        }

        m_mainMenuWidget->setFocusToLineEdit();
    }
    else {
        m_authorizationWidget->resetBlur();
        QString errorMessage = "Authorization failed";
        m_authorizationWidget->setErrorMessage(errorMessage);
    }
}

void MainWindow::onAuthorizationButtonClicked(const QString& friendNickname) {
    bool requestSent = calls::authorize(friendNickname.toStdString());
    if (requestSent) {
        m_authorizationWidget->startBlurAnimation();
    }
    else {
        m_authorizationWidget->resetBlur();
        QString errorMessage = "Failed to send authorization request. Please try again.";
        m_authorizationWidget->setErrorMessage(errorMessage);
    }
}

void MainWindow::onEndCallButtonClicked() {
    bool requestSent = calls::endCall();
    if (!requestSent) {
        handleEndCallErrorNotificationAppearance();
    }
}

void MainWindow::onRefreshAudioDevicesButtonClicked() {
    calls::refreshAudioDevices();
}

void MainWindow::onInputVolumeChanged(int newVolume) {
    calls::setInputVolume(newVolume);
}

void MainWindow::onOutputVolumeChanged(int newVolume) {
    calls::setOutputVolume(newVolume);
}

void MainWindow::onMuteButtonClicked(bool mute) {
    calls::mute(mute);
}

void MainWindow::onAcceptCallButtonClicked(const QString& friendNickname) {
    bool requestSent = calls::acceptCall(friendNickname.toStdString());
    if (!requestSent) {
        handleAcceptCallErrorNotificationAppearance();
    }
}

void MainWindow::onDeclineCallButtonClicked(const QString& friendNickname) {
    bool requestSent = calls::declineCall(friendNickname.toStdString());
    if (!requestSent) {
        handleDeclineCallErrorNotificationAppearance();
    }
}

void MainWindow::onStartCallingButtonClicked(const QString& friendNickname) {
    if (!friendNickname.isEmpty() && friendNickname.toStdString() != calls::getNickname()) {
        bool requestSent = calls::startCalling(friendNickname.toStdString());
        if (!requestSent) {
            handleStartCallingErrorNotificationAppearance();
        }
    }
    else {
        if (friendNickname.isEmpty()) {
            m_mainMenuWidget->setErrorMessage("cannot be empty");
        }
        else if (friendNickname.toStdString() == calls::getNickname()) {
            m_mainMenuWidget->setErrorMessage("cannot call yourself");
        }
    }
}

void MainWindow::onStopCallingButtonClicked() {
    bool requestSent = calls::stopCalling();
    if (!requestSent) {
        handleStopCallingErrorNotificationAppearance();
    }
}
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------

void MainWindow::handleAcceptCallErrorNotificationAppearance() {
    QString errorText = "Failed to accept call. Please try again";

    if (m_stackedLayout->currentWidget() == m_mainMenuWidget) {
        m_mainMenuWidget->showErrorNotification(errorText, 1500);
    }
    else if (m_stackedLayout->currentWidget() == m_callWidget) {
        m_callWidget->showErrorNotification(errorText, 1500);
    }
    else {
        DEBUG_LOG("Trying to accept call from weird widget");
    }
}

void MainWindow::handleDeclineCallErrorNotificationAppearance() {
    QString errorText = "Failed to decline call. Please try again";

    if (m_stackedLayout->currentWidget() == m_mainMenuWidget) {
        m_mainMenuWidget->showErrorNotification(errorText, 1500);
    }
    else if (m_stackedLayout->currentWidget() == m_callWidget) {
        m_callWidget->showErrorNotification(errorText, 1500);
    }
    else {
        DEBUG_LOG("Trying to decline call from weird widget");
    }
}

void MainWindow::handleDeclineAllCallsErrorNotificationAppearance() {
    QString errorText = "Joined the call, but was unable to automatically decline other invitations";

    if (m_stackedLayout->currentWidget() == m_callWidget) {
        m_callWidget->showErrorNotification(errorText, 1500);

        auto incomingCalls = m_mainMenuWidget->getIncomingCalls();
        for (auto& [nickname, remainingTime] : incomingCalls) {
            m_callWidget->addIncomingCall(QString::fromStdString(nickname), remainingTime);
        }
    }
    else {
        DEBUG_LOG("Trying to decline all calls from weird widget");
    }
}

void MainWindow::handleStartCallingErrorNotificationAppearance() {
    QString errorText = "Failed to start calling. Please try again";

    if (m_stackedLayout->currentWidget() == m_mainMenuWidget) {
        m_mainMenuWidget->showErrorNotification(errorText, 1500);
    }
    else {
        DEBUG_LOG("Trying to start calling from weird widget");
    }
}

void MainWindow::handleStopCallingErrorNotificationAppearance() {
    QString errorText = "Failed to stop calling. Please try again";

    if (m_stackedLayout->currentWidget() == m_mainMenuWidget) {
        m_mainMenuWidget->showErrorNotification(errorText, 1500);
    }
    else {
        DEBUG_LOG("Trying to stop calling from weird widget");
    }
}

void MainWindow::handleEndCallErrorNotificationAppearance() {
    QString errorText = "Failed to end call. Please try again";

    if (m_stackedLayout->currentWidget() == m_callWidget) {
        m_callWidget->showErrorNotification(errorText, 1500);
    }
    else {
        DEBUG_LOG("Trying to end call from weird widget");
    }
}

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------

void MainWindow::onAuthorizationResult(bool success) {}

void MainWindow::onLogoutResult(bool success) {

}

void MainWindow::onShutdownResult(bool success) {
    if (success) {
        calls::completeShutdown();
        QCoreApplication::quit();
    }
    else {
        calls::forceShutdown();
        QCoreApplication::quit();
    }
}

void MainWindow::onStartCallingResult(bool success) {
    if (success) {
        m_mainMenuWidget->showCallingPanel(QString::fromStdString(calls::getNicknameWhomCalling()));
        m_mainMenuWidget->setState(calls::State::CALLING);
    }
    else {
        QString errorMessage = "cannot call";
        m_mainMenuWidget->removeCallingPanel();
        m_mainMenuWidget->setState(calls::State::FREE);
        m_mainMenuWidget->setErrorMessage(errorMessage);
    }
}

void MainWindow::onCallingStoppedResult(bool success) {
    if (success) {
        m_mainMenuWidget->removeCallingPanel();
        m_mainMenuWidget->setState(calls::State::FREE);
    }
    else {
        handleStopCallingErrorNotificationAppearance();
    }
}

void MainWindow::onDeclineIncomingCallResult(bool success, QString nickname) {
    if (success) {
        if (calls::getIncomingCallsCount() == 0) {
            pauseRingtone();
        }

        m_mainMenuWidget->removeIncomingCall(nickname);
        playSoundEffect(":/resources/callingEndedByMe.wav");
    }
    else {
        void handleDeclineCallErrorNotificationAppearance();
    }
}

void MainWindow::onAcceptIncomingCallResult(bool success, QString nickname) {
    if (success) {
        m_mainMenuWidget->removeCallingPanel();

        m_mainMenuWidget->clearIncomingCalls();

        if (m_stackedLayout->currentWidget() == m_callWidget)
            m_callWidget->clearIncomingCalls();


        switchToCallWidget(nickname);
        pauseRingtone();
    }
    else {
        handleAcceptCallErrorNotificationAppearance();
    }
}

void MainWindow::onAllIncomingCallsDeclinedResult(bool success) {
    if (success) {
        m_mainMenuWidget->clearIncomingCalls();

        if (m_stackedLayout->currentWidget() == m_callWidget)
            m_callWidget->clearIncomingCalls();
    }
    else {
        handleDeclineAllCallsErrorNotificationAppearance();
    }
}

void MainWindow::onEndCallResult(bool success) {
    if (success) {
        m_mainMenuWidget->setState(calls::State::FREE);
        switchToMainMenuWidget();
        m_callWidget->clearIncomingCalls();
        playSoundEffect(":/resources/callEnd.wav");
    }
    else {
        handleEndCallErrorNotificationAppearance();
    }
}

void MainWindow::onCallingAccepted() {
    pauseRingtone();
    m_mainMenuWidget->removeCallingPanel();
    m_mainMenuWidget->setState(calls::State::BUSY);
    switchToCallWidget(QString::fromStdString(calls::getNicknameInCallWith()));

    if (calls::getIncomingCallsCount() > 0) {
        bool requestSent = calls::declineAllCalls();
        if (!requestSent) {
            handleDeclineAllCallsErrorNotificationAppearance();
        }
    }
}

void MainWindow::onCallingDeclined() {
    m_mainMenuWidget->removeCallingPanel();
    m_mainMenuWidget->setState(calls::State::FREE);
}

void MainWindow::onRemoteUserEndedCall() {
    switchToMainMenuWidget();

    m_mainMenuWidget->setState(calls::State::FREE);
    playSoundEffect(":/resources/callEnd.wav");
}

void MainWindow::onIncomingCall(QString friendNickName) {
    playRingtone();

    m_mainMenuWidget->addIncomingCall(friendNickName);

    if (calls::getState() == calls::State::BUSY) {
        m_callWidget->addIncomingCall(friendNickName);
    }
}

void MainWindow::onIncomingCallEnded(QString friendNickName) {
    m_mainMenuWidget->removeIncomingCall(friendNickName);

    if (calls::getState() == calls::State::BUSY) {
        m_callWidget->removeIncomingCall(friendNickName);
    }

    if (calls::getIncomingCallsCount() == 0) {
        pauseRingtone();
    }


    playSoundEffect(":/resources/callingEndedByRemote.wav");
}

void MainWindow::onNetworkError() {
    calls::reset();

    pauseRingtone();
    m_mainMenuWidget->removeCallingPanel();
    m_mainMenuWidget->clearIncomingCalls();

    if (m_stackedLayout->currentWidget() != m_authorizationWidget) {
        switchToAuthorizationWidget();
    }

    m_authorizationWidget->resetBlur();
    m_authorizationWidget->showNetworkErrorNotification();
}

void MainWindow::onConnectionRestored() {
    m_authorizationWidget->resetBlur();
    m_authorizationWidget->showConnectionRestoredNotification(1500);
}


void MainWindow::showInitializationErrorDialog()
{
    QFont font("Outfit", 16, QFont::Normal);

    OverlayWidget* overlay = new OverlayWidget(this);
    overlay->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    overlay->setAttribute(Qt::WA_TranslucentBackground);
    overlay->showMaximized();

    QDialog* dialog = new QDialog(overlay);
    dialog->setWindowTitle("Initialization problem");
    dialog->setMinimumWidth(500);
    dialog->setMinimumHeight(270);
    dialog->setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    dialog->setAttribute(Qt::WA_TranslucentBackground);

    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
    shadowEffect->setBlurRadius(20);
    shadowEffect->setXOffset(0);
    shadowEffect->setYOffset(0);
    shadowEffect->setColor(QColor(0, 0, 0, 120));

    QWidget* mainWidget = new QWidget(dialog);
    mainWidget->setGraphicsEffect(shadowEffect);
    mainWidget->setObjectName("mainWidget");

    QString mainWidgetStyle =
        "QWidget#mainWidget {"
        "   background-color: rgb(229, 228, 226);"
        "   border-radius: 12px;"
        "}";
    mainWidget->setStyleSheet(mainWidgetStyle);

    QVBoxLayout* mainLayout = new QVBoxLayout(dialog);
    mainLayout->addWidget(mainWidget);

    QVBoxLayout* contentLayout = new QVBoxLayout(mainWidget);
    contentLayout->setContentsMargins(16, 16, 16, 16);
    contentLayout->setSpacing(20);

    QLabel* iconLabel = new QLabel();
    iconLabel->setPixmap(QIcon(":/resources/error.png").pixmap(64, 64));
    iconLabel->setAlignment(Qt::AlignCenter);

    QLabel* messageLabel = new QLabel("Initialization Error!\nPlease close the existing instance and restart the app.");
    messageLabel->setAlignment(Qt::AlignCenter);
    messageLabel->setWordWrap(true);
    messageLabel->setStyleSheet("color: black; font-size: 14px; font-family: 'Outfit';");
    messageLabel->setFont(font);

    QPushButton* closeButton = new QPushButton("Close Application");
    closeButton->setMinimumHeight(40);
    closeButton->setStyleSheet(
        "QPushButton {"
        "   background-color: rgb(220, 220, 220);"
        "   color: black;"
        "   border-radius: 6px;"
        "   padding: 8px;"
        "   font-family: 'Outfit';"
        "   font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgb(200, 200, 200);"
        "}"
    );
    closeButton->setFont(font);

    contentLayout->addWidget(iconLabel);
    contentLayout->addWidget(messageLabel);
    contentLayout->addWidget(closeButton);

    connect(closeButton, &QPushButton::clicked, this, []() {
        calls::forceShutdown();
        QApplication::quit();
        });

    QObject::connect(dialog, &QDialog::finished, overlay, &QWidget::deleteLater);

    QScreen* targetScreen = this->screen();
    if (!targetScreen) {
        targetScreen = QGuiApplication::primaryScreen();
    }

    QRect screenGeometry = targetScreen->availableGeometry();
    dialog->adjustSize();
    QSize dialogSize = dialog->size();

    int x = screenGeometry.center().x() - dialogSize.width() / 2;
    int y = screenGeometry.center().y() - dialogSize.height() / 2;
    dialog->move(x, y);

    dialog->exec();
}