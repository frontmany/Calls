#pragma once
#include <QObject>
#include <QWidget>
#include <QList>
#include <QString>
#include <QTimer>
#include <functional>
#include <optional>
#include "userOperationType.h"
#include "notifications/notificationStyleType.h"

class OverlayWidget;
class Notification;

class NotificationController : public QObject
{
private:
    enum class NotificationType
    {
        ConnectionDown,
        ConnectionRestored,
        ConnectionDownWithUser,
        ConnectionRestoredWithUser,
        PendingOperation,
        UpdateError,
        Error
    };

    struct NotificationState
    {
        NotificationType type;
        QString text;
        bool isWaitingAnimation = false;
        bool isOverlay = false;
        std::optional<core::UserOperationType> key;
        NotificationStyleType notificationStyleType = NotificationStyleType::BASE;

        bool operator==(const NotificationState& other) const
        {
            return type == other.type &&
                (!key.has_value() && !other.key.has_value() ||
                    key.has_value() && other.key.has_value() && key.value() == other.key.value());
        }
    };

    Q_OBJECT

public:
    explicit NotificationController(QWidget* parent);
    ~NotificationController();

    // ConnectionDown - автоскрытие через таймер
    void showConnectionDown(int autoHideMs = 0);

    // ConnectionRestored - автоскрытие через таймер
    void showConnectionRestored(int autoHideMs = 0);

    // ConnectionDownWithUser - управляется через стек
    void showConnectionDownWithUser(const QString& statusText);
    void hideConnectionDownWithUser();

    // ConnectionRestoredWithUser - автоскрытие через таймер
    void showConnectionRestoredWithUser(const QString& statusText, int autoHideMs = 0);

    // PendingOperation - управляется через стек
    void showPendingOperation(const QString& statusText, core::UserOperationType key);
    void hidePendingOperation(core::UserOperationType key);

    // UpdateError - автоскрытие через таймер
    void showUpdateError(int autoHideMs = 0);

    // Error - автоскрытие через таймер
    void showErrorNotification(const QString& message, int autoHideMs = 0);

    void clearAll();

private:
    void addNotificationToStack(const NotificationState& state);
    void removeNotificationFromStack(NotificationType type, const std::optional<core::UserOperationType>& key);
    void showNotificationState(const NotificationState& state, bool addToStack, int autoHideMs = 0);
    void showLastNotification();
    void hideCurrentNotification();

    void updateNotificationContent(const NotificationState& state);
    void positionNotification();

private:
    QWidget* m_parent;
    QList<NotificationState> m_notificationStack;

    OverlayWidget* m_overlay = nullptr;
    Notification* m_notification = nullptr;
    NotificationState m_currentState;
    bool m_isShowing = false;
    QTimer* m_autoHideTimer = nullptr;
};
