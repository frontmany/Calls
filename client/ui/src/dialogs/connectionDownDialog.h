#pragma once

#include "dialogs/notificationDialogBase.h"

class ConnectionDownDialog : public NotificationDialogBase
{
public:
    explicit ConnectionDownDialog(QWidget* parent = nullptr);
};
