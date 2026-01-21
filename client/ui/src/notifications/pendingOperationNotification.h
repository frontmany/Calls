#pragma once

#include "notifications/baseNotification.h"

class PendingOperationNotification : public BaseNotification
{
public:
    explicit PendingOperationNotification(QWidget* parent = nullptr, const QString& statusText = "");
};
