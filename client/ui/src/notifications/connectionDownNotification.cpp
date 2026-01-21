#include "notifications/connectionDownNotification.h"
#include "notifications/baseNotificationStyleType.h"

ConnectionDownNotification::ConnectionDownNotification(QWidget* parent)
    : BaseNotification(parent, "Reconnecting...", BaseNotificationStyleType::BASE, true)
{
}
