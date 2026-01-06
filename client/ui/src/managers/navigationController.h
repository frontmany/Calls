#pragma once

#include <QObject>
#include <QStackedLayout>
#include <QString>
#include <memory>

#include "client.h"

class AuthorizationWidget;
class MainMenuWidget;
class CallWidget;

class NavigationController : public QObject {
    Q_OBJECT

public:
    explicit NavigationController(std::shared_ptr<callifornia::Client> client, QObject* parent = nullptr);
    
    void setWidgets(QStackedLayout* stackedLayout, AuthorizationWidget* authWidget, MainMenuWidget* mainMenuWidget, CallWidget* callWidget);

    void switchToAuthorizationWidget();
    void switchToMainMenuWidget();
    void switchToCallWidget(const QString& friendNickname);

public slots:
    void onCallWidgetEnterFullscreenRequested();
    void onCallWidgetExitFullscreenRequested();

signals:
    void windowTitleChanged(const QString& title);
    void windowFullscreenRequested();
    void windowMaximizedRequested();
    void callWidgetActivated(const QString& friendNickname);

private:
    std::shared_ptr<callifornia::Client> m_client = nullptr;
    QStackedLayout* m_stackedLayout = nullptr;
    AuthorizationWidget* m_authorizationWidget = nullptr;
    MainMenuWidget* m_mainMenuWidget = nullptr;
    CallWidget* m_callWidget = nullptr;
};
