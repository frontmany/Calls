#include "dialogs/updateErrorDialog.h"

UpdateErrorDialog::UpdateErrorDialog(QWidget* parent)
    : NotificationDialogBase(parent, "Update failed", false, false)
{
    setRedStyle(true);
}
