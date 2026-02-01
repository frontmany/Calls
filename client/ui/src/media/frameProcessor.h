#pragma once

#include <vector>

#include <QObject>
#include <QPixmap>
#include <QVideoFrame>
#include <QSize>

class AV1Encoder;

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
    void drawCursorOnPixmap(QPixmap& pixmap, const QPoint& cursorPos, const QRect& screenGeometry);

signals:
    void frameProcessed(const QPixmap& pixmap, const std::vector<unsigned char>& imageData);
    void processingError(const QString& errorMessage);

private:
    QPixmap videoFrameToPixmap(const QVideoFrame& frame);
    QPixmap cropToHorizontal(const QPixmap& pixmap);
    std::vector<unsigned char> pixmapToAV1(const QPixmap& pixmap, QSize targetSize);
    
    AV1Encoder* m_av1Encoder;
};

