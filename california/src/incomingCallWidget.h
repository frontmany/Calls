#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTimer>
#include <QPainter>

class ButtonIcon;

class IncomingCallWidget : public QWidget {
    Q_OBJECT
public:
    IncomingCallWidget(const QString& friendNickname, QWidget* parent = nullptr);
    ~IncomingCallWidget();
    const QString& getFriendNickname();

signals:
    void callAccepted(const QString& friendNickname);
    void callDeclined(const QString& friendNickname);

private slots:
    void updateTimer();

private:
    void setupUI();
    void setupTimer();
    void paintEvent(QPaintEvent* event) override;
    QColor generateRandomColor(const QString& seed);

    QString m_friendNickname;
    QTimer* m_timer;
    int m_remainingSeconds;

    // UI elements
    QLabel* m_avatarLabel;
    QLabel* m_nicknameLabel;
    QLabel* m_callTypeLabel;
    QLabel* m_timerLabel;
    ButtonIcon* m_acceptButton;
    ButtonIcon* m_declineButton;
};