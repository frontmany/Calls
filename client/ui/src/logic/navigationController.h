#pragma once

#include <QObject>
#include <QStackedLayout>
#include <QString>
#include <memory>

#include "core.h"

class AuthorizationWidget;
class MainMenuWidget;
class CallWidget;
class MeetingWidget;

class NavigationController : public QObject {
    Q_OBJECT

public:
    explicit NavigationController(std::shared_ptr<core::Core> client, QObject* parent = nullptr);

    void setWidgets(QStackedLayout* stackedLayout, AuthorizationWidget* authWidget, MainMenuWidget* mainMenuWidget, CallWidget* callWidget, MeetingWidget* meetingWidget = nullptr);

    void switchToAuthorizationWidget();
    void switchToMainMenuWidget();
    void switchToCallWidget(const QString& friendNickname);
    void switchToMeetingWidget();

public slots:
    void onEnterFullscreenRequested();
    void onExitFullscreenRequested();

signals:
    void windowTitleChanged(const QString& title);
    void windowFullscreenRequested();
    void windowMaximizedRequested();
    void callWidgetShown();

private:
    std::shared_ptr<core::Core> m_coreClient = nullptr;
    QStackedLayout* m_stackedLayout = nullptr;
    AuthorizationWidget* m_authorizationWidget = nullptr;
    MainMenuWidget* m_mainMenuWidget = nullptr;
    CallWidget* m_callWidget = nullptr;
    MeetingWidget* m_meetingWidget = nullptr;
};
