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
    updateGeometry();
    update();
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

    QPainterPath clipPath;
    clipPath.addRoundedRect(widgetRect, m_cornerRadius, m_cornerRadius);
    painter.setClipPath(clipPath);

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
        const QSize targetSize = m_pixmap.size().scaled(widgetRect.size(), Qt::KeepAspectRatio);
        const QPoint topLeft(
            widgetRect.x() + (widgetRect.width() - targetSize.width()) / 2,
            widgetRect.y() + (widgetRect.height() - targetSize.height()) / 2
        );
        const QRect targetRect(topLeft, targetSize);
        painter.drawPixmap(targetRect, m_pixmap);
    }
}