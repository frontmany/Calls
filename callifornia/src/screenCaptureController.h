#pragma once

#include <vector>

#include <QObject>
#include <QTimer>
#include <QPixmap>
#include <QScreen>
#include <QList>
#include <QSize>

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

private slots:
    void captureScreen();

private:
    QPixmap cropToHorizontal(const QPixmap& pixmap);
    std::vector<unsigned char> pixmapToBytes(const QPixmap& pixmap, QSize targetSize);

private:
    QTimer* m_captureTimer;
    bool m_isCapturing;
    QList<QScreen*> m_availableScreens;
    int m_selectedScreenIndex;
};