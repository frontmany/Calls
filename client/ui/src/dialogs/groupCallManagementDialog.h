#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QProgressBar>
#include <QLabel>

class GroupCallManagementDialog : public QWidget
{
    Q_OBJECT

public:
    explicit GroupCallManagementDialog(QWidget* parent = nullptr);

    void showConnectingState(const QString& roomId);
    void showInitialState();
    void setJoinProgress(int percent);
    void setJoinStatus(const QString& status);

signals:
    void closeRequested();
    void createCallRequested(const QString& uid);
    void joinCallRequested(const QString& uid);
    void joinCancelled();

private:
    QString generateCallUid() const;

    QStackedWidget* m_stackedWidget = nullptr;

    // Initial state
    QWidget* m_initialWidget = nullptr;
    QLineEdit* m_meetingIdEdit = nullptr;
    QPushButton* m_createCallButton = nullptr;
    QPushButton* m_joinMeetingButton = nullptr;

    // Connecting state
    QWidget* m_connectingWidget = nullptr;
    QLabel* m_roomIdLabel = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QLabel* m_statusLabel = nullptr;
    QPushButton* m_cancelRequestButton = nullptr;
};
