#include "screen.h"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QImage>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace
{
    inline uint32_t readUint32(const unsigned char* data)
    {
        return static_cast<uint32_t>(data[0])
            | (static_cast<uint32_t>(data[1]) << 8)
            | (static_cast<uint32_t>(data[2]) << 16)
            | (static_cast<uint32_t>(data[3]) << 24);
    }
}

Screen::Screen(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
}

void Screen::setFrame(const QPixmap& frame)
{
    if (frame.isNull())
    {
        return;
    }

    QImage image = frame.toImage();
    if (image.isNull())
    {
        return;
    }

    if (image.format() != QImage::Format_RGBA8888)
    {
        image = image.convertToFormat(QImage::Format_RGBA8888);
    }

    m_currentFrame = image;
    update();
}

void Screen::clearFrame()
{
    m_currentFrame = QImage();
    update();
}

void Screen::applyDiff(const ScreenDiffData& diffData)
{
    if (diffData.frame_id == 0 || diffData.block_size == 0)
    {
        return;
    }

    std::vector<unsigned char> tempBuffer;
    const std::vector<unsigned char>* bufferPtr = &diffData.serialized_data;
    if (bufferPtr->empty())
    {
        tempBuffer = serializeScreenDiffData(diffData);
        bufferPtr = &tempBuffer;
    }

    const std::vector<unsigned char>& buffer = *bufferPtr;
    if (buffer.size() < kScreenDiffHeaderSize || diffData.screen_width == 0 || diffData.screen_height == 0)
    {
        return;
    }

    const QSize frameSize(diffData.screen_width, diffData.screen_height);
    ensureFrameSize(frameSize);
    if (m_currentFrame.isNull())
    {
        return;
    }

    if (diffData.is_full_frame)
    {
        m_currentFrame.fill(Qt::black);
    }

    const int columns = (frameSize.width() + diffData.block_size - 1) / diffData.block_size;
    if (columns <= 0)
    {
        return;
    }

    std::vector<QRect> updatedRects;
    updatedRects.reserve(std::max<uint16_t>(1, diffData.changed_blocks_count));

    std::size_t offset = kScreenDiffHeaderSize;
    const std::size_t bufferSize = buffer.size();

    for (uint16_t i = 0; i < diffData.changed_blocks_count; ++i)
    {
        if (offset + 8 > bufferSize)
        {
            break;
        }

        uint32_t blockIndex = readUint32(buffer.data() + offset);
        uint32_t dataSize = readUint32(buffer.data() + offset + 4);
        offset += 8;

        if (offset + dataSize > bufferSize)
        {
            break;
        }

        const int column = static_cast<int>(blockIndex % static_cast<uint32_t>(columns));
        const int row = static_cast<int>(blockIndex / static_cast<uint32_t>(columns));
        const int x = column * diffData.block_size;
        const int y = row * diffData.block_size;
        const int width = std::min<int>(diffData.block_size, frameSize.width() - x);
        const int height = std::min<int>(diffData.block_size, frameSize.height() - y);

        if (width <= 0 || height <= 0)
        {
            offset += dataSize;
            continue;
        }

        QRect targetRect(x, y, width, height);

        QImage blockImage = QImage::fromData(buffer.data() + offset, static_cast<int>(dataSize), "JPG");
        offset += dataSize;

        if (blockImage.isNull())
        {
            continue;
        }

        if (blockImage.size() != targetRect.size())
        {
            blockImage = blockImage.scaled(targetRect.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        }

        if (blockImage.format() != QImage::Format_RGBA8888)
        {
            blockImage = blockImage.convertToFormat(QImage::Format_RGBA8888);
        }

        for (int rowIndex = 0; rowIndex < targetRect.height(); ++rowIndex)
        {
            const uchar* src = blockImage.constScanLine(rowIndex);
            uchar* dst = m_currentFrame.scanLine(targetRect.y() + rowIndex) + targetRect.x() * 4;
            std::memcpy(dst, src, static_cast<std::size_t>(targetRect.width()) * 4);
        }

        updatedRects.push_back(targetRect);
    }

    if (diffData.is_full_frame || updatedRects.empty())
    {
        update();
    }
    else
    {
        bool anyUpdate = false;
        for (const QRect& rect : updatedRects)
        {
            QRect widgetRect = mapFrameRectToWidget(rect);
            if (!widgetRect.isEmpty())
            {
                update(widgetRect);
                anyUpdate = true;
            }
        }
        if (!anyUpdate)
        {
            update();
        }
    }
}

void Screen::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setPen(Qt::NoPen);

    const QRect rect = this->rect();
    if (rect.isEmpty())
    {
        return;
    }

    QPainterPath clipPath;
    clipPath.addRoundedRect(rect, m_cornerRadius, m_cornerRadius);
    painter.setClipPath(clipPath);

    painter.fillRect(rect, palette().window());

    if (m_currentFrame.isNull())
    {
        return;
    }

    const QSizeF frameSize = m_currentFrame.size();
    if (frameSize.isEmpty())
    {
        return;
    }

    const QSizeF widgetSize = rect.size();
    const qreal scaleFactor = std::max(widgetSize.width() / frameSize.width(), widgetSize.height() / frameSize.height());
    const QSizeF scaledSize(frameSize.width() * scaleFactor, frameSize.height() * scaleFactor);
    const QPointF offset((widgetSize.width() - scaledSize.width()) / 2.0, (widgetSize.height() - scaledSize.height()) / 2.0);

    QRectF targetRect(rect.topLeft() + offset, scaledSize);
    painter.drawImage(targetRect, m_currentFrame);
}

void Screen::ensureFrameSize(const QSize& frameSize)
{
    if (frameSize.isEmpty())
    {
        m_currentFrame = QImage();
        return;
    }

    if (m_currentFrame.size() != frameSize)
    {
        m_currentFrame = QImage(frameSize, QImage::Format_RGBA8888);
        m_currentFrame.fill(Qt::black);
    }
    else if (m_currentFrame.format() != QImage::Format_RGBA8888)
    {
        m_currentFrame = m_currentFrame.convertToFormat(QImage::Format_RGBA8888);
    }
}

QRect Screen::mapFrameRectToWidget(const QRect& frameRect) const
{
    if (m_currentFrame.isNull() || frameRect.isEmpty())
    {
        return QRect();
    }

    const QRect widgetRect = this->rect();
    if (widgetRect.isEmpty())
    {
        return QRect();
    }

    const QSizeF frameSize = m_currentFrame.size();
    const QSizeF widgetSize = widgetRect.size();
    const qreal scaleFactor = std::max(widgetSize.width() / frameSize.width(), widgetSize.height() / frameSize.height());
    const QSizeF scaledSize(frameSize.width() * scaleFactor, frameSize.height() * scaleFactor);
    const QPointF offset((widgetSize.width() - scaledSize.width()) / 2.0, (widgetSize.height() - scaledSize.height()) / 2.0);

    const qreal x1 = frameRect.x() * scaleFactor + offset.x() + widgetRect.left();
    const qreal y1 = frameRect.y() * scaleFactor + offset.y() + widgetRect.top();
    const qreal x2 = (frameRect.x() + frameRect.width()) * scaleFactor + offset.x() + widgetRect.left();
    const qreal y2 = (frameRect.y() + frameRect.height()) * scaleFactor + offset.y() + widgetRect.top();

    QRect mappedRect(
        QPoint(static_cast<int>(std::floor(x1)), static_cast<int>(std::floor(y1))),
        QPoint(static_cast<int>(std::ceil(x2)), static_cast<int>(std::ceil(y2))));

    return mappedRect.intersected(widgetRect);
}

