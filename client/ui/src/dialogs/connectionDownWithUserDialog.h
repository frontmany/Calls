#pragma once

#include "dialogs/notificationDialogBase.h"

class ConnectionDownWithUserDialog : public NotificationDialogBase
{
public:
    explicit ConnectionDownWithUserDialog(QWidget* parent = nullptr, const QString& statusText = "");
};
