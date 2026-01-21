#include "notifications/connectionRestoredNotification.h"
#include "notifications/baseNotificationStyleType.h"

ConnectionRestoredNotification::ConnectionRestoredNotification(QWidget* parent)
    : BaseNotification(parent, "Connection restored", BaseNotificationStyleType::GREEN, false)
{
}
