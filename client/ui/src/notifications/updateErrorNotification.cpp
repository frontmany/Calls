#include "notifications/updateErrorNotification.h"
#include "notifications/baseNotificationStyleType.h"

UpdateErrorNotification::UpdateErrorNotification(QWidget* parent)
    : BaseNotification(parent, "Update failed", BaseNotificationStyleType::RED, false)
{
}
