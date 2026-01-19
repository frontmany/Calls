#pragma once

#include <QWidget>
#include <QLabel>
#include <QMovie>

struct NotificationDialogStyle
{
    static QString mainWidgetStyle(bool isGreenStyle);
    static QString labelStyle(bool isGreenStyle);
    static QString mainWidgetRedStyle();
    static QString labelRedStyle();
};

class NotificationDialogBase : public QWidget
{
    Q_OBJECT
public:
    explicit NotificationDialogBase(QWidget* parent,
        const QString& statusText,
        bool isGreenStyle,
        bool isAnimation);
    void setStatusText(const QString& text);
    void setGreenStyle(bool isGreenStyle);
    void setRedStyle(bool isRedStyle);
    void setAnimationEnabled(bool isAnimation);

private:
    void applyStyle();

    QWidget* m_mainWidget = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_gifLabel = nullptr;
    QMovie* m_movie = nullptr;
    bool m_isGreenStyle = false;
    bool m_isRedStyle = false;
    bool m_isAnimation = true;
};
