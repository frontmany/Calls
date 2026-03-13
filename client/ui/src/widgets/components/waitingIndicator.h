#pragma once

#include <QColor>
#include <QWidget>

class QTimer;

class WaitingIndicator : public QWidget
{
public:
    explicit WaitingIndicator(QWidget* parent = nullptr);

    void start();
    void stop();
    void setColor(const QColor& color);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    void advanceFrame();

private:
    QTimer* m_timer = nullptr;
    QColor m_color;
    int m_activeDotIndex = 0;
    int m_dotCount = 10;
};
