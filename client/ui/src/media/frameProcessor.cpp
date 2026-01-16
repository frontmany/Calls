#include "frameProcessor.h"

#include <QVideoFrame>
#include <QImage>
#include <QPainter>
#include <QBuffer>
#include <QCursor>
#include <QtGlobal>

#ifdef Q_OS_WIN
#include <windows.h>
#include <cstring>
#endif

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
                QSize targetSize = QSize(1920, 1080);
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

void FrameProcessor::processPixmap(const QPixmap& pixmap)
{
    if (pixmap.isNull())
    {
        return;
    }

    try
    {
        if (pixmap.width() > 0 && pixmap.height() > 0)
        {
            QSize targetSize = QSize(1920, 1080);
            std::vector<unsigned char> imageData = pixmapToBytes(pixmap, targetSize);
            emit frameProcessed(pixmap, imageData);
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

    const double targetAspectRatio = 16.0 / 9.0;
    const double currentAspectRatio = static_cast<double>(w) / h;

    if (qAbs(currentAspectRatio - targetAspectRatio) < 0.01)
    {
        return pixmap;
    }

    int cropW = w;
    int cropH = h;

    if (currentAspectRatio > targetAspectRatio)
    {
        cropW = static_cast<int>(h * targetAspectRatio);
        QRect cropRect(0, 0, cropW, cropH);
        return pixmap.copy(cropRect);
    }
    else
    {
        cropH = static_cast<int>(w / targetAspectRatio);
        QRect cropRect(0, 0, cropW, cropH);
        return pixmap.copy(cropRect);
    }
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

    scaledImage.save(&buffer, "JPG", 60);

    return std::vector<unsigned char>(byteArray.begin(), byteArray.end());
}

void FrameProcessor::drawCursorOnPixmap(QPixmap& pixmap, const QPoint& cursorPos, const QRect& screenGeometry)
{
    if (pixmap.isNull() || !screenGeometry.contains(cursorPos))
    {
        return;
    }

    QPoint screenRelativePos = cursorPos - screenGeometry.topLeft();
    
#ifdef Q_OS_WIN
    CURSORINFO cursorInfo = { 0 };
    cursorInfo.cbSize = sizeof(CURSORINFO);
    if (!GetCursorInfo(&cursorInfo) || cursorInfo.flags != CURSOR_SHOWING)
    {
        return;
    }

    ICONINFO iconInfo;
    if (!GetIconInfo(cursorInfo.hCursor, &iconInfo))
    {
        return;
    }

    QPoint hotspot(iconInfo.xHotspot, iconInfo.yHotspot);
    
    int cursorWidth = GetSystemMetrics(SM_CXCURSOR);
    int cursorHeight = GetSystemMetrics(SM_CYCURSOR);
    
    if (cursorWidth == 0) cursorWidth = 32;
    if (cursorHeight == 0) cursorHeight = 32;
    
    if (iconInfo.hbmColor)
    {
        BITMAP bmp;
        GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bmp);
        if (bmp.bmWidth > 0 && bmp.bmHeight > 0)
        {
            cursorWidth = bmp.bmWidth;
            cursorHeight = bmp.bmHeight;
        }
    }
    else if (iconInfo.hbmMask)
    {
        BITMAP bmp;
        GetObject(iconInfo.hbmMask, sizeof(BITMAP), &bmp);
        if (bmp.bmWidth > 0 && bmp.bmHeight > 1)
        {
            cursorWidth = bmp.bmWidth;
            cursorHeight = bmp.bmHeight / 2;
        }
    }
    
    QImage cursorImage(cursorWidth, cursorHeight, QImage::Format_ARGB32);
    cursorImage.fill(Qt::transparent);
    
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    
    if (hdcMem)
    {
        BITMAPINFO bmi = { 0 };
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = cursorWidth;
        bmi.bmiHeader.biHeight = -cursorHeight;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        
        void* bitsWhite = nullptr;
        void* bitsBlack = nullptr;
        HBITMAP hbitmapWhite = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &bitsWhite, NULL, 0);
        HBITMAP hbitmapBlack = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &bitsBlack, NULL, 0);
        
        if (hbitmapWhite && hbitmapBlack)
        {
            uint32_t whiteColor = 0xFFFFFFFF;
            uint32_t blackColor = 0xFF000000;
            uint32_t* whiteBits = reinterpret_cast<uint32_t*>(bitsWhite);
            uint32_t* blackBits = reinterpret_cast<uint32_t*>(bitsBlack);
            const int pixelCount = cursorWidth * cursorHeight;
            
            for (int i = 0; i < pixelCount; ++i)
            {
                whiteBits[i] = whiteColor;
                blackBits[i] = blackColor;
            }
            
            HGDIOBJ oldObj = SelectObject(hdcMem, hbitmapWhite);
            DrawIconEx(hdcMem, 0, 0, cursorInfo.hCursor, 0, 0, 0, NULL, DI_NORMAL);
            GdiFlush();
            SelectObject(hdcMem, hbitmapBlack);
            DrawIconEx(hdcMem, 0, 0, cursorInfo.hCursor, 0, 0, 0, NULL, DI_NORMAL);
            GdiFlush();
            
            uint32_t* cursorBits = reinterpret_cast<uint32_t*>(cursorImage.bits());
            
            for (int i = 0; i < pixelCount; ++i)
            {
                uint32_t whiteBgra = whiteBits[i];
                uint32_t blackBgra = blackBits[i];
                
                uint8_t whiteB = (whiteBgra >> 0) & 0xFF;
                uint8_t whiteG = (whiteBgra >> 8) & 0xFF;
                uint8_t whiteR = (whiteBgra >> 16) & 0xFF;
                
                uint8_t blackB = (blackBgra >> 0) & 0xFF;
                uint8_t blackG = (blackBgra >> 8) & 0xFF;
                uint8_t blackR = (blackBgra >> 16) & 0xFF;
                
                bool visibleOnWhite = (whiteR != 255 || whiteG != 255 || whiteB != 255);
                bool visibleOnBlack = (blackR != 0 || blackG != 0 || blackB != 0);
                
                if (visibleOnBlack && visibleOnWhite)
                {
                    cursorBits[i] = qRgba(blackR, blackG, blackB, 255);
                }
                else if (visibleOnBlack)
                {
                    cursorBits[i] = qRgba(blackR, blackG, blackB, 255);
                }
                else if (visibleOnWhite)
                {
                    cursorBits[i] = qRgba(whiteR, whiteG, whiteB, 255);
                }
            }
            
            SelectObject(hdcMem, oldObj);
            DeleteObject(hbitmapWhite);
            DeleteObject(hbitmapBlack);
        }
        
        DeleteDC(hdcMem);
    }
    
    ReleaseDC(NULL, hdcScreen);
    
    if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);
    if (iconInfo.hbmColor) DeleteObject(iconInfo.hbmColor);
    
    if (!cursorImage.isNull())
    {
        QPainter painter(&pixmap);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.drawImage(
            screenRelativePos.x() - hotspot.x(),
            screenRelativePos.y() - hotspot.y(),
            cursorImage
        );
    }
#else
    QCursor cursor;
    QPixmap cursorPixmap = cursor.pixmap();
    if (!cursorPixmap.isNull())
    {
        QPoint hotspot = cursor.hotSpot();
        QPainter painter(&pixmap);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.drawPixmap(
            screenRelativePos.x() - hotspot.x(),
            screenRelativePos.y() - hotspot.y(),
            cursorPixmap
        );
    }
#endif
}

