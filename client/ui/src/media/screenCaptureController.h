#pragma once

#include <vector>

#include <QObject>
#include <QTimer>
#include <QPixmap>
#include <QScreen>
#include <QList>
#include <QSize>
#include <QThread>

class FrameProcessorManager;

class ScreenCaptureController : public QObject
{
    Q_OBJECT

public:
    explicit ScreenCaptureController(QObject* parent);
    ~ScreenCaptureController();

    void startCapture();
    void stopCapture();
    void refreshAvailableScreens();
    void setSelectedScreenIndex(int screenIndex);
    void resetSelectedScreenIndex();

    const QList<QScreen*>& availableScreens() const;
    int selectedScreenIndex() const;
    bool isCapturing() const;

signals:
    void screenCaptured(const QPixmap& pixmap, const std::vector<unsigned char>& imageData);
    void captureStarted();
    void captureStopped();
    void pixmapReadyForProcessing(const QPixmap& pixmap);

private slots:
    void captureScreen();
    void onFrameProcessed(const QPixmap& pixmap, const std::vector<unsigned char>& imageData);
    void onProcessingError(const QString& errorMessage);

private:
    QTimer* m_captureTimer;
    bool m_isCapturing;
    int m_pendingFrames;
    int m_maxQueuedFrames;
    QList<QScreen*> m_availableScreens;
    int m_selectedScreenIndex;

    FrameProcessorManager* m_frameProcessorManager;
};
