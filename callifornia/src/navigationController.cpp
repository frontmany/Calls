#include "navigationController.h"
#include "authorizationWidget.h"
#include "mainMenuWidget.h"
#include "callWidget.h"
#include "calls.h"

NavigationController::NavigationController(QStackedLayout* stackedLayout, QObject* parent)
    : QObject(parent)
    , m_stackedLayout(stackedLayout)
{
}

void NavigationController::setWidgets(AuthorizationWidget* authWidget, MainMenuWidget* mainMenuWidget, CallWidget* callWidget)
{
    m_authorizationWidget = authWidget;
    m_mainMenuWidget = mainMenuWidget;
    m_callWidget = callWidget;
}

void NavigationController::switchToAuthorizationWidget()
{
    if (!m_stackedLayout || !m_authorizationWidget) return;

    m_stackedLayout->setCurrentWidget(m_authorizationWidget);
    emit windowTitleChanged("Authorization - Callifornia");
}

void NavigationController::switchToMainMenuWidget()
{
    if (!m_stackedLayout || !m_mainMenuWidget || !m_callWidget) return;

    m_stackedLayout->setCurrentWidget(m_mainMenuWidget);
    m_mainMenuWidget->setInputVolume(calls::getInputVolume());
    m_mainMenuWidget->setOutputVolume(calls::getOutputVolume());
    m_mainMenuWidget->setMicrophoneMuted(calls::isMicrophoneMuted());
    m_mainMenuWidget->setSpeakerMuted(calls::isSpeakerMuted());

    m_callWidget->hideEnterFullscreenButton();
    m_callWidget->clearIncomingCalls();
    m_callWidget->setScreenShareButtonActive(false);
    m_callWidget->hideMainScreen();
    m_callWidget->hideAdditionalScreens();

    emit windowTitleChanged("Callifornia");

    std::string nickname = calls::getNickname();
    if (!nickname.empty()) {
        m_mainMenuWidget->setNickname(QString::fromStdString(nickname));
    }
}

void NavigationController::switchToCallWidget(const QString& friendNickname)
{
    if (!m_stackedLayout || !m_callWidget) return;

    m_stackedLayout->setCurrentWidget(m_callWidget);

    // Clean up previous call state
    if (m_callWidget->isFullScreen()) {
        m_callWidget->exitFullscreen();
    }
    m_callWidget->hideEnterFullscreenButton();
    m_callWidget->hideMainScreen();
    m_callWidget->hideAdditionalScreens();
    m_callWidget->setScreenShareButtonActive(false);
    // Note: Camera state is preserved and will be set by the camera manager if needed

    m_callWidget->setInputVolume(calls::getInputVolume());
    m_callWidget->setOutputVolume(calls::getOutputVolume());
    m_callWidget->setMicrophoneMuted(calls::isMicrophoneMuted());
    m_callWidget->setSpeakerMuted(calls::isSpeakerMuted());

    emit windowTitleChanged("Call In Progress - Callifornia");
    m_callWidget->setCallInfo(friendNickname);

    emit callWidgetActivated(friendNickname);
}

void NavigationController::onCallWidgetEnterFullscreenRequested()
{
    emit windowFullscreenRequested();
}

void NavigationController::onCallWidgetExitFullscreenRequested()
{
    emit windowMaximizedRequested();
}
