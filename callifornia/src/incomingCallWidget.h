#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTimer>
#include <QPainter>

class ButtonIcon;

struct StyleIncomingCallWidget {
    static const QColor m_backgroundColor;
    static const QColor m_borderColor;
    static const QColor m_nicknameTextColor;
    static const QColor m_callTypeTextColor;
    static const QColor m_timerTextColor;
    static const QColor m_timerCircleColor;

    static QString widgetStyle();
    static QString nicknameStyle();
    static QString callTypeStyle();
    static QString timerStyle();
    static QString avatarStyle(const QColor& color);
};

class IncomingCallWidget : public QWidget {
    Q_OBJECT
public:
    IncomingCallWidget(QWidget* parent, const QString& friendNickname, int remainingTime = 32);
    ~IncomingCallWidget();
    const QString& getFriendNickname() const;
    int getRemainingTime() const;

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