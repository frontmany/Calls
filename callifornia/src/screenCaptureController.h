#pragma once

#include <QObject>
#include <QTimer>
#include <QPixmap>
#include <QScreen>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QGridLayout>
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>

class OverlayWidget;
class ScreenPreviewWidget;

class ScreenCaptureController : public QObject
{
    Q_OBJECT

public:
    explicit ScreenCaptureController(QWidget* parent);
    ~ScreenCaptureController();

    void showCaptureDialog();
    void hideCaptureDialog();

    void startCapture();
    void stopCapture();

    // Controls whether to encode captured frames to JPEG and emit as imageData.
    // Disable for local preview to avoid heavy per-frame encoding.
    void setEncodeImageData(bool enable);

signals:
    void captureStarted();
    void captureStopped();
    void screenCaptured(const QPixmap& pixmap, const std::string& imageData);

private slots:
    void captureScreen();
    void onShareButtonClicked();
    void onScreenSelected(int screenIndex, bool currentlySelected);

private:
    QWidget* createCaptureDialog(OverlayWidget* overlay);
    void refreshScreensPreview();
    std::string pixmapToString(const QPixmap& pixmap);
    void updateSelectedScreen();
    void closeCaptureUIOnly();

private:
    QWidget* m_parent;
    bool m_encodeImageData = false;

    OverlayWidget* m_captureOverlay;
    QWidget* m_captureDialog;

    QLabel* m_titleLabel;
    QLabel* m_instructionLabel;
    QWidget* m_screensContainer;
    QGridLayout* m_screensLayout;
    QPushButton* m_shareButton;
    QLabel* m_statusLabel;

    QTimer* m_captureTimer;
    bool m_isCapturing;
    QList<QScreen*> m_availableScreens;
    QList<ScreenPreviewWidget*> m_previewWidgets;
    int m_selectedScreenIndex;
};