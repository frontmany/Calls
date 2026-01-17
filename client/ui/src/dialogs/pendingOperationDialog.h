#pragma once

#include "dialogs/notificationDialogBase.h"

class PendingOperationDialog : public NotificationDialogBase
{
    Q_OBJECT
public:
    explicit PendingOperationDialog(QWidget* parent = nullptr, const QString& statusText = "");
};
