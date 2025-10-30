#include "mainWindow.h"
#include <QStackedLayout>
#include <QHBoxLayout>
#include <QFontDatabase>
#include <QApplication>
#include <QSoundEffect>
#include <QFileInfo>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include "AuthorizationWidget.h"
#include "MainMenuWidget.h"
#include "OverlayWidget.h"
#include "CallWidget.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
    m_updater(
        [this](updater::CheckResult checkResult) {
            QMetaObject::invokeMethod(this, "onUpdaterCheckResult",
                Qt::QueuedConnection, Q_ARG(updater::CheckResult, checkResult));
        },
        [this](double progress) {
            QMetaObject::invokeMethod(this, "onLoadingProgress",
                Qt::QueuedConnection, Q_ARG(double, progress));
        },
        [this]() {
            QMetaObject::invokeMethod(this, "onUpdateLoaded", Qt::QueuedConnection);
        },
        [this]() {
            QMetaObject::invokeMethod(this, "onUpdaterError", Qt::QueuedConnection);
        })
{
    setWindowIcon(QIcon(":/resources/callifornia.ico"));
}

MainWindow::~MainWindow() {
    if (m_ringtonePlayer) {
        m_ringtonePlayer->stop();
    }
}

void MainWindow::initializeCallifornia(const std::string& host, const std::string& port) {
    m_updater.connect(host, port);
    m_updater.checkUpdates(parseVersionFromConfig());


    m_ringtonePlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_audioOutput->setVolume(0.4f);
    m_ringtonePlayer->setAudioOutput(m_audioOutput);

    connect(m_ringtonePlayer, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state) {
        if (state == QMediaPlayer::StoppedState) {
            if (calls::getIncomingCallsCount() != 0) {
                m_ringtonePlayer->play();
            }
        }
    });

    loadFonts();
    setupUI();

    std::unique_ptr<CallsClientHandler> callsClientHandler = std::make_unique<CallsClientHandler>(this);
    bool initialized = calls::init(host, port, std::move(callsClientHandler));
    
    if (!initialized) 
        qDebug("calls client initialization error");
    else 
        calls::run();

    /*
    QTimer::singleShot(0, [this]() {
        showUpdatingDialog();
    });
    */

    showMaximized();
}

void MainWindow::playRingtone(const QUrl& ringtoneUrl) {
    if (!m_ringtonePlayer) return;

    if (m_ringtonePlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_ringtonePlayer->stop();
    }

    m_ringtonePlayer->setSource(ringtoneUrl);

    if (m_ringtonePlayer->playbackState() != QMediaPlayer::PlayingState) {
        m_ringtonePlayer->setLoops(QMediaPlayer::Infinite);
        m_ringtonePlayer->play();
    }
}

void MainWindow::stopRingtone() {
    if (!m_ringtonePlayer) return;

    if (m_ringtonePlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_ringtonePlayer->stop();
    }
}

void MainWindow::playIncomingCallRingtone() {
    playRingtone(QUrl("qrc:/resources/incomingCallRingtone.mp3"));
}

void MainWindow::stopIncomingCallRingtone() {
    stopRingtone();
}

void MainWindow::playCallingRingtone() {
    playRingtone(QUrl("qrc:/resources/calling.mp3"));
}

void MainWindow::stopCallingRingtone() {
    stopRingtone();
}

void MainWindow::playSoundEffect(const QString& soundPath) {
    QSoundEffect* effect = new QSoundEffect(this);
    effect->setSource(QUrl::fromLocalFile(soundPath));
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

std::string MainWindow::parseVersionFromConfig() {
    const QString filename = "config.json";

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return "versionLost";
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull()) return "";

    QJsonObject json = doc.object();
    std::string version = json[calls::VERSION].toString().toStdString();

    return version;
}

updater::OperationSystemType MainWindow::resolveOperationSystemType() {
#if defined(Q_OS_WINDOWS)
    return updater::OperationSystemType::WINDOWS;
#elif defined(Q_OS_LINUX)
    return OperationSystemType::LINUX;
#elif defined(Q_OS_MACOS)
    return OperationSystemType::MAC;
#else
#if defined(_WIN32)
    return OperationSystemType::WINDOWS;
#elif defined(__linux__)
    return OperationSystemType::LINUX;
#elif defined(__APPLE__)
    return OperationSystemType::MAC;
#else
    qWarning() << "Unknown operating system";
    return updater::OperationSystemType::WINDOWS;
#endif
#endif
}

void MainWindow::onUpdaterCheckResult(updater::CheckResult checkResult) {
    m_authorizationWidget->hideUpdatesCheckingNotification();

    if (checkResult == updater::CheckResult::POSSIBLE_UPDATE) {
        if (!calls::isNetworkError()) {
            m_authorizationWidget->setAuthorizationDisabled(false);
        }

        if (calls::isNetworkError()) {
            m_authorizationWidget->hideNetworkErrorNotification();
        }

        m_authorizationWidget->showUpdateAvailableNotification();
        m_mainMenuWidget->showUpdateAvailableNotification();
    }
    else if (checkResult == updater::CheckResult::REQUIRED_UPDATE) {
        m_updater.startUpdate(resolveOperationSystemType());
        showUpdatingDialog();
    }
    else if (checkResult == updater::CheckResult::UPDATE_NOT_NEEDED) {
        m_updater.disconnect();

        if (!calls::isNetworkError()) {
            m_authorizationWidget->setAuthorizationDisabled(false);
        }
    }
    else {
        qWarning() << "error: unknown CheckResult type";
    }
}


void MainWindow::onUpdateButtonClicked() {
    if (calls::isCalling()) {
        calls::stopCalling();
    }

    auto callers = calls::getCallers();
    for (const auto& nickname : callers) {
        calls::declineCall(nickname);
    }

    stopRingtone();
    m_mainMenuWidget->removeCallingPanel();
    m_mainMenuWidget->clearIncomingCalls();

    m_updater.startUpdate(resolveOperationSystemType());
    showUpdatingDialog();
}

void MainWindow::onUpdateLoaded() {
    calls::logout();

    QDialog* updatingDialog = nullptr;
    QList<QDialog*> dialogs = findChildren<QDialog*>();
    for (QDialog* dialog : dialogs) {
        if (dialog->windowTitle() == "Updating") {
            updatingDialog = dialog;
            break;
        }
    }

    if (updatingDialog) {
        QLabel* gifLabel = updatingDialog->findChild<QLabel*>();
        QLabel* progressLabel = updatingDialog->findChild<QLabel*>(QString(), Qt::FindDirectChildrenOnly);
        QLabel* updatingLabel = nullptr;

        QList<QLabel*> labels = updatingDialog->findChildren<QLabel*>();
        for (QLabel* label : labels) {
            if (label->text().contains("Updating...")) {
                updatingLabel = label;
            }
        }

        if (gifLabel && gifLabel->movie()) {
            gifLabel->movie()->stop();
        }

        if (updatingLabel) {
            updatingLabel->setText("Restarting...");
        }

        if (progressLabel) {
            progressLabel->setVisible(false);
        }

        qApp->processEvents();

        QTimer::singleShot(1500, this, [this]() {
            launchUpdateApplier();
        });
    }
}

void MainWindow::launchUpdateApplier() {
    qint64 currentPid = QCoreApplication::applicationPid();
    QString appPath = QCoreApplication::applicationFilePath();

    QString updateApplierName;
#ifdef Q_OS_WIN
    updateApplierName = "update_applier.exe";
#else
    updateApplierName = "update_applier";
#endif

    QFileInfo updateSupplierFile(updateApplierName);
    if (!updateSupplierFile.exists()) {
        qWarning() << "Update applier not found:" << updateApplierName;
        return;
    }

    QStringList arguments;
    arguments << QString::number(currentPid) << appPath;

    QProcess::startDetached(updateApplierName, arguments);
}

void MainWindow::onLoadingProgress(double progress) {
    if (m_updatingProgressLabel) {
        QString progressText = QString("%1%").arg(progress, 0, 'f', 2);
        m_updatingProgressLabel->setText(progressText);
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
    connect(m_authorizationWidget, &AuthorizationWidget::updateButtonClicked, this, &MainWindow::onUpdateButtonClicked);
    connect(m_authorizationWidget, &AuthorizationWidget::authorizationButtonClicked, this, &MainWindow::onAuthorizationButtonClicked);
    connect(m_authorizationWidget, &AuthorizationWidget::blurAnimationFinished, this, &MainWindow::onBlurAnimationFinished);

    m_stackedLayout->addWidget(m_authorizationWidget);

    m_mainMenuWidget = new MainMenuWidget(this);
    connect(m_mainMenuWidget, &MainMenuWidget::updateButtonClicked, this, &MainWindow::onUpdateButtonClicked);
    connect(m_mainMenuWidget, &MainMenuWidget::acceptCallButtonClicked, this, &MainWindow::onAcceptCallButtonClicked);
    connect(m_mainMenuWidget, &MainMenuWidget::declineCallButtonClicked, this, &MainWindow::onDeclineCallButtonClicked);
    connect(m_mainMenuWidget, &MainMenuWidget::startCallingButtonClicked, this, &MainWindow::onStartCallingButtonClicked);
    connect(m_mainMenuWidget, &MainMenuWidget::stopCallingButtonClicked, this, &MainWindow::onStopCallingButtonClicked);
    connect(m_mainMenuWidget, &MainMenuWidget::refreshAudioDevicesButtonClicked, this, &MainWindow::onRefreshAudioDevicesButtonClicked);
    connect(m_mainMenuWidget, &MainMenuWidget::inputVolumeChanged, this, &MainWindow::onInputVolumeChanged);
    connect(m_mainMenuWidget, &MainMenuWidget::outputVolumeChanged, this, &MainWindow::onOutputVolumeChanged);
    connect(m_mainMenuWidget, &MainMenuWidget::muteMicrophoneClicked, this, &MainWindow::onMuteMicrophoneButtonClicked);
    connect(m_mainMenuWidget, &MainMenuWidget::muteSpeakerClicked, this, &MainWindow::onMuteSpeakerButtonClicked);
    m_stackedLayout->addWidget(m_mainMenuWidget); 

    m_callWidget = new CallWidget(this);
    connect(m_callWidget, &CallWidget::hangupClicked, this, &MainWindow::onEndCallButtonClicked);
    connect(m_callWidget, &CallWidget::inputVolumeChanged, this, &MainWindow::onInputVolumeChanged);
    connect(m_callWidget, &CallWidget::outputVolumeChanged, this, &MainWindow::onOutputVolumeChanged);
    connect(m_callWidget, &CallWidget::muteSpeakerClicked, this, &MainWindow::onMuteSpeakerButtonClicked);
    connect(m_callWidget, &CallWidget::muteMicrophoneClicked, this, &MainWindow::onMuteMicrophoneButtonClicked);
    connect(m_callWidget, &CallWidget::acceptCallButtonClicked, this, &MainWindow::onAcceptCallButtonClicked);
    connect(m_callWidget, &CallWidget::declineCallButtonClicked, this, &MainWindow::onDeclineCallButtonClicked);
    m_stackedLayout->addWidget(m_callWidget);

    m_authorizationWidget->showUpdatesCheckingNotification();
    m_authorizationWidget->setAuthorizationDisabled(true);

    switchToAuthorizationWidget();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    calls::stop();
    m_updater.disconnect();
    event->accept();
}

void MainWindow::switchToAuthorizationWidget() {
    m_stackedLayout->setCurrentWidget(m_authorizationWidget);
    setWindowTitle("Authorization - Callifornia");
}

void MainWindow::switchToMainMenuWidget() {
    m_stackedLayout->setCurrentWidget(m_mainMenuWidget);
    m_mainMenuWidget->setInputVolume(calls::getInputVolume());
    m_mainMenuWidget->setOutputVolume(calls::getOutputVolume());
    m_mainMenuWidget->setMicrophoneMuted(calls::isMicrophoneMuted());
    m_mainMenuWidget->setSpeakerMuted(calls::isSpeakerMuted());

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
    m_callWidget->setMicrophoneMuted(calls::isMicrophoneMuted());
    m_callWidget->setSpeakerMuted(calls::isSpeakerMuted());

    setWindowTitle("Call In Progress - Callifornia");
    m_callWidget->setCallInfo(friendNickname);
}

void MainWindow::onBlurAnimationFinished() {
    if (calls::isAuthorized()) {
        m_authorizationWidget->resetBlur();
        m_authorizationWidget->clearErrorMessage();

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
        return;
    }

    m_callWidget->clearIncomingCalls();
    m_mainMenuWidget->setState(calls::State::FREE);
    switchToMainMenuWidget();
    playSoundEffect(":/resources/endCall.wav");
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

void MainWindow::onMuteMicrophoneButtonClicked(bool mute) {
    calls::muteMicrophone(mute);
}

void MainWindow::onMuteSpeakerButtonClicked(bool mute) {
    calls::muteSpeaker(mute);
}

void MainWindow::onAcceptCallButtonClicked(const QString& friendNickname) {
    bool requestSent = calls::acceptCall(friendNickname.toStdString());
    if (!requestSent) {
        handleAcceptCallErrorNotificationAppearance();
        return;
    }

    playSoundEffect(":/resources/callJoined.wav");
}

void MainWindow::onDeclineCallButtonClicked(const QString& friendNickname) {
    bool requestSent = calls::declineCall(friendNickname.toStdString());
    if (!requestSent) {
        handleDeclineCallErrorNotificationAppearance();
    }
    else {
        if (calls::getIncomingCallsCount() == 0) {
            stopRingtone();
        }

        if (m_stackedLayout->currentWidget() == m_callWidget)
            m_callWidget->removeIncomingCall(friendNickname);

        m_mainMenuWidget->removeIncomingCall(friendNickname);
        playSoundEffect(":/resources/callingEnded.wav");
    }
}

void MainWindow::onStartCallingButtonClicked(const QString& friendNickname) {
    if (!friendNickname.isEmpty() && friendNickname.toStdString() != calls::getNickname()) {
        bool requestSent = calls::startCalling(friendNickname.toStdString());
        if (!requestSent) {
            handleStartCallingErrorNotificationAppearance();
            return;
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
    else {
        m_mainMenuWidget->removeCallingPanel();
        m_mainMenuWidget->setState(calls::State::FREE);
        stopRingtone();
        playSoundEffect(":/resources/callingEnded.wav");
    }
}

void MainWindow::handleAcceptCallErrorNotificationAppearance() {
    QString errorText = "Failed to accept call";

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

void MainWindow::onAuthorizationResult(calls::ErrorCode ec) {
    QString errorMessage;

    if (ec == calls::ErrorCode::OK) {
        return;
    }
    else if (ec == calls::ErrorCode::TAKEN_NICKNAME) {
        errorMessage = "Taken nickname";
    }
    else if (ec == calls::ErrorCode::TIMEOUT) {
        errorMessage = "Timeout (please try again)";
    }

    m_authorizationWidget->setErrorMessage(errorMessage);
}

void MainWindow::onStartCallingResult(calls::ErrorCode ec) {
    QString errorMessage;

    if (ec == calls::ErrorCode::OK) {
        m_mainMenuWidget->showCallingPanel(QString::fromStdString(calls::getNicknameWhomCalling()));
        m_mainMenuWidget->setState(calls::State::CALLING);
        playCallingRingtone();
        return;
    }
    else if (ec == calls::ErrorCode::UNEXISTING_USER) {
        errorMessage = "User not found";
    }
    else if (ec == calls::ErrorCode::TIMEOUT) {
        errorMessage = "Timeout (please try again)";
    }

    m_mainMenuWidget->setErrorMessage(errorMessage);
}

void MainWindow::onAcceptCallResult(calls::ErrorCode ec, const QString& nickname) {
    if (ec == calls::ErrorCode::OK) {
        m_mainMenuWidget->removeCallingPanel();

        m_mainMenuWidget->clearIncomingCalls();

        if (m_stackedLayout->currentWidget() == m_callWidget)
            m_callWidget->clearIncomingCalls();

        switchToCallWidget(nickname);
        stopRingtone();
    }
    else {
        m_mainMenuWidget->removeIncomingCall(nickname);

        if (m_stackedLayout->currentWidget() == m_callWidget)
            m_callWidget->removeIncomingCall(nickname);

        handleAcceptCallErrorNotificationAppearance();
    }
}

void MainWindow::onMaximumCallingTimeReached() {
    m_mainMenuWidget->removeCallingPanel();
    m_mainMenuWidget->setState(calls::State::FREE);
    stopRingtone();
}

void MainWindow::onCallingAccepted() {
    m_mainMenuWidget->clearIncomingCalls();

    stopRingtone();
    m_mainMenuWidget->removeCallingPanel();
    m_mainMenuWidget->setState(calls::State::BUSY);
    switchToCallWidget(QString::fromStdString(calls::getNicknameInCallWith()));
    playSoundEffect(":/resources/callJoined.wav");
}

void MainWindow::onCallingDeclined() {
    m_mainMenuWidget->removeCallingPanel();
    m_mainMenuWidget->setState(calls::State::FREE);
    stopRingtone();
    playSoundEffect(":/resources/callingEnded.wav");
}

void MainWindow::onRemoteUserEndedCall() {
    m_callWidget->clearIncomingCalls();
    switchToMainMenuWidget();

    m_mainMenuWidget->setState(calls::State::FREE);
    playSoundEffect(":/resources/endCall.wav");
}

void MainWindow::onIncomingCall(const QString& friendNickName) {
    playIncomingCallRingtone();

    m_mainMenuWidget->addIncomingCall(friendNickName);

    if (m_stackedLayout->currentWidget() == m_callWidget) 
        m_callWidget->addIncomingCall(friendNickName);
}

void MainWindow::onIncomingCallExpired(const QString& friendNickName) {
    m_mainMenuWidget->removeIncomingCall(friendNickName);

    if (m_stackedLayout->currentWidget() == m_callWidget)
        m_callWidget->removeIncomingCall(friendNickName);

    if (calls::getIncomingCallsCount() == 0)
        stopRingtone();

    playSoundEffect(":/resources/callingEnded.wav");
}

void MainWindow::onNetworkError() {
    stopRingtone();

    m_mainMenuWidget->removeCallingPanel();
    m_mainMenuWidget->clearIncomingCalls();

    if (m_stackedLayout->currentWidget() != m_authorizationWidget) {
        switchToAuthorizationWidget();
    }

    m_authorizationWidget->resetBlur();
    m_authorizationWidget->setAuthorizationDisabled(true);

    if (m_updater.getState() != updater::ClientUpdater::State::AWAITING_START_UPDATE)
        m_authorizationWidget->showNetworkErrorNotification();
}

void MainWindow::onUpdaterError() {
    m_authorizationWidget->hideUpdateAvailableNotification();
    m_mainMenuWidget->hideUpdateAvailableNotification();

    hideUpdatingDialog();
    if (!calls::isAuthorized() || m_updater.getState() == updater::ClientUpdater::State::AWAITING_SERVER_RESPONSE) {
        showConnectionErrorDialog();
    }

    m_updater.disconnect();
}

void MainWindow::onConnectionRestored() {
    m_authorizationWidget->resetBlur();
    m_authorizationWidget->setAuthorizationDisabled(false);
    m_authorizationWidget->showConnectionRestoredNotification(1500);

    if (m_updater.getState() == updater::ClientUpdater::State::DISCONNECTED) {
        m_updater.connect("192.168.1.44", "8081");
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        m_updater.checkUpdates(parseVersionFromConfig());
    }
}

void MainWindow::hideUpdatingDialog()
{
    QList<OverlayWidget*> overlays = findChildren<OverlayWidget*>();
    for (OverlayWidget* overlay : overlays) {
        overlay->close();
        overlay->deleteLater();
    }

    QList<QDialog*> dialogs = findChildren<QDialog*>();
    for (QDialog* dialog : dialogs) {
        if (dialog->windowTitle() == "Updating") {
            dialog->close();
            dialog->deleteLater();
        }
    }

    m_updatingProgressLabel = nullptr;
}

void MainWindow::showUpdatingDialog()
{
    QFont font("Outfit", 14, QFont::Normal);

    OverlayWidget* overlay = new OverlayWidget(this);
    overlay->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    overlay->setAttribute(Qt::WA_TranslucentBackground);
    overlay->showMaximized();

    QDialog* dialog = new QDialog(overlay);
    dialog->setWindowTitle("Updating");
    dialog->setMinimumWidth(300);
    dialog->setMinimumHeight(250);
    dialog->setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    dialog->setAttribute(Qt::WA_TranslucentBackground);

    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
    shadowEffect->setBlurRadius(30);
    shadowEffect->setXOffset(0);
    shadowEffect->setYOffset(0);
    shadowEffect->setColor(QColor(0, 0, 0, 150));

    QWidget* mainWidget = new QWidget(dialog);
    mainWidget->setGraphicsEffect(shadowEffect);
    mainWidget->setObjectName("mainWidget");

    QString mainWidgetStyle =
        "QWidget#mainWidget {"
        "   background-color: rgb(226, 243, 231);"
        "   border-radius: 16px;"
        "   border: 1px solid rgb(210, 210, 210);"
        "}";
    mainWidget->setStyleSheet(mainWidgetStyle);

    QVBoxLayout* mainLayout = new QVBoxLayout(dialog);
    mainLayout->addWidget(mainWidget);

    QVBoxLayout* contentLayout = new QVBoxLayout(mainWidget);
    contentLayout->setContentsMargins(24, 24, 24, 24);
    contentLayout->setSpacing(16);

    QLabel* gifLabel = new QLabel();
    gifLabel->setAlignment(Qt::AlignCenter);
    gifLabel->setMinimumHeight(120);

    QMovie* movie = new QMovie(":/resources/updating.gif");
    if (movie->isValid()) {
        gifLabel->setMovie(movie);
        movie->start();
    }

    // Создаем метку для отображения прогресса
    QLabel* progressLabel = new QLabel("0.00%");
    progressLabel->setAlignment(Qt::AlignCenter);
    progressLabel->setStyleSheet(
        "color: rgb(80, 80, 80);"
        "font-size: 14px;"
        "font-family: 'Outfit';"
        "font-weight: bold;"
    );

    QLabel* updatingLabel = new QLabel("Updating...");
    updatingLabel->setAlignment(Qt::AlignCenter);
    updatingLabel->setStyleSheet(
        "color: rgb(60, 60, 60);"
        "font-size: 16px;"
        "font-family: 'Outfit';"
        "font-weight: bold;"
    );
    updatingLabel->setFont(font);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setAlignment(Qt::AlignCenter);

    QPushButton* exitButton = new QPushButton("Exit");
    exitButton->setFixedWidth(120);
    exitButton->setMinimumHeight(36);
    exitButton->setStyleSheet(
        "QPushButton {"
        "   background-color: transparent;"
        "   color: rgb(120, 120, 120);"
        "   border-radius: 6px;"
        "   padding: 6px 12px;"
        "   font-family: 'Outfit';"
        "   font-size: 13px;"
        "   border: none;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgba(0, 0, 0, 8);"
        "   color: rgb(100, 100, 100);"
        "}"
        "QPushButton:pressed {"
        "   background-color: rgba(0, 0, 0, 15);"
        "}"
    );
    exitButton->setFont(font);

    buttonLayout->addWidget(exitButton);

    contentLayout->addWidget(gifLabel);
    contentLayout->addWidget(progressLabel); 
    contentLayout->addWidget(updatingLabel);
    contentLayout->addLayout(buttonLayout);

    m_updatingProgressLabel = progressLabel;

    connect(exitButton, &QPushButton::clicked, this, [this, dialog, overlay]() {
        this->close();
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

void MainWindow::hideUpdatingErrorDialog()
{
    QList<OverlayWidget*> overlays = findChildren<OverlayWidget*>();
    for (OverlayWidget* overlay : overlays) {
        overlay->close();
        overlay->deleteLater();
    }

    QList<QDialog*> dialogs = findChildren<QDialog*>();
    for (QDialog* dialog : dialogs) {
        if (dialog->windowTitle() == "Update Error") {
            dialog->close();
            dialog->deleteLater();
        }
    }
}

void MainWindow::showConnectionErrorDialog()
{
    QFont font("Outfit", 14, QFont::Normal);

    OverlayWidget* overlay = new OverlayWidget(this);
    overlay->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    overlay->setAttribute(Qt::WA_TranslucentBackground);
    overlay->showMaximized();

    QDialog* dialog = new QDialog(overlay);
    dialog->setWindowTitle("Update Error");
    dialog->setMinimumWidth(520);
    dialog->setMinimumHeight(360);
    dialog->setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    dialog->setAttribute(Qt::WA_TranslucentBackground);

    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
    shadowEffect->setBlurRadius(30);
    shadowEffect->setXOffset(0);
    shadowEffect->setYOffset(0);
    shadowEffect->setColor(QColor(0, 0, 0, 150));

    QWidget* mainWidget = new QWidget(dialog);
    mainWidget->setGraphicsEffect(shadowEffect);
    mainWidget->setObjectName("mainWidget");

    QString mainWidgetStyle =
        "QWidget#mainWidget {"
        "   background-color: rgb(255, 240, 240);"
        "   border-radius: 16px;"
        "   border: 1px solid rgb(210, 210, 210);"
        "}";
    mainWidget->setStyleSheet(mainWidgetStyle);

    QVBoxLayout* mainLayout = new QVBoxLayout(dialog);
    mainLayout->addWidget(mainWidget);

    QVBoxLayout* contentLayout = new QVBoxLayout(mainWidget);
    contentLayout->setContentsMargins(32, 32, 32, 32);
    contentLayout->setSpacing(20);

    QLabel* imageLabel = new QLabel();
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setMinimumHeight(140);

    QPixmap errorPixmap(":/resources/error.png");
    imageLabel->setPixmap(errorPixmap.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation));  // Увеличил размер изображения


    QLabel* errorLabel = new QLabel("Connection Error");
    errorLabel->setAlignment(Qt::AlignCenter);
    errorLabel->setStyleSheet(
        "color: rgb(180, 60, 60);"
        "font-size: 18px;"
        "font-family: 'Outfit';"
        "font-weight: bold;"
    );
    errorLabel->setFont(font);

    QLabel* messageLabel = new QLabel("An error occurred. Please restart and try again.");
    messageLabel->setAlignment(Qt::AlignCenter);
    messageLabel->setWordWrap(true);
    messageLabel->setStyleSheet(
        "color: rgb(100, 100, 100);"
        "font-size: 14px;"
        "font-family: 'Outfit';"
    );

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setAlignment(Qt::AlignCenter);

    QPushButton* closeButton = new QPushButton("Close Calllifornia");
    closeButton->setFixedWidth(140);
    closeButton->setMinimumHeight(42);
    closeButton->setStyleSheet(
        "QPushButton {"
        "   background-color: rgba(180, 60, 60, 30);"
        "   color: rgb(180, 60, 60);"
        "   border-radius: 8px;"
        "   padding: 8px 16px;"
        "   font-family: 'Outfit';"
        "   font-size: 14px;"
        "   border: none;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgba(180, 60, 60, 40);"
        "   color: rgb(160, 50, 50);"
        "}"
        "QPushButton:pressed {"
        "   background-color: rgba(180, 60, 60, 60);"
        "}"
    );
    closeButton->setFont(font);

    buttonLayout->addWidget(closeButton);

    contentLayout->addWidget(imageLabel);
    contentLayout->addWidget(errorLabel);
    contentLayout->addWidget(messageLabel);
    contentLayout->addLayout(buttonLayout);

    connect(closeButton, &QPushButton::clicked, this, [this, dialog, overlay]() {
        this->close();
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