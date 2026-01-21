#include "notificationController.h"
#include "widgets/overlayWidget.h"
#include "utilities/utilities.h"
#include "notifications/connectionDownNotification.h"
#include "notifications/connectionRestoredNotification.h"
#include "notifications/connectionDownWithUserNotification.h"
#include "notifications/connectionRestoredWithUserNotification.h"
#include "notifications/updateErrorNotification.h"
#include "notifications/pendingOperationNotification.h"
#include "notifications/errorNotification.h"

#include <QTimer>
#include <QPointer>
#include <functional>

NotificationController::NotificationController(QWidget* parent)
    : QObject(parent)
    , m_parent(parent)
    , m_connectionDownOverlay(nullptr)
    , m_connectionDownNotification(nullptr)
    , m_connectionDownTimer(nullptr)
    , m_connectionRestoredOverlay(nullptr)
    , m_connectionRestoredNotification(nullptr)
    , m_connectionRestoredTimer(nullptr)
    , m_connectionDownWithUserOverlay(nullptr)
    , m_connectionDownWithUserNotification(nullptr)
    , m_connectionRestoredWithUserOverlay(nullptr)
    , m_connectionRestoredWithUserNotification(nullptr)
    , m_connectionRestoredWithUserTimer(nullptr)
    , m_updateErrorOverlay(nullptr)
    , m_updateErrorNotification(nullptr)
    , m_updateErrorTimer(nullptr)
    , m_pendingOperationOverlay(nullptr)
    , m_pendingOperationNotification(nullptr)
    , m_errorNotificationOverlay(nullptr)
    , m_errorNotification(nullptr)
    , m_errorNotificationTimer(nullptr)
{
}

NotificationController::~NotificationController()
{
    hideConnectionDown();
    hideConnectionRestored();
    hideConnectionDownWithUser();
    hideConnectionRestoredWithUser();
    hidePendingOperation();
    hideUpdateError();
    hideErrorNotification();
}

void NotificationController::addManagedNotification(ManagedNotificationType type, bool hasKey, core::UserOperationType key, const QString& statusText)
{
    for (int i = 0; i < m_managedNotificationStack.size();)
    {
        const ManagedNotificationState& state = m_managedNotificationStack[i];
        const bool sameType = state.type == type;
        const bool shouldRemove = sameType
            && (type != ManagedNotificationType::PendingOperation || !hasKey || (state.hasKey && state.key == key));
        if (shouldRemove)
        {
            m_managedNotificationStack.removeAt(i);
            if (type != ManagedNotificationType::PendingOperation || hasKey)
            {
                break;
            }
            continue;
        }
        ++i;
    }

    m_managedNotificationStack.append({ type, hasKey, key, statusText });
}

void NotificationController::removeManagedNotification(ManagedNotificationType type, bool hasKey, core::UserOperationType key)
{
    for (int i = 0; i < m_managedNotificationStack.size();)
    {
        const ManagedNotificationState& state = m_managedNotificationStack[i];
        const bool sameType = state.type == type;
        const bool shouldRemove = sameType
            && (type != ManagedNotificationType::PendingOperation || !hasKey || (state.hasKey && state.key == key));
        if (shouldRemove)
        {
            m_managedNotificationStack.removeAt(i);
            if (type != ManagedNotificationType::PendingOperation || hasKey)
            {
                return;
            }
            continue;
        }
        ++i;
    }
}

bool NotificationController::isManagedNotificationActive(ManagedNotificationType type, bool hasKey, core::UserOperationType key) const
{
    switch (type)
    {
    case ManagedNotificationType::ConnectionDownWithUser:
        return m_connectionDownWithUserNotification != nullptr;
    case ManagedNotificationType::PendingOperation:
        if (!m_pendingOperationNotification)
        {
            return false;
        }
        if (!hasKey)
        {
            return true;
        }
        return m_hasActivePendingOperationKey && key == m_activePendingOperationKey;
    }

    return false;
}

void NotificationController::showManagedNotification(const ManagedNotificationState& state)
{
    switch (state.type)
    {
    case ManagedNotificationType::ConnectionDownWithUser:
    {
        BaseNotification* baseNotification = m_connectionDownWithUserNotification;
        auto createNotification = [statusText = state.statusText](QWidget* parent) -> BaseNotification*
        {
            return new ConnectionDownWithUserNotification(parent, statusText);
        };
        auto updateNotification = [statusText = state.statusText](BaseNotification* notification)
        {
            notification->setStatusText(statusText);
        };
        showNotificationInternal(m_connectionDownWithUserOverlay,
            baseNotification,
            false,
            createNotification,
            updateNotification);
        m_connectionDownWithUserNotification = baseNotification;
        break;
    }
    case ManagedNotificationType::PendingOperation:
    {
        BaseNotification* baseNotification = m_pendingOperationNotification;
        auto createNotification = [statusText = state.statusText](QWidget* parent) -> BaseNotification*
        {
            return new PendingOperationNotification(parent, statusText);
        };
        auto updateNotification = [statusText = state.statusText](BaseNotification* notification)
        {
            notification->setStatusText(statusText);
        };
        showNotificationInternal(m_pendingOperationOverlay,
            baseNotification,
            false,
            createNotification,
            updateNotification);
        m_pendingOperationNotification = baseNotification;
        m_hasActivePendingOperationKey = state.hasKey;
        if (state.hasKey)
        {
            m_activePendingOperationKey = state.key;
        }
        break;
    }
    }
}

void NotificationController::showLastManagedNotification()
{
    if (m_managedNotificationStack.isEmpty())
    {
        return;
    }

    showManagedNotification(m_managedNotificationStack.back());
}

void NotificationController::hideActiveNotification()
{
    if (m_connectionDownWithUserNotification)
    {
        BaseNotification* baseNotification = m_connectionDownWithUserNotification;
        hideNotificationInternal(m_connectionDownWithUserOverlay, baseNotification);
        m_connectionDownWithUserNotification = baseNotification;
        return;
    }

    if (m_pendingOperationNotification)
    {
        BaseNotification* baseNotification = m_pendingOperationNotification;
        hideNotificationInternal(m_pendingOperationOverlay, baseNotification);
        m_pendingOperationNotification = baseNotification;
        m_hasActivePendingOperationKey = false;
        return;
    }
}

void NotificationController::showNotificationInternal(OverlayWidget*& overlay,
    BaseNotification*& notification,
    bool createOverlay,
    const std::function<BaseNotification*(QWidget*)>& createNotification,
    const std::function<void(BaseNotification*)>& updateNotification,
    int autoHideMs)
{
    QWidget* parentWidget = m_parent;
    if (createOverlay)
    {
        if (!overlay)
        {
            overlay = new OverlayWidget(m_parent);
            overlay->setAttribute(Qt::WA_TranslucentBackground);
        }
        overlay->show();
        overlay->raise();
        parentWidget = overlay;
    }
    else
    {
        if (!overlay)
        {
            overlay = new OverlayWidget(m_parent);
            overlay->setAttribute(Qt::WA_TranslucentBackground);
        }
        overlay->setVisible(false);
    }

    if (!parentWidget && overlay)
    {
        parentWidget = overlay;
    }

    if (!notification && createNotification)
    {
        notification = createNotification(parentWidget);
    }

    if (notification)
    {
        if (updateNotification)
        {
            updateNotification(notification);
        }
        notification->setParent(parentWidget);
    }

    QPointer<OverlayWidget> overlayGuard = overlay;
    QPointer<BaseNotification> notificationGuard = notification;
    auto positionNotification = [this, createOverlay, overlayGuard, notificationGuard]()
    {
        if (!notificationGuard)
            return;

        QWidget* referenceWidget = createOverlay ? overlayGuard.data() : m_parent;
        if (!referenceWidget && overlayGuard)
        {
            referenceWidget = overlayGuard.data();
        }
        if (!referenceWidget)
            return;

        notificationGuard->adjustSize();
        QSize notificationSize = notificationGuard->size();

        int x = (referenceWidget->width() - notificationSize.width()) / 2;
        int y = 40;

        notificationGuard->move(x, y);
        notificationGuard->raise();
    };

    positionNotification();
    notification->show();
    QTimer::singleShot(0, this, positionNotification);

    if (overlay)
    {
        QObject::disconnect(overlay, &OverlayWidget::geometryChanged, this, nullptr);
        QObject::connect(overlay, &OverlayWidget::geometryChanged, this, positionNotification);
    }

    // Настройка автоскрытия
    if (autoHideMs > 0)
    {
        QPointer<OverlayWidget> overlayGuard = overlay;
        QPointer<BaseNotification> notificationGuard = notification;
        
        // Определяем тип нотификации для обновления указателей после скрытия
        enum NotificationType { Error, ConnectionDown, ConnectionRestored, ConnectionRestoredWithUser, UpdateError };
        NotificationType notificationType = Error;
        
        if (notification == m_errorNotification) {
            notificationType = Error;
        } else if (notification == m_connectionDownNotification) {
            notificationType = ConnectionDown;
        } else if (notification == m_connectionRestoredNotification) {
            notificationType = ConnectionRestored;
        } else if (notification == m_connectionRestoredWithUserNotification) {
            notificationType = ConnectionRestoredWithUser;
        } else if (notification == m_updateErrorNotification) {
            notificationType = UpdateError;
        }
        
        QTimer::singleShot(autoHideMs, this, [this, overlayGuard, notificationGuard, notificationType]() {
            if (notificationGuard && overlayGuard)
            {
                OverlayWidget* overlayPtr = overlayGuard.data();
                BaseNotification* notificationPtr = notificationGuard.data();
                hideNotificationInternal(overlayPtr, notificationPtr);
                
                // Обновляем соответствующий член класса
                switch (notificationType) {
                case Error:
                    m_errorNotification = nullptr;
                    m_errorNotificationOverlay = nullptr;
                    break;
                case ConnectionDown:
                    m_connectionDownNotification = nullptr;
                    m_connectionDownOverlay = nullptr;
                    break;
                case ConnectionRestored:
                    m_connectionRestoredNotification = nullptr;
                    m_connectionRestoredOverlay = nullptr;
                    break;
                case ConnectionRestoredWithUser:
                    m_connectionRestoredWithUserNotification = nullptr;
                    m_connectionRestoredWithUserOverlay = nullptr;
                    break;
                case UpdateError:
                    m_updateErrorNotification = nullptr;
                    m_updateErrorOverlay = nullptr;
                    break;
                }
                
                showLastManagedNotification();
            }
        });
    }
}

void NotificationController::hideNotificationInternal(OverlayWidget*& overlay, BaseNotification*& notification)
{
    // Проверяем, что объекты еще существуют перед использованием
    QPointer<BaseNotification> notificationGuard(notification);
    QPointer<OverlayWidget> overlayGuard(overlay);
    
    if (notificationGuard)
    {
        notificationGuard->disconnect();
        notificationGuard->hide();
        notificationGuard->deleteLater();
        notification = nullptr;
    }
    else if (notification)
    {
        // Объект уже удален, просто обнуляем указатель
        notification = nullptr;
    }

    if (overlayGuard)
    {
        overlayGuard->close();
        overlayGuard->deleteLater();
        overlay = nullptr;
    }
    else if (overlay)
    {
        // Объект уже удален, просто обнуляем указатель
        overlay = nullptr;
    }
}

void NotificationController::showConnectionDown(int autoHideMs)
{
    QPointer<BaseNotification> guard(m_connectionDownNotification);
    if (guard && !guard->isHidden())
    {
        guard->raise();
        return;
    }
    
    if (!guard)
    {
        m_connectionDownNotification = nullptr;
        m_connectionDownOverlay = nullptr;
    }

    hideActiveNotification();

    BaseNotification* baseNotification = m_connectionDownNotification;
    auto createNotification = [](QWidget* parent) -> BaseNotification*
    {
        return new ConnectionDownNotification(parent);
    };

    showNotificationInternal(m_connectionDownOverlay,
        baseNotification,
        true,
        createNotification,
        nullptr,
        autoHideMs);

    m_connectionDownNotification = baseNotification;
}

void NotificationController::hideConnectionDown()
{
    if (!m_connectionDownNotification)
    {
        return;
    }

    BaseNotification* baseNotification = m_connectionDownNotification;
    hideNotificationInternal(m_connectionDownOverlay, baseNotification);
    m_connectionDownNotification = baseNotification;
    showLastManagedNotification();
}

void NotificationController::showConnectionRestored(int autoHideMs)
{
    QPointer<BaseNotification> guard(m_connectionRestoredNotification);
    if (guard && !guard->isHidden())
    {
        guard->raise();
        return;
    }
    
    if (!guard)
    {
        m_connectionRestoredNotification = nullptr;
        m_connectionRestoredOverlay = nullptr;
    }

    hideActiveNotification();

    BaseNotification* baseNotification = m_connectionRestoredNotification;
    auto createNotification = [](QWidget* parent) -> BaseNotification*
    {
        return new ConnectionRestoredNotification(parent);
    };
    showNotificationInternal(m_connectionRestoredOverlay,
        baseNotification,
        false,
        createNotification,
        nullptr,
        autoHideMs);
    m_connectionRestoredNotification = baseNotification;
}

void NotificationController::hideConnectionRestored()
{
    if (!m_connectionRestoredNotification)
    {
        return;
    }

    BaseNotification* baseNotification = m_connectionRestoredNotification;
    hideNotificationInternal(m_connectionRestoredOverlay, baseNotification);
    m_connectionRestoredNotification = baseNotification;
    showLastManagedNotification();
}

void NotificationController::showConnectionDownWithUser(const QString& statusText)
{
    addManagedNotification(ManagedNotificationType::ConnectionDownWithUser, false, core::UserOperationType::AUTHORIZE, statusText);

    if (isManagedNotificationActive(ManagedNotificationType::ConnectionDownWithUser, false, core::UserOperationType::AUTHORIZE))
    {
        QPointer<BaseNotification> guard(m_connectionDownWithUserNotification);
        if (guard && !guard->isHidden())
        {
            guard->setStatusText(statusText);
            guard->raise();
            return;
        }
        else if (!guard)
        {
            m_connectionDownWithUserNotification = nullptr;
            m_connectionDownWithUserOverlay = nullptr;
        }
    }

    hideActiveNotification();
    ManagedNotificationState state;
    state.type = ManagedNotificationType::ConnectionDownWithUser;
    state.hasKey = false;
    state.statusText = statusText;
    showManagedNotification(state);
}

void NotificationController::hideConnectionDownWithUser()
{
    removeManagedNotification(ManagedNotificationType::ConnectionDownWithUser, false, core::UserOperationType::AUTHORIZE);

    if (!m_connectionDownWithUserNotification)
    {
        return;
    }

    BaseNotification* baseNotification = m_connectionDownWithUserNotification;
    hideNotificationInternal(m_connectionDownWithUserOverlay, baseNotification);
    m_connectionDownWithUserNotification = baseNotification;
    showLastManagedNotification();
}

void NotificationController::showConnectionRestoredWithUser(const QString& statusText, int autoHideMs)
{
    QPointer<BaseNotification> guard(m_connectionRestoredWithUserNotification);
    if (guard && !guard->isHidden())
    {
        guard->setStatusText(statusText);
        guard->raise();
        return;
    }
    
    if (!guard)
    {
        m_connectionRestoredWithUserNotification = nullptr;
        m_connectionRestoredWithUserOverlay = nullptr;
    }

    hideActiveNotification();

    BaseNotification* baseNotification = m_connectionRestoredWithUserNotification;
    auto createNotification = [statusText](QWidget* parent) -> BaseNotification*
    {
        return new ConnectionRestoredWithUserNotification(parent, statusText);
    };
    auto updateNotification = [statusText](BaseNotification* notification)
    {
        notification->setStatusText(statusText);
    };
    showNotificationInternal(m_connectionRestoredWithUserOverlay,
        baseNotification,
        false,
        createNotification,
        updateNotification,
        autoHideMs);
    m_connectionRestoredWithUserNotification = baseNotification;
}

void NotificationController::hideConnectionRestoredWithUser()
{
    if (!m_connectionRestoredWithUserNotification)
    {
        return;
    }

    BaseNotification* baseNotification = m_connectionRestoredWithUserNotification;
    hideNotificationInternal(m_connectionRestoredWithUserOverlay, baseNotification);
    m_connectionRestoredWithUserNotification = baseNotification;
    showLastManagedNotification();
}

void NotificationController::showPendingOperation(const QString& statusText, core::UserOperationType key)
{
    addManagedNotification(ManagedNotificationType::PendingOperation, true, key, statusText);

    if (isManagedNotificationActive(ManagedNotificationType::PendingOperation, true, key))
    {
        QPointer<BaseNotification> guard(m_pendingOperationNotification);
        if (guard && !guard->isHidden())
        {
            guard->setStatusText(statusText);
            guard->raise();
            m_activePendingOperationKey = key;
            m_hasActivePendingOperationKey = true;
            return;
        }
        else if (!guard)
        {
            m_pendingOperationNotification = nullptr;
            m_pendingOperationOverlay = nullptr;
        }
    }

    hideActiveNotification();
    ManagedNotificationState state;
    state.type = ManagedNotificationType::PendingOperation;
    state.hasKey = true;
    state.key = key;
    state.statusText = statusText;
    showManagedNotification(state);
}

void NotificationController::hidePendingOperation(core::UserOperationType key)
{
    removeManagedNotification(ManagedNotificationType::PendingOperation, true, key);

    if (!m_pendingOperationNotification)
    {
        return;
    }

    if (m_hasActivePendingOperationKey && key != m_activePendingOperationKey)
    {
        return;
    }

    BaseNotification* baseNotification = m_pendingOperationNotification;
    hideNotificationInternal(m_pendingOperationOverlay, baseNotification);
    m_pendingOperationNotification = baseNotification;
    m_hasActivePendingOperationKey = false;
    showLastManagedNotification();
}

void NotificationController::hidePendingOperation()
{
    removeManagedNotification(ManagedNotificationType::PendingOperation, false, core::UserOperationType::AUTHORIZE);

    if (!m_pendingOperationNotification)
    {
        return;
    }

    BaseNotification* baseNotification = m_pendingOperationNotification;
    hideNotificationInternal(m_pendingOperationOverlay, baseNotification);
    m_pendingOperationNotification = baseNotification;
    m_hasActivePendingOperationKey = false;
    showLastManagedNotification();
}

void NotificationController::showUpdateError(int autoHideMs)
{
    QPointer<BaseNotification> guard(m_updateErrorNotification);
    if (guard && !guard->isHidden())
    {
        guard->raise();
        return;
    }
    
    if (!guard)
    {
        m_updateErrorNotification = nullptr;
        m_updateErrorOverlay = nullptr;
    }

    hideActiveNotification();

    BaseNotification* baseNotification = m_updateErrorNotification;
    auto createNotification = [](QWidget* parent) -> BaseNotification*
    {
        return new UpdateErrorNotification(parent);
    };
    showNotificationInternal(m_updateErrorOverlay,
        baseNotification,
        false,
        createNotification,
        nullptr,
        autoHideMs);
    m_updateErrorNotification = baseNotification;
}

void NotificationController::hideUpdateError()
{
    if (!m_updateErrorNotification)
    {
        return;
    }

    BaseNotification* baseNotification = m_updateErrorNotification;
    hideNotificationInternal(m_updateErrorOverlay, baseNotification);
    m_updateErrorNotification = baseNotification;
    showLastManagedNotification();
}

void NotificationController::showErrorNotification(const QString& message, int autoHideMs)
{
    // Всегда скрываем старую нотификацию перед показом новой
    hideErrorNotification();
    hideActiveNotification();

    BaseNotification* baseNotification = nullptr;
    auto createNotification = [message](QWidget* parent) -> BaseNotification*
    {
        return new ErrorNotification(parent, message);
    };
    auto updateNotification = [message](BaseNotification* notification)
    {
        if (notification)
        {
            notification->setStatusText(message);
        }
    };

    showNotificationInternal(m_errorNotificationOverlay,
        baseNotification,
        false,
        createNotification,
        updateNotification,
        autoHideMs);

    m_errorNotification = baseNotification;
}

void NotificationController::hideErrorNotification()
{
    if (!m_errorNotification)
    {
        return;
    }

    // Проверяем, что объект еще существует
    QPointer<BaseNotification> guard(m_errorNotification);
    if (!guard)
    {
        // Объект уже удален, просто обнуляем указатели
        m_errorNotification = nullptr;
        if (m_errorNotificationOverlay)
        {
            QPointer<OverlayWidget> overlayGuard(m_errorNotificationOverlay);
            if (overlayGuard)
            {
                overlayGuard->close();
                overlayGuard->deleteLater();
            }
            m_errorNotificationOverlay = nullptr;
        }
        return;
    }

    BaseNotification* baseNotification = m_errorNotification;
    hideNotificationInternal(m_errorNotificationOverlay, baseNotification);
    m_errorNotification = nullptr;
    m_errorNotificationOverlay = nullptr;
    showLastManagedNotification();
}

void NotificationController::clearAll()
{
    m_managedNotificationStack.clear();
    hideConnectionDown();
    hideConnectionRestored();
    hideConnectionDownWithUser();
    hideConnectionRestoredWithUser();
    hidePendingOperation();
    hideUpdateError();
    hideErrorNotification();
}
