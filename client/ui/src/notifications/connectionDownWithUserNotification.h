#pragma once

#include "notifications/baseNotification.h"

class ConnectionDownWithUserNotification : public BaseNotification
{
public:
    explicit ConnectionDownWithUserNotification(QWidget* parent = nullptr, const QString& statusText = "");
};
