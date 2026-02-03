#pragma once

#include <vector>

#include <QObject>
#include <QPixmap>
#include <QVideoFrame>
#include <QSize>

class H264Encoder;

class FrameProcessor : public QObject
{
    Q_OBJECT

public:
    explicit FrameProcessor(QObject* parent = nullptr);
    ~FrameProcessor();

public slots:
    void processVideoFrame(const QVideoFrame& frame); 
    void processPixmap(const QPixmap& pixmap);       

public:
    void processScreenFrame(const QPixmap& pixmap); 
    void processCameraFrame(const QVideoFrame& frame);

public:
    void drawCursorOnPixmap(QPixmap& pixmap, const QPoint& cursorPos, const QRect& screenGeometry);

signals:
    void screenFrameProcessed(const QPixmap& pixmap, const std::vector<unsigned char>& imageData);
    void cameraFrameProcessed(const QPixmap& pixmap, const std::vector<unsigned char>& imageData);
    void processingError(const QString& errorMessage);

private:
    QPixmap videoFrameToPixmap(const QVideoFrame& frame);
    QPixmap cropToHorizontal(const QPixmap& pixmap);
    std::vector<unsigned char> pixmapToH264(const QPixmap& pixmap, QSize targetSize, bool isScreen);
    
    void updateBitratesForSimultaneousStreaming();
    
    H264Encoder* m_screenEncoder;
    H264Encoder* m_cameraEncoder;
    
    bool m_isCameraActive;
    bool m_isScreenActive;
    
    // Memory optimization - reusable buffers
    QImage m_reusableImage;
    std::vector<unsigned char> m_reusableBuffer;
    QPixmap m_reusablePixmap;
    QSize m_lastCameraSize;
    QSize m_lastScreenSize;
};

