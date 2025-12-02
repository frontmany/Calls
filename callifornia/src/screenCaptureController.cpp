#include "screenCaptureController.h"

#include <QBuffer>
#include <QGuiApplication>
#include <QScreen>
#include <QTimer>
#include <QtGlobal>
#include <QImage>
#include <QPainter>
#include <algorithm>

ScreenCaptureController::ScreenCaptureController(QObject* parent)
    : QObject(parent),
    m_captureTimer(new QTimer(this)),
    m_isCapturing(false),
    m_selectedScreenIndex(-1)
{
    connect(m_captureTimer, &QTimer::timeout, this, &ScreenCaptureController::captureScreen);
    refreshAvailableScreens();
}

ScreenCaptureController::~ScreenCaptureController()
{
    stopCapture();
}

void ScreenCaptureController::startCapture()
{
    if (m_isCapturing || m_selectedScreenIndex == -1)
    {
        return;
    }

    m_isCapturing = true;
    m_captureTimer->start(100);

    emit captureStarted();
}

void ScreenCaptureController::stopCapture()
{
    if (!m_isCapturing)
    {
        return;
    }

    m_isCapturing = false;
    m_captureTimer->stop();

    emit captureStopped();
}

void ScreenCaptureController::captureScreen()
{
    if (m_selectedScreenIndex == -1 || m_selectedScreenIndex >= m_availableScreens.size())
        return;
    
    QScreen* screen = m_availableScreens[m_selectedScreenIndex];
    if (!screen) return;
    
    QPixmap screenshot = screen->grabWindow(0);

    std::vector<unsigned char> imageData;

    if (!screenshot.isNull())
    {
        QPixmap croppedScreenshot = cropToHorizontal(screenshot);
        
        bool isVertical = screenshot.height() > screenshot.width();
        QSize targetSize = QSize(1600, 900);
        
        imageData = pixmapToBytes(croppedScreenshot, targetSize);

        emit screenCaptured(croppedScreenshot, imageData);
    }
}

void ScreenCaptureController::refreshAvailableScreens()
{
    m_availableScreens = QGuiApplication::screens();
    if (m_selectedScreenIndex >= m_availableScreens.size())
    {
        m_selectedScreenIndex = -1;
    }
}

const QList<QScreen*>& ScreenCaptureController::availableScreens() const
{
    return m_availableScreens;
}

void ScreenCaptureController::setSelectedScreenIndex(int screenIndex)
{
    if (screenIndex >= 0 && screenIndex < m_availableScreens.size())
    {
        m_selectedScreenIndex = screenIndex;
    }
    else
    {
        m_selectedScreenIndex = -1;
    }
}

int ScreenCaptureController::selectedScreenIndex() const
{
    return m_selectedScreenIndex;
}

void ScreenCaptureController::resetSelectedScreenIndex()
{
    m_selectedScreenIndex = -1;
}

bool ScreenCaptureController::isCapturing() const
{
    return m_isCapturing;
}

QPixmap ScreenCaptureController::cropToHorizontal(const QPixmap& pixmap)
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

std::vector<unsigned char> ScreenCaptureController::pixmapToBytes(const QPixmap& pixmap, QSize targetSize)
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