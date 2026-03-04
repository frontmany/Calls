#pragma once

#include <QWidget>
#include <QPushButton>

class EndMeetingConfirmationDialog : public QWidget
{
    Q_OBJECT

public:
    explicit EndMeetingConfirmationDialog(QWidget* parent = nullptr);

signals:
    void endMeetingConfirmed();
    void endMeetingCancelled();

private:
    QPushButton* m_okButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
};
