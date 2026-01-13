#pragma once

#include <QWidget>
#include <QLabel>

struct StyleFirstLaunchDialog
{
    static QString mainWidgetStyle(int radius, int border);
    static QString descriptionStyle(int fontSize, int padding);
    static QString okButtonStyle(int radius, int fontSize);
};

class FirstLaunchDialog : public QWidget
{
    Q_OBJECT
public:
    explicit FirstLaunchDialog(QWidget* parent = nullptr, const QString& imagePath = "", const QString& descriptionText = "");

signals:
    void closeRequested();
};
