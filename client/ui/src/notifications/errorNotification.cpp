#include "notifications/errorNotification.h"
#include "notifications/baseNotificationStyleType.h"

ErrorNotification::ErrorNotification(QWidget* parent, const QString& message)
    : BaseNotification(parent, message, BaseNotificationStyleType::RED, false)
{
}
