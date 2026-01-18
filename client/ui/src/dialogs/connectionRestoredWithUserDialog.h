#pragma once

#include "dialogs/notificationDialogBase.h"

class ConnectionRestoredWithUserDialog : public NotificationDialogBase
{
    Q_OBJECT
public:
    explicit ConnectionRestoredWithUserDialog(QWidget* parent = nullptr, const QString& statusText = "");
};
