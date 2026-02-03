#pragma once

#include <QObject>
#include <QMutex>
#include <QThread>
#include <QVideoFrame>
#include <QPixmap>
#include <vector>
#include <memory>

class FrameProcessor;

class FrameProcessorManager : public QObject
{
    Q_OBJECT

public:
    static FrameProcessorManager& getInstance();
    ~FrameProcessorManager();

    FrameProcessorManager(const FrameProcessorManager&) = delete;
    FrameProcessorManager& operator=(const FrameProcessorManager&) = delete;
    
    FrameProcessor* getFrameProcessor();
    
    void processVideoFrame(const QVideoFrame& frame);  
    void processPixmap(const QPixmap& pixmap);         

signals:
    void cameraFrameProcessed(const QPixmap& pixmap, const std::vector<unsigned char>& imageData);
    void screenFrameProcessed(const QPixmap& pixmap, const std::vector<unsigned char>& imageData);
    void processingError(const QString& errorMessage);

private slots:
    void onCameraFrameProcessed(const QPixmap& pixmap, const std::vector<unsigned char>& imageData);
    void onScreenFrameProcessed(const QPixmap& pixmap, const std::vector<unsigned char>& imageData);
    void onProcessingError(const QString& errorMessage);

private:
    FrameProcessorManager(QObject* parent = nullptr);
    
    FrameProcessor* m_frameProcessor;
    QThread* m_processingThread;
    static QMutex s_instanceMutex;
};
