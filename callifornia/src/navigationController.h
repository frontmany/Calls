#pragma once

#include <QObject>
#include <QStackedLayout>
#include <QString>

class AuthorizationWidget;
class MainMenuWidget;
class CallWidget;

class NavigationController : public QObject {
    Q_OBJECT

public:
    explicit NavigationController(QStackedLayout* stackedLayout, QObject* parent = nullptr);
    
    void setWidgets(AuthorizationWidget* authWidget, MainMenuWidget* mainMenuWidget, CallWidget* callWidget);

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
    QStackedLayout* m_stackedLayout = nullptr;
    AuthorizationWidget* m_authorizationWidget = nullptr;
    MainMenuWidget* m_mainMenuWidget = nullptr;
    CallWidget* m_callWidget = nullptr;
};
