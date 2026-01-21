#include "notifications/connectionDownWithUserNotification.h"
#include "notifications/baseNotificationStyleType.h"

ConnectionDownWithUserNotification::ConnectionDownWithUserNotification(QWidget* parent, const QString& statusText)
    : BaseNotification(parent, statusText, BaseNotificationStyleType::BASE, true)
{
}
