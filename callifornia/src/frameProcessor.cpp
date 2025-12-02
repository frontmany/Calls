#include "frameProcessor.h"

#include <QVideoFrame>
#include <QImage>
#include <QPainter>
#include <QBuffer>

FrameProcessor::FrameProcessor(QObject* parent)
    : QObject(parent)
{
}

void FrameProcessor::processVideoFrame(const QVideoFrame& frame)
{
    QVideoFrame clonedFrame = frame;
    if (!clonedFrame.map(QVideoFrame::ReadOnly)) return;
    
    QPixmap pixmap;
    try
    {
        pixmap = videoFrameToPixmap(clonedFrame);
    }
    catch (const std::exception& e)
    {
        clonedFrame.unmap();
        emit processingError(QString("Failed to convert video frame: %1").arg(e.what()));
        return;
    }

    clonedFrame.unmap();

    if (!pixmap.isNull() && pixmap.width() > 0 && pixmap.height() > 0)
    {
        try
        {
            QPixmap croppedPixmap = cropToHorizontal(pixmap);

            if (!croppedPixmap.isNull() && croppedPixmap.width() > 0 && croppedPixmap.height() > 0)
            {
                QSize targetSize = QSize(1280, 720);
                std::vector<unsigned char> imageData = pixmapToBytes(croppedPixmap, targetSize);

                emit frameProcessed(croppedPixmap, imageData);
            }
        }
        catch (const std::exception& e)
        {
            emit processingError(QString("Failed to process video frame: %1").arg(e.what()));
        }
    }
}

void FrameProcessor::processPixmap(const QPixmap& pixmap, const QSize& targetSize)
{
    if (pixmap.isNull())
    {
        return;
    }

    try
    {
        QPixmap croppedPixmap = cropToHorizontal(pixmap);
        
        if (!croppedPixmap.isNull() && croppedPixmap.width() > 0 && croppedPixmap.height() > 0)
        {
            std::vector<unsigned char> imageData = pixmapToBytes(croppedPixmap, targetSize);
            emit frameProcessed(croppedPixmap, imageData);
        }
    }
    catch (const std::exception& e)
    {
        emit processingError(QString("Failed to process pixmap: %1").arg(e.what()));
    }
}

QPixmap FrameProcessor::videoFrameToPixmap(const QVideoFrame& frame)
{
    QImage image = frame.toImage();

    if (!image.isNull())
    {
        return QPixmap::fromImage(image);
    }

    QVideoFrame localFrame = frame;
    if (!localFrame.map(QVideoFrame::ReadOnly))
    {
        return QPixmap();
    }

    QImage convertedImage;
    QImage::Format format = QVideoFrameFormat::imageFormatFromPixelFormat(localFrame.pixelFormat());
    if (format == QImage::Format_Invalid)
    {
        format = QImage::Format_RGB32;
    }

    convertedImage = QImage(localFrame.bits(0),
        localFrame.width(),
        localFrame.height(),
        localFrame.bytesPerLine(0),
        format);

    localFrame.unmap();

    return QPixmap::fromImage(convertedImage);
}

QPixmap FrameProcessor::cropToHorizontal(const QPixmap& pixmap)
{
    if (pixmap.isNull()) return pixmap;

    int w = pixmap.width();
    int h = pixmap.height();

    if (h <= w)
    {
        return pixmap;
    }

    int targetH = static_cast<int>(w * 9.0 / 16.0);
    targetH = qMin(targetH, h);
    QRect cropRect(0, 0, w, targetH);
    return pixmap.copy(cropRect);
}

std::vector<unsigned char> FrameProcessor::pixmapToBytes(const QPixmap& pixmap, QSize targetSize)
{
    QImage image = pixmap.toImage();

    if (image.format() != QImage::Format_RGB32 && image.format() != QImage::Format_ARGB32)
    {
        image = image.convertToFormat(QImage::Format_RGB32);
    }

    QImage scaledImage = image.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);

    scaledImage.save(&buffer, "JPG", 50);

    return std::vector<unsigned char>(byteArray.begin(), byteArray.end());
}

