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
        QSize scaledSize = m_pixmap.size().scaled(widgetRect.size(), Qt::KeepAspectRatioByExpanding);
        QPixmap scaledPixmap = m_pixmap.scaled(scaledSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        
        QRect sourceRect;
        if (scaledSize.width() > widgetRect.width())
        {
            int xOffset = (scaledSize.width() - widgetRect.width()) / 2;
            sourceRect = QRect(xOffset, 0, widgetRect.width(), scaledSize.height());
        }
        else
        {
            int yOffset = (scaledSize.height() - widgetRect.height()) / 2;
            sourceRect = QRect(0, yOffset, scaledSize.width(), widgetRect.height());
        }
        
        painter.drawPixmap(widgetRect, scaledPixmap, sourceRect);
    }
}