#pragma once

#include <QWidget>
#include <QLabel>

class QPushButton;
class QMovie;

struct StyleUpdatingDialog
{
    static QString mainWidgetStyle(int borderRadius);
    static QString progressStyle();
    static QString titleStyle();
    static QString exitButtonStyle(int radius, int paddingH, int paddingV, int fontSize);
};

class UpdatingDialog : public QWidget
{
    Q_OBJECT

public:
    explicit UpdatingDialog(QWidget* parent = nullptr);

    void setProgress(double progressPercent);
    void setStatus(const QString& text, bool hideProgress);

signals:
    void closeRequested();
  
private:
    QLabel* m_progressLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_gifLabel = nullptr;
    QPushButton* m_exitButton = nullptr;
    QMovie* m_movie = nullptr;
};
