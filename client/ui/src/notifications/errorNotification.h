#pragma once

#include "notifications/baseNotification.h"

class ErrorNotification : public BaseNotification
{
public:
    explicit ErrorNotification(QWidget* parent = nullptr, const QString& message = "");
};
