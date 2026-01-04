#include "authorizationManager.h"
#include "widgets/authorizationWidget.h"
#include "widgets/mainMenuWidget.h"
#include "managers/navigationController.h"
#include "managers/configManager.h"
#include "managers/dialogsController.h"
#include "client.h"
#include "utilities/logger.h"
#include "../state.h"

AuthorizationManager::AuthorizationManager(callifornia::Client* client, NavigationController* navigationController, ConfigManager* configManager, DialogsController* dialogsController, QObject* parent)
    : QObject(parent)
    , m_client(client)
    , m_navigationController(navigationController)
    , m_configManager(configManager)
    , m_dialogsController(dialogsController)
{
}

void AuthorizationManager::setAuthorizationWidget(AuthorizationWidget* authWidget)
{
    m_authorizationWidget = authWidget;
}

void AuthorizationManager::setMainMenuWidget(MainMenuWidget* mainMenuWidget)
{
    m_mainMenuWidget = mainMenuWidget;
}

void AuthorizationManager::onAuthorizationButtonClicked(const QString& friendNickname)
{
    if (!m_authorizationWidget) return;

    if (!m_client) return;
    bool requestSent = m_client->authorize(friendNickname.toStdString());
    if (requestSent) {
        m_authorizationWidget->startBlurAnimation();
    }
    else {
        m_authorizationWidget->resetBlur();
        QString errorMessage = "Failed to send authorization request. Please try again.";
        m_authorizationWidget->setErrorMessage(errorMessage);
    }
}

void AuthorizationManager::onAuthorizationResult(callifornia::ErrorCode ec)
{
    if (!m_authorizationWidget) return;

    QString errorMessage;

    if (ec == callifornia::ErrorCode::network_error) {
        // Handle network error if needed
        errorMessage = "Network error (please try again)";
        LOG_ERROR("Authorization failed: network error");
    }
    else if (ec == callifornia::ErrorCode::taken_nickname) {
        errorMessage = "Taken nickname";
        LOG_WARN("Authorization failed: nickname already taken");
    }
    else {
        // Success case - no error code means success
        LOG_INFO("User authorization successful");
        return;
    }

    m_authorizationWidget->setErrorMessage(errorMessage);
}

void AuthorizationManager::onBlurAnimationFinished()
{
    if (!m_authorizationWidget) return;

    if (m_client && m_client->isAuthorized()) {
        m_authorizationWidget->resetBlur();
        m_authorizationWidget->clearErrorMessage();

        if (m_navigationController) {
            m_navigationController->switchToMainMenuWidget();
        }

        if (m_mainMenuWidget) {
            m_mainMenuWidget->setState(callifornia::State::FREE);

            std::string nickname = m_client->getMyNickname();
            if (!nickname.empty()) {
                m_mainMenuWidget->setNickname(QString::fromStdString(nickname));
            }

            m_mainMenuWidget->setFocusToLineEdit();
        }

        // Show first launch dialog if this is the first launch
        if (m_configManager && m_configManager->isFirstLaunch() && m_dialogsController) {
            m_dialogsController->showFirstLaunchDialog();
            m_configManager->setFirstLaunch(false);
        }
    }
    else {
        m_authorizationWidget->resetBlur();
    }
}
