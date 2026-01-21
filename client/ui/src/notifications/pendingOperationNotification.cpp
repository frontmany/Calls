#include "notifications/pendingOperationNotification.h"
#include "notifications/baseNotificationStyleType.h"

PendingOperationNotification::PendingOperationNotification(QWidget* parent, const QString& statusText)
    : BaseNotification(parent, statusText, BaseNotificationStyleType::BASE, true)
{
}
