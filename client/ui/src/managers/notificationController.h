#pragma once
#include <QObject>
#include <QWidget>
#include <QList>
#include <QString>
#include <QTimer>
#include <functional>
#include "userOperationType.h"

class OverlayWidget;
class BaseNotification;

class NotificationController : public QObject
{
    Q_OBJECT

public:
    explicit NotificationController(QWidget* parent);
    ~NotificationController();

    // ConnectionDown - автоскрытие через таймер
    void showConnectionDown(int autoHideMs = 0);
    void hideConnectionDown();

    // ConnectionRestored - автоскрытие через таймер
    void showConnectionRestored(int autoHideMs = 0);
    void hideConnectionRestored();

    // ConnectionDownWithUser - управляется через стек
    void showConnectionDownWithUser(const QString& statusText);
    void hideConnectionDownWithUser();

    // ConnectionRestoredWithUser - автоскрытие через таймер
    void showConnectionRestoredWithUser(const QString& statusText, int autoHideMs = 0);
    void hideConnectionRestoredWithUser();

    // PendingOperation - управляется через стек
    void showPendingOperation(const QString& statusText, core::UserOperationType key);
    void hidePendingOperation(core::UserOperationType key);
    void hidePendingOperation();

    // UpdateError - автоскрытие через таймер
    void showUpdateError(int autoHideMs = 0);
    void hideUpdateError();

    // Error - автоскрытие через таймер
    void showErrorNotification(const QString& message, int autoHideMs = 0);
    void hideErrorNotification();

    void clearAll();

private:
    enum class ManagedNotificationType
    {
        ConnectionDownWithUser,
        PendingOperation
    };

    struct ManagedNotificationState
    {
        ManagedNotificationType type;
        bool hasKey = false;
        core::UserOperationType key = core::UserOperationType::AUTHORIZE;
        QString statusText;
    };

    void addManagedNotification(ManagedNotificationType type, bool hasKey, core::UserOperationType key, const QString& statusText);
    void removeManagedNotification(ManagedNotificationType type, bool hasKey, core::UserOperationType key);
    bool isManagedNotificationActive(ManagedNotificationType type, bool hasKey, core::UserOperationType key) const;
    void showManagedNotification(const ManagedNotificationState& state);
    void showLastManagedNotification();
    void hideActiveNotification();

    void showNotificationInternal(OverlayWidget*& overlay,
        BaseNotification*& notification,
        bool createOverlay,
        const std::function<BaseNotification*(QWidget*)>& createNotification,
        const std::function<void(BaseNotification*)>& updateNotification,
        int autoHideMs = 0);

    void hideNotificationInternal(OverlayWidget*& overlay, BaseNotification*& notification);

    QWidget* m_parent;
    QList<ManagedNotificationState> m_managedNotificationStack;
    bool m_hasActivePendingOperationKey = false;
    core::UserOperationType m_activePendingOperationKey = core::UserOperationType::AUTHORIZE;

    OverlayWidget* m_connectionDownOverlay = nullptr;
    BaseNotification* m_connectionDownNotification = nullptr;
    QTimer* m_connectionDownTimer = nullptr;

    OverlayWidget* m_connectionRestoredOverlay = nullptr;
    BaseNotification* m_connectionRestoredNotification = nullptr;
    QTimer* m_connectionRestoredTimer = nullptr;

    OverlayWidget* m_connectionDownWithUserOverlay = nullptr;
    BaseNotification* m_connectionDownWithUserNotification = nullptr;

    OverlayWidget* m_connectionRestoredWithUserOverlay = nullptr;
    BaseNotification* m_connectionRestoredWithUserNotification = nullptr;
    QTimer* m_connectionRestoredWithUserTimer = nullptr;

    OverlayWidget* m_updateErrorOverlay = nullptr;
    BaseNotification* m_updateErrorNotification = nullptr;
    QTimer* m_updateErrorTimer = nullptr;

    OverlayWidget* m_pendingOperationOverlay = nullptr;
    BaseNotification* m_pendingOperationNotification = nullptr;

    OverlayWidget* m_errorNotificationOverlay = nullptr;
    BaseNotification* m_errorNotification = nullptr;
    QTimer* m_errorNotificationTimer = nullptr;
};
