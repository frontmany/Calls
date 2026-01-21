#pragma once

#include <QWidget>
#include <QLabel>
#include <QMovie>

#include "notifications/baseNotificationStyleType.h"

struct BaseNotificationStyle
{
    static QString mainWidgetStyle(BaseNotificationStyleType styleType);
    static QString labelStyle(BaseNotificationStyleType styleType);
};

class BaseNotification : public QWidget
{
    Q_OBJECT
public:
    explicit BaseNotification(QWidget* parent,
        const QString& statusText,
        BaseNotificationStyleType styleType,
        bool waitingAnimation);

    void setStatusText(const QString& text);
    void setStyle(BaseNotificationStyleType styleType);
    void setAnimationEnabled(bool isAnimation);

private:
    void applyStyle();

    QWidget* m_mainWidget = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_gifLabel = nullptr;
    QMovie* m_movie = nullptr;
    BaseNotificationStyleType m_styleType = BaseNotificationStyleType::BASE;
    bool m_isWaitingAnimation = true;
};
