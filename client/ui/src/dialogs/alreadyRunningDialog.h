#pragma once

#include <QWidget>
#include <QLabel>

struct StyleAlreadyRunningDialog
{
    static QString mainWidgetStyle(int radius, int border);
    static QString titleStyle(int scalePx, const QString& fontFamily = "Outfit");
    static QString messageStyle(int scalePx, const QString& fontFamily = "Outfit");
    static QString closeButtonStyle(int radius, int paddingH, int paddingV, int fontSize);
};

class AlreadyRunningDialog : public QWidget
{
    Q_OBJECT
public:
    explicit AlreadyRunningDialog(QWidget* parent = nullptr);
signals:
    void closeRequested();
};
