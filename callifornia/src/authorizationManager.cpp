#include "authorizationManager.h"
#include "authorizationWidget.h"
#include "mainMenuWidget.h"
#include "navigationController.h"
#include "configManager.h"
#include "dialogsController.h"
#include "calls.h"
#include "logger.h"
#include "state.h"

AuthorizationManager::AuthorizationManager(NavigationController* navigationController, ConfigManager* configManager, DialogsController* dialogsController, QObject* parent)
    : QObject(parent)
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

void AuthorizationManager::onAuthorizationResult(calls::ErrorCode ec)
{
    if (!m_authorizationWidget) return;

    QString errorMessage;

    if (ec == calls::ErrorCode::OK) {
        LOG_INFO("User authorization successful");
        return;
    }
    else if (ec == calls::ErrorCode::TAKEN_NICKNAME) {
        errorMessage = "Taken nickname";
        LOG_WARN("Authorization failed: nickname already taken");
    }
    else if (ec == calls::ErrorCode::TIMEOUT) {
        errorMessage = "Timeout (please try again)";
        LOG_ERROR("Authorization failed: timeout");
    }

    m_authorizationWidget->setErrorMessage(errorMessage);
}

void AuthorizationManager::onBlurAnimationFinished()
{
    if (!m_authorizationWidget) return;

    if (calls::isAuthorized()) {
        m_authorizationWidget->resetBlur();
        m_authorizationWidget->clearErrorMessage();

        if (m_navigationController) {
            m_navigationController->switchToMainMenuWidget();
        }

        if (m_mainMenuWidget) {
            m_mainMenuWidget->setState(calls::State::FREE);

            std::string nickname = calls::getNickname();
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
