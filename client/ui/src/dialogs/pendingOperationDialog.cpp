#include "dialogs/pendingOperationDialog.h"

PendingOperationDialog::PendingOperationDialog(QWidget* parent, const QString& statusText)
    : NotificationDialogBase(parent, statusText, false, true)
{
}
