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
    void processVideoFrame(const QVideoFrame& frame);  // Камера - высокая скорость
    void processPixmap(const QPixmap& pixmap);          // Экран - высокое качество

public:
    void processScreenFrame(const QPixmap& pixmap);      // Экран - высокое качество
    void processCameraFrame(const QVideoFrame& frame);   // Камера - высокая скорость

public:
    void drawCursorOnPixmap(QPixmap& pixmap, const QPoint& cursorPos, const QRect& screenGeometry);

signals:
    void frameProcessed(const QPixmap& pixmap, const std::vector<unsigned char>& imageData);
    void processingError(const QString& errorMessage);

private:
    QPixmap videoFrameToPixmap(const QVideoFrame& frame);
    QPixmap cropToHorizontal(const QPixmap& pixmap);
    std::vector<unsigned char> pixmapToH264(const QPixmap& pixmap, QSize targetSize, bool isScreen);
    
    H264Encoder* m_screenEncoder;  // Для экрана - высокое качество
    H264Encoder* m_cameraEncoder;  // Для камеры - высокая скорость
};

