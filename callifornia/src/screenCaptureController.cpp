#include "screenCaptureController.h"
#include "frameProcessor.h"

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