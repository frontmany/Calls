#include "authorizationManager.h"
#include "widgets/authorizationWidget.h"
#include "widgets/mainMenuWidget.h"
#include "managers/navigationController.h"
#include "managers/configManager.h"
#include "managers/dialogsController.h"

#include "../utilities/logger.h"

AuthorizationManager::AuthorizationManager(std::shared_ptr<core::Client> client, NavigationController* navigationController, ConfigManager* configManager, DialogsController* dialogsController, QObject* parent)
    : QObject(parent)
    , m_client(client)
    , m_navigationController(navigationController)
    , m_configManager(configManager)
    , m_dialogsController(dialogsController)
{
}

void AuthorizationManager::setWidgets(AuthorizationWidget* authorizationWidget, MainMenuWidget* mainMenuWidget)
{
    m_authorizationWidget = authorizationWidget;
    m_mainMenuWidget = mainMenuWidget;
}

void AuthorizationManager::onAuthorizationButtonClicked(const QString& friendNickname) {
    if (m_client->authorize(friendNickname.toStdString())) {
        m_authorizationWidget->startBlurAnimation();
    }
    else {
        QString errorMessage = "Failed to send authorization request. Please try again.";
        m_authorizationWidget->setErrorMessage(errorMessage);
    }
}

void AuthorizationManager::onAuthorizationResult(std::error_code ec) {
    m_authorizationWidget->waitForBlurAnimation();
    m_authorizationWidget->resetBlur();

    if (ec == core::ErrorCode::network_error) {
        LOG_ERROR("Authorization failed: network error");

        QString errorMessage = "Network error (please try again)";
        m_authorizationWidget->setErrorMessage(errorMessage);
    }
    else if (ec == core::ErrorCode::taken_nickname) {
        LOG_WARN("Authorization failed: nickname already taken");

        QString errorMessage = "Taken nickname";
        m_authorizationWidget->setErrorMessage(errorMessage);
    }
    else {
        LOG_INFO("User authorization successful");

        m_authorizationWidget->clearErrorMessage();

        m_navigationController->switchToMainMenuWidget();
        
        m_mainMenuWidget->setStatusLabelOnline();
        m_mainMenuWidget->setNickname(QString::fromStdString(m_client->getMyNickname()));
        m_mainMenuWidget->setFocusToLineEdit();
        
        if (m_configManager->isFirstLaunch()) {
            m_dialogsController->showFirstLaunchDialog();
            m_configManager->setFirstLaunch(false);
        }
    }
}