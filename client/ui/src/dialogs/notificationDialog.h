#pragma once

#include <QWidget>
#include <QLabel>
#include <QMovie>

struct StyleNotificationDialog
{
    static QString mainWidgetStyle(bool isGreenStyle);
    static QString labelStyle(bool isGreenStyle);
};

class NotificationDialog : public QWidget
{
    Q_OBJECT
public:
    explicit NotificationDialog(QWidget* parent = nullptr, const QString& statusText = "", bool isGreenStyle = false, bool isAnimation = true);
    void setStatusText(const QString& text);
    void setGreenStyle(bool isGreenStyle);
    void setAnimationEnabled(bool isAnimation);

private:
    void applyStyle();

    QWidget* m_mainWidget = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_gifLabel = nullptr;
    QMovie* m_movie = nullptr;
    bool m_isGreenStyle = false;
    bool m_isAnimation = true;
};
