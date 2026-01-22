#pragma once

#include <QDialog>
#include <QLabel>
#include <QTimer>
#include <QMouseEvent>
#include "utilities/constant.h"

class ButtonIcon;

class IncomingCallDialog : public QDialog
{
    Q_OBJECT

public:
    explicit IncomingCallDialog(QWidget* parent, const QString& friendNickname, int remainingTime = DEFAULT_INCOMING_CALL_SECONDS);

    const QString& getFriendNickname() const;
    void setButtonsEnabled(bool enabled);
    void setRemainingTime(int remainingTime);

signals:
    void callAccepted(const QString& friendNickname);
    void callDeclined(const QString& friendNickname);
    void dialogClosed(const QList<QString>& pendingCalls);

protected:
    void reject() override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private slots:
    void updateTimer();

private:
    void setupUi();
    void setupTimer();
    void updateTimerLabel();

    QString m_friendNickname;
    QTimer* m_timer = nullptr;
    int m_remainingSeconds = 0;
    int m_totalSeconds = 1;

    QLabel* m_avatarLabel = nullptr;
    QLabel* m_nicknameLabel = nullptr;
    QLabel* m_callTypeLabel = nullptr;
    QLabel* m_timerLabel = nullptr;
    ButtonIcon* m_closeButton = nullptr;
    ButtonIcon* m_acceptButton = nullptr;
    ButtonIcon* m_declineButton = nullptr;

    bool m_dragging = false;
    QPoint m_dragStartPos;
    QPixmap m_backgroundTexture;
};
