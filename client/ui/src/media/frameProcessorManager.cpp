#include "frameProcessorManager.h"
#include "frameProcessor.h"
#include "utilities/logger.h"

#include <QMutexLocker>
#include <QThread>
#include <QVideoFrame>
#include <QPixmap>
#include <QMetaObject>

QMutex FrameProcessorManager::s_instanceMutex;

FrameProcessorManager& FrameProcessorManager::getInstance()
{
    QMutexLocker locker(&s_instanceMutex);
    static FrameProcessorManager instance;
    return instance;
}

FrameProcessorManager::FrameProcessorManager(QObject* parent)
    : QObject(parent)
    , m_frameProcessor(nullptr)
    , m_processingThread(new QThread(this))
{
    m_frameProcessor = new FrameProcessor();
    m_frameProcessor->moveToThread(m_processingThread);
    
    connect(m_frameProcessor, &FrameProcessor::cameraFrameProcessed,
            this, &FrameProcessorManager::onCameraFrameProcessed);

    connect(m_frameProcessor, &FrameProcessor::screenFrameProcessed,
            this, &FrameProcessorManager::onScreenFrameProcessed);

    connect(m_frameProcessor, &FrameProcessor::processingError,
            this, &FrameProcessorManager::onProcessingError);
    
    connect(m_processingThread, &QThread::finished, m_frameProcessor, &QObject::deleteLater);
    
    m_processingThread->start();
}

FrameProcessorManager::~FrameProcessorManager()
{
    if (m_processingThread && m_processingThread->isRunning()) {
        m_processingThread->quit();
        m_processingThread->wait();
    }
}

void FrameProcessorManager::processVideoFrame(const QVideoFrame& frame)
{
    QMutexLocker locker(&s_instanceMutex);
    if (m_frameProcessor) {
        // Вызываем через QMetaObject для выполнения в правильном потоке
        QMetaObject::invokeMethod(m_frameProcessor, "processVideoFrame", Qt::QueuedConnection,
                                 Q_ARG(QVideoFrame, frame));
    }
}

void FrameProcessorManager::processPixmap(const QPixmap& pixmap)
{
    QMutexLocker locker(&s_instanceMutex);
    if (m_frameProcessor) {
        // Вызываем через QMetaObject для выполнения в правильном потоке
        QMetaObject::invokeMethod(m_frameProcessor, "processPixmap", Qt::QueuedConnection,
                                 Q_ARG(QPixmap, pixmap));
    }
}

FrameProcessor* FrameProcessorManager::getFrameProcessor()
{
    QMutexLocker locker(&s_instanceMutex);
    return m_frameProcessor;
}

void FrameProcessorManager::onCameraFrameProcessed(const QPixmap& pixmap, const std::vector<unsigned char>& imageData)
{
    emit cameraFrameProcessed(pixmap, imageData);
}

void FrameProcessorManager::onScreenFrameProcessed(const QPixmap& pixmap, const std::vector<unsigned char>& imageData)
{
    emit screenFrameProcessed(pixmap, imageData);
}

void FrameProcessorManager::onProcessingError(const QString& errorMessage)
{
    emit processingError(errorMessage);
}