#pragma once

#include "dialogs/notificationDialogBase.h"

class ConnectionDownWithUserDialog : public NotificationDialogBase
{
    Q_OBJECT
public:
    explicit ConnectionDownWithUserDialog(QWidget* parent = nullptr, const QString& statusText = "");
};
