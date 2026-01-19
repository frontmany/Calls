#pragma once

#include "dialogs/notificationDialogBase.h"

class UpdateErrorDialog : public NotificationDialogBase
{
public:
    explicit UpdateErrorDialog(QWidget* parent = nullptr);
};
