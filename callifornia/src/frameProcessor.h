#pragma once

#include <vector>

#include <QObject>
#include <QPixmap>
#include <QVideoFrame>
#include <QSize>

class FrameProcessor : public QObject
{
    Q_OBJECT

public:
    explicit FrameProcessor(QObject* parent = nullptr);

public slots:
    void processVideoFrame(const QVideoFrame& frame);
    void processPixmap(const QPixmap& pixmap, const QSize& targetSize);

signals:
    void frameProcessed(const QPixmap& pixmap, const std::vector<unsigned char>& imageData);
    void processingError(const QString& errorMessage);

private:
    QPixmap videoFrameToPixmap(const QVideoFrame& frame);
    QPixmap cropToHorizontal(const QPixmap& pixmap);
    std::vector<unsigned char> pixmapToBytes(const QPixmap& pixmap, QSize targetSize);
};

