#include "screen.h"
#include "gpuNv12VideoSurface.h"
#include "utilities/utility.h"
#include "utilities/logger.h"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QCoreApplication>

namespace
{
    bool s_loggedNv12GpuPresent = false;
}

Screen::Screen(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
}

void Screen::ensureGpuSurface()
{
    if (m_gpuSurface)
        return;

    m_gpuSurface = new GpuNv12VideoSurface(this);
    m_gpuSurface->setAttribute(Qt::WA_TranslucentBackground, true);
    m_gpuSurface->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_gpuSurface->hide();
}

void Screen::rebuildScaledPixmap()
{
    m_hasPreparedDraw = false;
    m_scaledPixmap = QPixmap();

    if (m_useGpu)
        return;

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
    m_useGpu = false;
    if (m_gpuSurface)
    {
        m_gpuSurface->hide();
        m_gpuSurface->clearMask();
        m_gpuSurface->setNv12Frame(core::VideoFrameBuffer{});
    }
    m_sourcePixmap = pixmap;
    rebuildScaledPixmap();
    updateGeometry();
    update();
}

void Screen::setVideoFrame(const core::VideoFrameBuffer& frame)
{
    if (frame.format == core::VideoPixelFormat::Nv12 && !frame.isEmpty())
    {
        if (!s_loggedNv12GpuPresent)
        {
            s_loggedNv12GpuPresent = true;
            LOG_INFO("Screen: NV12 -> GpuNv12VideoSurface (OpenGL Y/UV textures, {}x{})", frame.width, frame.height);
        }
        m_useGpu = true;
        m_gpuW = frame.width;
        m_gpuH = frame.height;
        m_sourcePixmap = QPixmap();
        m_scaledPixmap = QPixmap();
        m_hasPreparedDraw = false;
        ensureGpuSurface();
        m_gpuSurface->setKeepAspectRatio(m_scaleMode == ScaleMode::KeepAspectRatio);
        m_gpuSurface->setGeometry(rect());
        m_gpuSurface->setNv12Frame(frame);
        m_gpuSurface->show();
        m_gpuSurface->raise();
        updateGpuMask();
        updateGeometry();
        update();
        return;
    }

    m_useGpu = false;
    if (m_gpuSurface)
    {
        m_gpuSurface->hide();
        m_gpuSurface->setNv12Frame(core::VideoFrameBuffer{});
    }

    if (frame.format == core::VideoPixelFormat::Rgb24 && !frame.isEmpty())
    {
        const int rowBytes = frame.strideY > 0 ? frame.strideY : frame.width * 3;
        const QImage image(
            reinterpret_cast<const uchar*>(frame.data.data()),
            frame.width,
            frame.height,
            rowBytes,
            QImage::Format_RGB888);
        setPixmap(QPixmap::fromImage(image));
    }
}

void Screen::clear()
{
    m_useGpu = false;
    m_gpuW = 0;
    m_gpuH = 0;
    if (m_gpuSurface)
    {
        m_gpuSurface->hide();
        m_gpuSurface->clearMask();
        m_gpuSurface->setNv12Frame(core::VideoFrameBuffer{});
    }
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
    updateGpuMask();
    update();
}

void Screen::setScaleMode(ScaleMode mode)
{
    if (m_scaleMode != mode)
    {
        m_scaleMode = mode;
        if (m_useGpu && m_gpuSurface)
            m_gpuSurface->setKeepAspectRatio(mode == ScaleMode::KeepAspectRatio);
        rebuildScaledPixmap();
        update();
    }
}

QSize Screen::contentSize() const
{
    if (m_useGpu && m_gpuW > 0 && m_gpuH > 0)
        return QSize(m_gpuW, m_gpuH);
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
    if (m_useGpu && m_gpuSurface)
    {
        m_gpuSurface->setGeometry(rect());
        updateGpuMask();
    }
    rebuildScaledPixmap();
}

void Screen::updateGpuMask()
{
    if (!m_gpuSurface)
        return;
    // Applying mask too early (before the underlying native widget exists) can
    // cause context/window recreation problems on some platforms.
    if (!m_gpuSurface->windowHandle() || !m_gpuSurface->isVisible())
        return;
    if (!m_roundedCornersEnabled)
    {
        if (m_hasGpuMask)
        {
            m_gpuSurface->clearMask();
            m_hasGpuMask = false;
            m_lastMaskSize = {};
        }
        return;
    }
    const QRect r = m_gpuSurface->rect();
    if (r.isEmpty())
        return;
    const QSize s = r.size();
    if (m_hasGpuMask && m_lastMaskSize == s)
        return;

    QPainterPath path;
    path.addRoundedRect(QRectF(r), m_cornerRadius, m_cornerRadius);
    m_gpuSurface->setMask(QRegion(path.toFillPolygon().toPolygon()));
    m_lastMaskSize = s;
    m_hasGpuMask = true;
}

void Screen::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    if (m_useGpu)
        return;

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
