#include "notificationController.h"
#include "widgets/overlayWidget.h"
#include "utilities/utilities.h"
#include "notifications/notification.h"
#include "notifications/notificationStyleType.h"

#include <QTimer>
#include <functional>

NotificationController::NotificationController(QWidget* parent)
    : QObject(parent)
    , m_parent(parent)
    , m_overlay(nullptr)
    , m_notification(nullptr)
    , m_isShowing(false)
    , m_autoHideTimer(nullptr)
{
    m_currentState.type = NotificationType::ConnectionDown;
}

NotificationController::~NotificationController()
{
    if (m_autoHideTimer)
    {
        m_autoHideTimer->stop();
        m_autoHideTimer->deleteLater();
    }

    if (m_notification)
    {
        m_notification->hide();
        m_notification->deleteLater();
    }

    if (m_overlay)
    {
        m_overlay->close();
        m_overlay->deleteLater();
    }
}

void NotificationController::showConnectionDown(int autoHideMs)
{
    NotificationState state;
    state.type = NotificationType::ConnectionDown;
    state.text = "Reconnecting...";
    state.isWaitingAnimation = true;
    state.isOverlay = true;
    state.notificationStyleType = NotificationStyleType::BASE;
    state.key = std::nullopt;

    showNotificationState(state, false, autoHideMs);
}

void NotificationController::showConnectionRestored(int autoHideMs)
{
    NotificationState state;
    state.type = NotificationType::ConnectionRestored;
    state.text = "Connection restored";
    state.isWaitingAnimation = false;
    state.isOverlay = false;
    state.notificationStyleType = NotificationStyleType::GREEN;
    state.key = std::nullopt;

    showNotificationState(state, false, autoHideMs);
}

void NotificationController::showConnectionDownWithUser(const QString& statusText)
{
    NotificationState state;
    state.type = NotificationType::ConnectionDownWithUser;
    state.text = statusText;
    state.isWaitingAnimation = true;
    state.isOverlay = false;
    state.notificationStyleType = NotificationStyleType::BASE;
    state.key = core::UserOperationType::AUTHORIZE;

    showNotificationState(state, true, 0);
}

void NotificationController::hideConnectionDownWithUser()
{
    removeNotificationFromStack(NotificationType::ConnectionDownWithUser, core::UserOperationType::AUTHORIZE);
    
    if (m_isShowing && m_currentState.type == NotificationType::ConnectionDownWithUser &&
        m_currentState.key == core::UserOperationType::AUTHORIZE)
    {
        hideCurrentNotification();
    }
    
    showLastNotification();
}

void NotificationController::showConnectionRestoredWithUser(const QString& statusText, int autoHideMs)
{
    NotificationState state;
    state.type = NotificationType::ConnectionRestoredWithUser;
    state.text = statusText;
    state.isWaitingAnimation = false;
    state.isOverlay = false;
    state.notificationStyleType = NotificationStyleType::GREEN;
    state.key = std::nullopt;

    showNotificationState(state, false, autoHideMs);
}

void NotificationController::showPendingOperation(const QString& statusText, core::UserOperationType key)
{
    NotificationState state;
    state.type = NotificationType::PendingOperation;
    state.text = statusText;
    state.isWaitingAnimation = true;
    state.isOverlay = false;
    state.notificationStyleType = NotificationStyleType::BASE;
    state.key = key;

    showNotificationState(state, true, 0);
}

void NotificationController::hidePendingOperation(core::UserOperationType key)
{
    // Удаляем из стека
    removeNotificationFromStack(NotificationType::PendingOperation, key);
    
    // Если сейчас показывается это уведомление, скрываем его и показываем последнее из стека
    if (m_isShowing && m_currentState.type == NotificationType::PendingOperation &&
        m_currentState.key.has_value() && m_currentState.key.value() == key)
    {
        hideCurrentNotification();
        showLastNotification();
    }
}

void NotificationController::showUpdateError(int autoHideMs)
{
    NotificationState state;
    state.type = NotificationType::UpdateError;
    state.text = "Update failed";
    state.isWaitingAnimation = false;
    state.isOverlay = false;
    state.notificationStyleType = NotificationStyleType::RED;
    state.key = std::nullopt;

    showNotificationState(state, false, autoHideMs);
}

void NotificationController::showErrorNotification(const QString& message, int autoHideMs)
{
    NotificationState state;
    state.type = NotificationType::Error;
    state.text = message;
    state.isWaitingAnimation = false;
    state.isOverlay = false;
    state.notificationStyleType = NotificationStyleType::RED;
    state.key = std::nullopt;

    showNotificationState(state, false, autoHideMs);
}

void NotificationController::clearAll()
{
    m_notificationStack.clear();
    if (m_autoHideTimer)
    {
        m_autoHideTimer->stop();
    }
    hideCurrentNotification();
}

void NotificationController::addNotificationToStack(const NotificationState& state)
{
    // Удаляем старую запись с таким же типом и ключом, если есть
    for (int i = 0; i < m_notificationStack.size();)
    {
        const NotificationState& stackState = m_notificationStack[i];
        if (stackState.type == state.type && 
            ((!stackState.key.has_value() && !state.key.has_value()) ||
             (stackState.key.has_value() && state.key.has_value() && stackState.key.value() == state.key.value())))
        {
            m_notificationStack.removeAt(i);
            break;
        }
        ++i;
    }

    m_notificationStack.append(state);
}

void NotificationController::removeNotificationFromStack(NotificationType type, const std::optional<core::UserOperationType>& key)
{
    for (int i = 0; i < m_notificationStack.size();)
    {
        const NotificationState& stackState = m_notificationStack[i];
        if (stackState.type == type &&
            ((!stackState.key.has_value() && !key.has_value()) ||
             (stackState.key.has_value() && key.has_value() && stackState.key.value() == key.value())))
        {
            m_notificationStack.removeAt(i);
            return;
        }
        ++i;
    }
}

void NotificationController::showNotificationState(const NotificationState& state, bool addToStack, int autoHideMs)
{
    // Если уже показываем уведомление, сохраняем текущее состояние в стек
    // только если текущее уведомление из стека (ConnectionDownWithUser или PendingOperation)
    if (m_isShowing)
    {
        bool currentIsFromStack = (m_currentState.type == NotificationType::ConnectionDownWithUser ||
                                   m_currentState.type == NotificationType::PendingOperation);
        if (currentIsFromStack)
        {
            addNotificationToStack(m_currentState);
        }
        hideCurrentNotification();
    }

    // Останавливаем таймер автоскрытия если был
    if (m_autoHideTimer)
    {
        m_autoHideTimer->stop();
    }

    // Добавляем новое состояние в стек если нужно
    if (addToStack)
    {
        addNotificationToStack(state);
    }

    // Показываем новое уведомление
    m_currentState = state;
    m_isShowing = true;

    // Создаем или обновляем overlay
    if (state.isOverlay)
    {
        if (!m_overlay)
        {
            m_overlay = new OverlayWidget(m_parent);
            m_overlay->setAttribute(Qt::WA_TranslucentBackground);
            connect(m_overlay, &OverlayWidget::geometryChanged, this, [this]() {
                positionNotification();
            });
        }
        if (!m_overlay->isVisible())
        {
            m_overlay->show();
            m_overlay->raise();
        }
    }
    else
    {
        if (m_overlay && m_overlay->isVisible())
        {
            m_overlay->hide();
        }
    }

    // Создаем или обновляем notification
    QWidget* parentWidget = state.isOverlay ? static_cast<QWidget*>(m_overlay) : m_parent;
    
    if (!m_notification)
    {
        m_notification = new Notification(parentWidget, state.text, state.notificationStyleType, state.isWaitingAnimation);
    }
    else
    {
        // Если родитель изменился, пересоздаем уведомление
        if (m_notification->parentWidget() != parentWidget)
        {
            m_notification->hide();
            m_notification->deleteLater();
            m_notification = new Notification(parentWidget, state.text, state.notificationStyleType, state.isWaitingAnimation);
        }
        else
        {
            // Обновляем существующее уведомление
            updateNotificationContent(state);
        }
    }

    positionNotification();
    m_notification->show();
    m_notification->raise();

    // Настройка автоскрытия
    if (autoHideMs > 0)
    {
        if (!m_autoHideTimer)
        {
            m_autoHideTimer = new QTimer(this);
            m_autoHideTimer->setSingleShot(true);
            connect(m_autoHideTimer, &QTimer::timeout, this, [this]() {
                hideCurrentNotification();
                showLastNotification();
            });
        }
        m_autoHideTimer->start(autoHideMs);
    }
}

void NotificationController::showLastNotification()
{
    if (m_notificationStack.isEmpty())
    {
        return;
    }

    NotificationState state = m_notificationStack.back();
    m_notificationStack.removeLast();
    
    // Останавливаем таймер автоскрытия если был
    if (m_autoHideTimer)
    {
        m_autoHideTimer->stop();
    }

    // Обновляем текущее состояние
    m_currentState = state;
    m_isShowing = true;

    // Создаем или обновляем overlay
    if (state.isOverlay)
    {
        if (!m_overlay)
        {
            m_overlay = new OverlayWidget(m_parent);
            m_overlay->setAttribute(Qt::WA_TranslucentBackground);
            connect(m_overlay, &OverlayWidget::geometryChanged, this, [this]() {
                positionNotification();
            });
        }
        if (!m_overlay->isVisible())
        {
            m_overlay->show();
            m_overlay->raise();
        }
    }
    else
    {
        if (m_overlay && m_overlay->isVisible())
        {
            m_overlay->hide();
        }
    }

    // Создаем или обновляем notification
    QWidget* parentWidget = state.isOverlay ? static_cast<QWidget*>(m_overlay) : m_parent;
    
    if (!m_notification)
    {
        m_notification = new Notification(parentWidget, state.text, state.notificationStyleType, state.isWaitingAnimation);
    }
    else
    {
        // Если родитель изменился, пересоздаем уведомление
        if (m_notification->parentWidget() != parentWidget)
        {
            m_notification->hide();
            m_notification->deleteLater();
            m_notification = new Notification(parentWidget, state.text, state.notificationStyleType, state.isWaitingAnimation);
        }
        else
        {
            // Обновляем существующее уведомление
            updateNotificationContent(state);
        }
    }

    positionNotification();
    m_notification->show();
    m_notification->raise();
}

void NotificationController::hideCurrentNotification()
{
    if (!m_isShowing)
    {
        return;
    }

    // Останавливаем таймер если есть
    if (m_autoHideTimer)
    {
        m_autoHideTimer->stop();
    }

    if (m_notification)
    {
        m_notification->hide();
    }

    if (m_overlay && m_overlay->isVisible())
    {
        m_overlay->hide();
    }

    m_isShowing = false;
}

void NotificationController::updateNotificationContent(const NotificationState& state)
{
    if (!m_notification)
    {
        return;
    }

    m_notification->setStatusText(state.text);
    m_notification->setStyle(state.notificationStyleType);
    m_notification->setAnimationEnabled(state.isWaitingAnimation);
}

void NotificationController::positionNotification()
{
    if (!m_notification || !m_parent)
    {
        return;
    }

    QWidget* referenceWidget = m_overlay && m_overlay->isVisible() ? m_overlay : m_parent;
    if (!referenceWidget)
    {
        return;
    }

    m_notification->adjustSize();
    QSize notificationSize = m_notification->size();

    int x = (referenceWidget->width() - notificationSize.width()) / 2;
    int y = 40;

    m_notification->move(x, y);
    m_notification->raise();
}
