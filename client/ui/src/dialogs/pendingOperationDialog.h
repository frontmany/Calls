#pragma once

#include "dialogs/notificationDialogBase.h"

class PendingOperationDialog : public NotificationDialogBase
{
public:
    explicit PendingOperationDialog(QWidget* parent = nullptr, const QString& statusText = "");
};
