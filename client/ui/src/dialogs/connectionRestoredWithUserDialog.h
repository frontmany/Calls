#pragma once

#include "dialogs/notificationDialogBase.h"

class ConnectionRestoredWithUserDialog : public NotificationDialogBase
{
public:
    explicit ConnectionRestoredWithUserDialog(QWidget* parent = nullptr, const QString& statusText = "");
};
