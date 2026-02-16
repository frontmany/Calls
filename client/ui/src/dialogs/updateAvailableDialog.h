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
    void setNewVersion(const QString& newVersion);

signals:
    void updateButtonClicked();

private:
    void updateButtonText();

    QPushButton* m_updateButton = nullptr;
    QLabel* m_buttonTextLabel = nullptr;
    QLabel* m_arrowLabel = nullptr;
    QString m_newVersion;
};
