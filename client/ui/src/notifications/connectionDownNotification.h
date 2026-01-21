#pragma once

#include "notifications/baseNotification.h"

class ConnectionDownNotification : public BaseNotification
{
public:
    explicit ConnectionDownNotification(QWidget* parent = nullptr);
};
