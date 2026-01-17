#pragma once

#include "dialogs/notificationDialogBase.h"

class ConnectionDownDialog : public NotificationDialogBase
{
    Q_OBJECT
public:
    explicit ConnectionDownDialog(QWidget* parent = nullptr);
};
