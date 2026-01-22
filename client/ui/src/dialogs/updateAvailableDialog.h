#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>

struct StyleUpdateAvailableDialog
{
    static QString buttonStyle();
    static QString containerStyle();
};

class UpdateAvailableDialog : public QWidget
{
    Q_OBJECT

public:
    explicit UpdateAvailableDialog(QWidget* parent = nullptr);

signals:
    void updateButtonClicked();

private:
    QPushButton* m_updateButton = nullptr;
    QLabel* m_confettiLabel = nullptr;
};
