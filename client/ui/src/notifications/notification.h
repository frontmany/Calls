#pragma once

#include <QWidget>
#include <QLabel>
#include <QMovie>

#include "notifications/notificationStyleType.h"

struct NotificationStyle
{
    static QString mainWidgetStyle(NotificationStyleType styleType);
    static QString labelStyle(NotificationStyleType styleType);
};

class Notification : public QWidget
{
    Q_OBJECT
public:
    explicit Notification(QWidget* parent,
        const QString& statusText,
        NotificationStyleType styleType,
        bool waitingAnimation);

    void setStatusText(const QString& text);
    void setStyle(NotificationStyleType styleType);
    void setAnimationEnabled(bool isAnimation);

private:
    void applyStyle();
    void updateMinimumSize();

    QWidget* m_mainWidget = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_gifLabel = nullptr;
    QMovie* m_movie = nullptr;
    NotificationStyleType m_styleType = NotificationStyleType::BASE;
    bool m_isWaitingAnimation = true;
};
