#include "screen.h"
#include "utilities/utility.h"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QCoreApplication>

Screen::Screen(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
}

void Screen::rebuildScaledPixmap()
{
    m_hasPreparedDraw = false;
    m_scaledPixmap = QPixmap();

    if (m_sourcePixmap.isNull())
        return;

    const QRect widgetRect = rect();
    if (widgetRect.isEmpty())
        return;

    if (m_scaleMode == ScaleMode::CropToFit)
    {
        m_scaledPixmap = m_sourcePixmap.scaled(widgetRect.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

        const int offsetX = (m_scaledPixmap.width() - widgetRect.width()) / 2;
        const int offsetY = (m_scaledPixmap.height() - widgetRect.height()) / 2;

        m_drawSourceRect = QRect(offsetX, offsetY, widgetRect.width(), widgetRect.height());
        m_drawTargetRect = widgetRect;
    }
    else
    {
        m_scaledPixmap = m_sourcePixmap.scaled(widgetRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

        const int x = widgetRect.x() + (widgetRect.width() - m_scaledPixmap.width()) / 2;
        const int y = widgetRect.y() + (widgetRect.height() - m_scaledPixmap.height()) / 2;

        m_drawTargetRect = QRect(x, y, m_scaledPixmap.width(), m_scaledPixmap.height());
        m_drawSourceRect = QRect(QPoint(0, 0), m_scaledPixmap.size());
    }

    m_hasPreparedDraw = !m_scaledPixmap.isNull();
}

void Screen::setPixmap(const QPixmap& pixmap)
{
    m_sourcePixmap = pixmap;
    rebuildScaledPixmap();
    updateGeometry();
    update();
}

void Screen::clear()
{
    m_sourcePixmap = QPixmap();
    m_scaledPixmap = QPixmap();
    m_hasPreparedDraw = false;
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
        rebuildScaledPixmap();
        update();
    }
}

QSize Screen::contentSize() const
{
    return m_sourcePixmap.isNull() ? QSize() : m_sourcePixmap.size();
}

QSize Screen::sizeHint() const
{
    QSize content = contentSize();
    if (!content.isEmpty()) return content;
    return QWidget::sizeHint();
}

QSize Screen::minimumSizeHint() const
{
    return QSize(scale(100), scale(100));
}

void Screen::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    rebuildScaledPixmap();
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

    if (m_sourcePixmap.isNull() && m_clearToWhite)
    {
        painter.fillRect(widgetRect, Qt::white);
    }
    else
    {
        painter.fillRect(widgetRect, palette().window());
        m_clearToWhite = false;
    }

    if (m_hasPreparedDraw && !m_scaledPixmap.isNull())
    {
        if (m_roundedCornersEnabled)
        {
            QPainterPath pixmapClip;
            pixmapClip.addRoundedRect(m_drawTargetRect, m_cornerRadius, m_cornerRadius);
            painter.setClipPath(pixmapClip, Qt::IntersectClip);
        }

        painter.drawPixmap(m_drawTargetRect, m_scaledPixmap, m_drawSourceRect);
    }
}

void Screen::mouseMoveEvent(QMouseEvent* event)
{
    if (parentWidget()) {
        QPoint parentPos = mapToParent(event->pos());
        QMouseEvent parentEvent(
            event->type(),
            parentPos,
            event->globalPosition(),
            event->button(),
            event->buttons(),
            event->modifiers()
        );
        QCoreApplication::sendEvent(parentWidget(), &parentEvent);
    }
    QWidget::mouseMoveEvent(event);
}
