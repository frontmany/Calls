#include "screen.h"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

Screen::Screen(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
}

void Screen::setPixmap(const QPixmap& pixmap)
{
    m_pixmap = pixmap;
    updateGeometry();
    update();
}

void Screen::clear()
{
    m_pixmap = QPixmap();
    m_clearToWhite = true;
    setRoundedCornersEnabled(true);
    updateGeometry();
    update();
}

void Screen::setRoundedCornersEnabled(bool enabled)
{
    m_roundedCornersEnabled = enabled;
    update();
}

void Screen::setScaleMode(ScaleMode mode)
{
    if (m_scaleMode != mode)
    {
        m_scaleMode = mode;
        update();
    }
}

QSize Screen::sizeHint() const
{
    if (!m_pixmap.isNull()) {
        QSize pixmapSize = m_pixmap.size();
        return QSize(
            pixmapSize.width(),
            pixmapSize.height()
        );
    }

    return QWidget::sizeHint();
}

QSize Screen::minimumSizeHint() const
{
    return QSize(100, 100);
}

void Screen::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    const QRect widgetRect = this->rect();
    if (widgetRect.isEmpty())
        return;

    if (m_roundedCornersEnabled)
    {
        QPainterPath clipPath;
        clipPath.addRoundedRect(widgetRect, m_cornerRadius, m_cornerRadius);
        painter.setClipPath(clipPath);
    }

    if (m_pixmap.isNull() && m_clearToWhite)
    {
        painter.fillRect(widgetRect, Qt::white);
    }
    else
    {
        painter.fillRect(widgetRect, palette().window());
        m_clearToWhite = false;
    }

    if (!m_pixmap.isNull())
    {
        QPixmap scaledPixmap;
        QRect targetRect;
        QRect sourceRect;

        if (m_scaleMode == ScaleMode::CropToFit)
        {
            scaledPixmap = m_pixmap.scaled(widgetRect.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

            const int offsetX = (scaledPixmap.width() - widgetRect.width()) / 2;
            const int offsetY = (scaledPixmap.height() - widgetRect.height()) / 2;

            sourceRect = QRect(offsetX, offsetY, widgetRect.width(), widgetRect.height());
            targetRect = widgetRect;
        }
        else
        {
            scaledPixmap = m_pixmap.scaled(widgetRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

            const int x = widgetRect.x() + (widgetRect.width() - scaledPixmap.width()) / 2;
            const int y = widgetRect.y() + (widgetRect.height() - scaledPixmap.height()) / 2;

            targetRect = QRect(x, y, scaledPixmap.width(), scaledPixmap.height());
            sourceRect = QRect(QPoint(0, 0), scaledPixmap.size());
        }

        if (m_roundedCornersEnabled)
        {
            QPainterPath pixmapClip;
            pixmapClip.addRoundedRect(targetRect, m_cornerRadius, m_cornerRadius);
            painter.setClipPath(pixmapClip, Qt::IntersectClip);
        }

        painter.drawPixmap(targetRect, scaledPixmap, sourceRect);
    }
}