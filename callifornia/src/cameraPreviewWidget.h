#pragma once
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QMouseEvent>
#include <QPoint>

class Screen;

class CameraPreviewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CameraPreviewWidget(QWidget* parent);
    ~CameraPreviewWidget() = default;
    void setPixmap(const QPixmap& pixmap);


protected:
    void updateVisibility(bool visible);
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
   
private slots:
    void onHideTimerTimeout();

private:
    void setupUI();

    QWidget* m_videoContainer;
    Screen* m_screenWidget;
    QVBoxLayout* m_mainLayout;
    QTimer* m_hideTimer;
};