#include "screenCaptureController.h"
#include "frameProcessor.h"

#include <QBuffer>
#include <QGuiApplication>
#include <QScreen>
#include <QTimer>
#include <QtGlobal>
#include <QImage>
#include <QPainter>
#include <QCursor>
#include <algorithm>
#include <cstring>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

ScreenCaptureController::ScreenCaptureController(QObject* parent)
    : QObject(parent),
    m_captureTimer(new QTimer(this)),
    m_isCapturing(false),
    m_selectedScreenIndex(-1),
    m_processingThread(new QThread(this)),
    m_frameProcessor(new FrameProcessor())
{
    m_frameProcessor->moveToThread(m_processingThread);
    
    connect(m_captureTimer, &QTimer::timeout, this, &ScreenCaptureController::captureScreen);
    connect(this, &ScreenCaptureController::pixmapReadyForProcessing, m_frameProcessor, &FrameProcessor::processPixmap);
    connect(m_frameProcessor, &FrameProcessor::frameProcessed, this, &ScreenCaptureController::onFrameProcessed);
    connect(m_frameProcessor, &FrameProcessor::processingError, this, &ScreenCaptureController::onProcessingError);
    
    connect(m_processingThread, &QThread::finished, m_frameProcessor, &QObject::deleteLater);
    
    refreshAvailableScreens();
    m_processingThread->start();
}

ScreenCaptureController::~ScreenCaptureController()
{
    stopCapture();
    
    m_processingThread->quit();
    m_processingThread->wait();
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
    if (!m_isCapturing)
    {
        return;
    }

    if (m_selectedScreenIndex == -1 || m_selectedScreenIndex >= m_availableScreens.size())
        return;
    
    QScreen* screen = m_availableScreens[m_selectedScreenIndex];
    if (!screen) return;
    
    QPixmap screenshot = screen->grabWindow(0);

    if (!screenshot.isNull())
    {
        QPoint globalCursorPos = QCursor::pos();
        QRect screenGeometry = screen->geometry();
        
        if (screenGeometry.contains(globalCursorPos))
        {
            QPoint screenRelativePos = globalCursorPos - screenGeometry.topLeft();
            QPixmap cursorPixmap;
            QPoint hotspot(0, 0);
            
#ifdef Q_OS_WIN
            CURSORINFO cursorInfo = { 0 };
            cursorInfo.cbSize = sizeof(CURSORINFO);
            if (GetCursorInfo(&cursorInfo) && cursorInfo.flags == CURSOR_SHOWING)
            {
                ICONINFO iconInfo;
                if (GetIconInfo(cursorInfo.hCursor, &iconInfo))
                {
                    hotspot = QPoint(iconInfo.xHotspot, iconInfo.yHotspot);
                    
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
                            
                            for (int i = 0; i < cursorWidth * cursorHeight; ++i)
                            {
                                reinterpret_cast<uint32_t*>(bitsWhite)[i] = whiteColor;
                                reinterpret_cast<uint32_t*>(bitsBlack)[i] = blackColor;
                            }
                            
                            HGDIOBJ oldObj = SelectObject(hdcMem, hbitmapWhite);
                            DrawIconEx(hdcMem, 0, 0, cursorInfo.hCursor, 0, 0, 0, NULL, DI_NORMAL);
                            GdiFlush();
                            SelectObject(hdcMem, hbitmapBlack);
                            DrawIconEx(hdcMem, 0, 0, cursorInfo.hCursor, 0, 0, 0, NULL, DI_NORMAL);
                            GdiFlush();
                            
                            uint32_t* whiteBits = reinterpret_cast<uint32_t*>(bitsWhite);
                            uint32_t* blackBits = reinterpret_cast<uint32_t*>(bitsBlack);
                            
                            for (int i = 0; i < cursorWidth * cursorHeight; ++i)
                            {
                                uint32_t whiteBgra = whiteBits[i];
                                uint32_t blackBgra = blackBits[i];
                                
                                uint8_t whiteB = (whiteBgra >> 0) & 0xFF;
                                uint8_t whiteG = (whiteBgra >> 8) & 0xFF;
                                uint8_t whiteR = (whiteBgra >> 16) & 0xFF;
                                
                                uint8_t blackB = (blackBgra >> 0) & 0xFF;
                                uint8_t blackG = (blackBgra >> 8) & 0xFF;
                                uint8_t blackR = (blackBgra >> 16) & 0xFF;
                                
                                int x = i % cursorWidth;
                                int y = i / cursorWidth;
                                
                                bool visibleOnWhite = (whiteR != 255 || whiteG != 255 || whiteB != 255);
                                bool visibleOnBlack = (blackR != 0 || blackG != 0 || blackB != 0);
                                
                                if (visibleOnBlack && visibleOnWhite)
                                {
                                    cursorImage.setPixel(x, y, qRgba(blackR, blackG, blackB, 255));
                                }
                                else if (visibleOnBlack)
                                {
                                    cursorImage.setPixel(x, y, qRgba(blackR, blackG, blackB, 255));
                                }
                                else if (visibleOnWhite)
                                {
                                    cursorImage.setPixel(x, y, qRgba(whiteR, whiteG, whiteB, 255));
                                }
                            }
                            
                            SelectObject(hdcMem, oldObj);
                            DeleteObject(hbitmapWhite);
                            DeleteObject(hbitmapBlack);
                        }
                        
                        DeleteDC(hdcMem);
                    }
                    
                    ReleaseDC(NULL, hdcScreen);
                    
                    cursorPixmap = QPixmap::fromImage(cursorImage);
                    
                    if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);
                    if (iconInfo.hbmColor) DeleteObject(iconInfo.hbmColor);
                }
            }
#else
            QCursor cursor;
            cursorPixmap = cursor.pixmap();
            if (!cursorPixmap.isNull())
            {
                hotspot = cursor.hotSpot();
            }
#endif
            
            if (!cursorPixmap.isNull())
            {
                QPainter painter(&screenshot);
                painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
                painter.drawPixmap(
                    screenRelativePos.x() - hotspot.x(),
                    screenRelativePos.y() - hotspot.y(),
                    cursorPixmap
                );
            }
        }
        
        emit pixmapReadyForProcessing(screenshot);
    }
}

void ScreenCaptureController::onFrameProcessed(const QPixmap& pixmap, const std::vector<unsigned char>& imageData)
{
    if (!m_isCapturing)
    {
        return;
    }

    emit screenCaptured(pixmap, imageData);
}

void ScreenCaptureController::onProcessingError(const QString& errorMessage)
{
    qWarning() << "Screen processing error:" << errorMessage;
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