#pragma once

#include "dialogs/notificationDialogBase.h"

class ConnectionRestoredDialog : public NotificationDialogBase
{
    Q_OBJECT
public:
    explicit ConnectionRestoredDialog(QWidget* parent = nullptr);
};
