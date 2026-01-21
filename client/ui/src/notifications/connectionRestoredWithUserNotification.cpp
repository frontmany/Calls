#include "notifications/connectionRestoredWithUserNotification.h"
#include "notifications/baseNotificationStyleType.h"

ConnectionRestoredWithUserNotification::ConnectionRestoredWithUserNotification(QWidget* parent, const QString& statusText)
    : BaseNotification(parent, statusText, BaseNotificationStyleType::GREEN, false)
{
}
