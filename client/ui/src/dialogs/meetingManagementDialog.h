#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QLabel>

class MeetingManagementDialog : public QWidget
{
    Q_OBJECT

public:
    explicit MeetingManagementDialog(QWidget* parent = nullptr);

    void showConnectingState(const QString& roomId);
    void showInitialState();
    void setJoinStatus(const QString& status);

signals:
    void closeRequested();
    void createMeetingRequested(const QString& uid);
    void joinMeetingRequested(const QString& uid);
    void joinCancelled();

private:
    QString generateMeetingUid() const;

    QStackedWidget* m_stackedWidget = nullptr;

    // Initial state
    QWidget* m_initialWidget = nullptr;
    QLabel* m_meetingHeartsIcon = nullptr;
    QLineEdit* m_meetingIdEdit = nullptr;
    QPushButton* m_createMeetingButton = nullptr;
    QPushButton* m_joinMeetingButton = nullptr;

    // Connecting state
    QWidget* m_connectingWidget = nullptr;
    QLabel* m_joinMeetingHeartsIcon = nullptr;
    QLabel* m_roomIdLabel = nullptr;
    QLabel* m_waitingGifLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    QPushButton* m_cancelRequestButton = nullptr;

    int m_initialHeight = 0;
    int m_connectingHeight = 0;
};
