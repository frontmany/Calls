#pragma once

#include <QWidget>
#include <QLabel>
#include <QMovie>

struct StyleWaitingStatusDialog
{
    static QString mainWidgetStyle();
    static QString labelStyle();
};

class WaitingStatusDialog : public QWidget
{
    Q_OBJECT
public:
    explicit WaitingStatusDialog(QWidget* parent = nullptr, const QString& statusText = "");
    void setStatusText(const QString& text);

private:
    QLabel* m_statusLabel = nullptr;
    QLabel* m_gifLabel = nullptr;
    QMovie* m_movie = nullptr;
};
