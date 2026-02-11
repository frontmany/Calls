#include "authorizationManager.h"
#include "widgets/authorizationWidget.h"
#include "widgets/mainMenuWidget.h"
#include "managers/navigationController.h"
#include "managers/configManager.h"
#include "managers/dialogsController.h"
#include "managers/updateManager.h"

#include "../utilities/logger.h"
#include "constants/errorCode.h"
#include <QApplication>
#include <system_error>

AuthorizationManager::AuthorizationManager(std::shared_ptr<core::Core> client, NavigationController* navigationController, ConfigManager* configManager, DialogsController* dialogsController, UpdateManager* updateManager, QObject* parent)
    : QObject(parent)
    , m_coreClient(client)
    , m_navigationController(navigationController)
    , m_configManager(configManager)
    , m_dialogsController(dialogsController)
    , m_updateManager(updateManager)
{
}

void AuthorizationManager::setWidgets(AuthorizationWidget* authorizationWidget, MainMenuWidget* mainMenuWidget)
{
    m_authorizationWidget = authorizationWidget;
    m_mainMenuWidget = mainMenuWidget;
}

void AuthorizationManager::onAuthorizationButtonClicked(const QString& friendNickname) {
    std::error_code ec = m_coreClient->authorize(friendNickname.toStdString());
    
    if (ec) {
        if (ec == core::constant::make_error_code(core::constant::ErrorCode::connection_down)) {
            LOG_ERROR("Authorization failed: connection down");

            QString errorMessage = "Connection down. Please check your connection and try again.";
            m_authorizationWidget->setErrorMessage(errorMessage);
        }
        else if (ec == core::constant::make_error_code(core::constant::ErrorCode::already_authorized)) {
            LOG_WARN("Authorization failed: already authorized");

            QString errorMessage = "You are already authorized.";
            m_authorizationWidget->setErrorMessage(errorMessage);
        }
        else if (ec == core::constant::make_error_code(core::constant::ErrorCode::operation_in_progress)) {
            LOG_WARN("Authorization failed: operation in progress");

            QString errorMessage = "Authorization is already in progress. Please wait.";
            m_authorizationWidget->setErrorMessage(errorMessage);
        }
        else {
            LOG_ERROR("Authorization failed: unknown error");

            QString errorMessage = "Failed to send authorization request. Please try again.";
            m_authorizationWidget->setErrorMessage(errorMessage);
        }
    }
    else {
        m_authorizationWidget->startBlurAnimation();
        m_authorizationWidget->setAuthorizationDisabled(true);
    }
}

void AuthorizationManager::onAuthorizationResult(std::error_code ec) {
    m_authorizationWidget->waitForBlurAnimation();
    m_authorizationWidget->resetBlur(); 

    if (ec) {
        if (ec == core::constant::make_error_code(core::constant::ErrorCode::network_error)) {
            LOG_ERROR("Authorization failed: network error");

            QString errorMessage = "Network error: please try again";
            m_authorizationWidget->setErrorMessage(errorMessage);
        }
        else if (ec == core::constant::make_error_code(core::constant::ErrorCode::taken_nickname)) {
            LOG_WARN("Authorization failed: nickname already taken");

            QString errorMessage = "Taken nickname";
            m_authorizationWidget->setErrorMessage(errorMessage);
            m_authorizationWidget->setAuthorizationDisabled(false);
        }
        else {
            LOG_WARN("Authorization failed: unknown error");

            QString errorMessage = "Unknown error";
            m_authorizationWidget->setErrorMessage(errorMessage);
            m_authorizationWidget->setAuthorizationDisabled(false);
        }
    }
    else {
        m_authorizationWidget->setAuthorizationDisabled(false);

        m_authorizationWidget->clearErrorMessage();

        m_navigationController->switchToMainMenuWidget();
        
        m_mainMenuWidget->setStatusLabelOnline();
        m_mainMenuWidget->setNickname(QString::fromStdString(m_coreClient->getMyNickname()));
        m_mainMenuWidget->setFocusToLineEdit();
        
        if (m_configManager->isFirstLaunch()) {
            m_configManager->setFirstLaunch(false);
            m_dialogsController->showFirstLaunchDialog();
        }
    }
}

