#pragma once

#include "notifications/baseNotification.h"

class ConnectionRestoredNotification : public BaseNotification
{
public:
    explicit ConnectionRestoredNotification(QWidget* parent = nullptr);
};
