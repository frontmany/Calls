#include "navigationController.h"
#include "widgets/authorizationWidget.h"
#include "widgets/mainMenuWidget.h"
#include "widgets/callWidget.h"
#include "client.h"

NavigationController::NavigationController(callifornia::Client* client, QStackedLayout* stackedLayout, QObject* parent)
    : QObject(parent)
    , m_client(client)
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
    if (m_client) {
        m_mainMenuWidget->setInputVolume(m_client->getInputVolume());
        m_mainMenuWidget->setOutputVolume(m_client->getOutputVolume());
        m_mainMenuWidget->setMicrophoneMuted(m_client->isMicrophoneMuted());
        m_mainMenuWidget->setSpeakerMuted(m_client->isSpeakerMuted());
    }

    m_callWidget->hideEnterFullscreenButton();
    m_callWidget->clearIncomingCalls();
    m_callWidget->setScreenShareButtonActive(false);
    m_callWidget->hideMainScreen();
    m_callWidget->hideAdditionalScreens();

    emit windowTitleChanged("Callifornia");

    std::string nickname = m_client ? m_client->getMyNickname() : "";
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

    if (m_client) {
        m_callWidget->setInputVolume(m_client->getInputVolume());
        m_callWidget->setOutputVolume(m_client->getOutputVolume());
        m_callWidget->setMicrophoneMuted(m_client->isMicrophoneMuted());
        m_callWidget->setSpeakerMuted(m_client->isSpeakerMuted());
    }

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
