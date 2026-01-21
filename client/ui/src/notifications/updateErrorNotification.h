#pragma once

#include "notifications/baseNotification.h"

class UpdateErrorNotification : public BaseNotification
{
public:
    explicit UpdateErrorNotification(QWidget* parent = nullptr);
};
