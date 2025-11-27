#include "cameraPreviewWidget.h"
#include "screen.h"
#include "scaleFactor.h"
#include <QPainter>
#include <QResizeEvent>
#include <QEnterEvent>

CameraPreviewWidget::CameraPreviewWidget(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setFixedSize(scale(320), scale(180));
    setupUI();
}

void CameraPreviewWidget::setupUI()
{
    m_screenWidget = new Screen(this);
    m_screenWidget->setFixedSize(scale(320), scale(180));

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    m_videoContainer = new QWidget(this);
    m_videoContainer->setStyleSheet(
        QString("QWidget {"
            "   background-color: rgba(0, 0, 0, 200);"
            "   border-radius: %1px;"
            "}").arg(QString::fromStdString(std::to_string(scale(8)))));
    m_videoContainer->setFixedSize(scale(320), scale(180));

    QVBoxLayout* containerLayout = new QVBoxLayout(m_videoContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);

    m_mainLayout->addWidget(m_videoContainer);

    m_hideTimer = new QTimer(this);
    m_hideTimer->setSingleShot(true);
    m_hideTimer->setInterval(3000);
    connect(m_hideTimer, &QTimer::timeout, this, &CameraPreviewWidget::onHideTimerTimeout);
}

void CameraPreviewWidget::updateVisibility(bool visible)
{
    if (visible)
    {
        show();
        m_hideTimer->stop();
    }
    else
    {
        hide();
        m_hideTimer->start();
    }
}

void CameraPreviewWidget::setPixmap(const QPixmap& pixmap) {
    m_screenWidget->setPixmap(pixmap);
}

void CameraPreviewWidget::enterEvent(QEnterEvent* event) {
    updateVisibility(true);
    QWidget::enterEvent(event);
}

void CameraPreviewWidget::leaveEvent(QEvent* event) {
    updateVisibility(false);
    QWidget::leaveEvent(event);
}

void CameraPreviewWidget::mousePressEvent(QMouseEvent* event) {
    updateVisibility(true);
    event->ignore();
}

void CameraPreviewWidget::mouseMoveEvent(QMouseEvent* event)
{
    updateVisibility(true);
    event->ignore();
}

void CameraPreviewWidget::onHideTimerTimeout()
{
    updateVisibility(false);
}