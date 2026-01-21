#pragma once

#include "notifications/baseNotification.h"

class ConnectionRestoredWithUserNotification : public BaseNotification
{
public:
    explicit ConnectionRestoredWithUserNotification(QWidget* parent = nullptr, const QString& statusText = "");
};
