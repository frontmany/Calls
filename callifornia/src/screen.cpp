#include "screen.h"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QResizeEvent>

Screen::Screen(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
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
    enableRoundedCorners(true);
    updateGeometry();
    update();
}

void Screen::enableRoundedCorners(bool enabled)
{
    if (m_roundedCornersEnabled != enabled)
    {
        m_roundedCornersEnabled = enabled;
        update();
    }
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
        const QPixmap scaledPixmap = m_pixmap.scaled(widgetRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

        const int x = widgetRect.x() + (widgetRect.width() - scaledPixmap.width()) / 2;
        const int y = widgetRect.y();

        painter.drawPixmap(QPoint(x, y), scaledPixmap);
    }
}