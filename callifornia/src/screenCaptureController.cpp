#include "screenCaptureController.h"
#include "screenPreviewWidget.h"
#include "overlayWidget.h"

#include <QApplication>
#include <QScreen>
#include <QWindow>
#include <QBuffer>
#include <QFont>
#include <QTimer>
#include "scaleFactor.h"
#include <algorithm>

ScreenCaptureController::ScreenCaptureController(QWidget* parent)
    : QObject(parent), m_parent(parent), m_captureOverlay(nullptr), m_captureDialog(nullptr),
    m_screensContainer(nullptr), m_screensLayout(nullptr), m_shareButton(nullptr),
    m_statusLabel(nullptr), m_isCapturing(false), m_selectedScreenIndex(-1)
{
    m_captureTimer = new QTimer(this);
    connect(m_captureTimer, &QTimer::timeout, this, &ScreenCaptureController::captureScreen);
}

ScreenCaptureController::~ScreenCaptureController()
{
    hideCaptureDialog();
}

void ScreenCaptureController::showCaptureDialog()
{
    if (m_captureDialog)
    {
        m_captureDialog->raise();
        return;
    }

    m_captureOverlay = new OverlayWidget(m_parent);
    m_captureOverlay->setStyleSheet("background-color: transparent;");
    m_captureOverlay->setAttribute(Qt::WA_TranslucentBackground);
    m_captureOverlay->show();
    m_captureOverlay->raise();

    m_captureDialog = createCaptureDialog(m_captureOverlay);

    auto centerDialog = [this]() {
        if (!m_captureDialog || !m_captureOverlay)
        {
            return;
        }

        m_captureDialog->adjustSize();
        const QSize dialogSize = m_captureDialog->size();

        QWidget* referenceWidget = m_captureOverlay->window();
        if (!referenceWidget && m_parent)
        {
            referenceWidget = m_parent->window();
        }

        QRect targetRect;
        if (referenceWidget)
        {
            const QRect windowRect = referenceWidget->frameGeometry();
            const QPoint windowCenter = windowRect.center();

            QScreen* screen = QGuiApplication::screenAt(windowCenter);
            if (!screen && referenceWidget->windowHandle())
            {
                screen = referenceWidget->windowHandle()->screen();
            }
            if (!screen)
            {
                screen = QGuiApplication::primaryScreen();
            }
            if (screen)
            {
                targetRect = screen->availableGeometry();
            }
        }

        if (!targetRect.isValid())
        {
            if (QScreen* primaryScreen = QGuiApplication::primaryScreen())
            {
                targetRect = primaryScreen->availableGeometry();
            }
            else if (referenceWidget)
            {
                targetRect = referenceWidget->frameGeometry();
            }
            else
            {
                const QPoint overlayTopLeft = m_captureOverlay->mapToGlobal(QPoint(0, 0));
                targetRect = QRect(overlayTopLeft, m_captureOverlay->size());
            }
        }

        const QPoint desiredGlobal(
            targetRect.x() + (targetRect.width() - dialogSize.width()) / 2,
            targetRect.y() + (targetRect.height() - dialogSize.height()) / 2);

        QPoint localPos = m_captureOverlay->mapFromGlobal(desiredGlobal);
        const QRect overlayRect = m_captureOverlay->rect();
        const QRect desiredLocalRect(localPos, dialogSize);

        if (!overlayRect.contains(desiredLocalRect))
        {
            const int maxX = std::max(0, overlayRect.width() - dialogSize.width());
            const int maxY = std::max(0, overlayRect.height() - dialogSize.height());

            const int boundedX = std::clamp(localPos.x(), 0, maxX);
            const int boundedY = std::clamp(localPos.y(), 0, maxY);

            localPos = QPoint(boundedX, boundedY);

            if (!overlayRect.contains(QRect(localPos, dialogSize)))
            {
                const QPoint overlayCenter = overlayRect.center();
                localPos.setX(std::clamp(overlayCenter.x() - dialogSize.width() / 2, 0, maxX));
                localPos.setY(std::clamp(overlayCenter.y() - dialogSize.height() / 2, 0, maxY));
            }
        }

        m_captureDialog->move(localPos);
        m_captureDialog->raise();
    };

    centerDialog();
    m_captureDialog->show();
    QTimer::singleShot(0, this, centerDialog);
    QObject::connect(m_captureOverlay, &OverlayWidget::geometryChanged, this, centerDialog);

    refreshScreensPreview();
}

void ScreenCaptureController::hideCaptureDialog()
{
    if (m_captureDialog)
    {
        m_captureDialog->disconnect();
        m_captureDialog->hide();
        m_captureDialog->deleteLater();
        m_captureDialog = nullptr;
    }

    if (m_captureOverlay)
    {
        m_captureOverlay->close();
        m_captureOverlay->deleteLater();
        m_captureOverlay = nullptr;
    }

    for (ScreenPreviewWidget* preview : m_previewWidgets) {
        preview->deleteLater();
    }
    m_previewWidgets.clear();

    // Nullify dangling UI pointers (dialog is destroyed)
    m_titleLabel = nullptr;
    m_instructionLabel = nullptr;
    m_screensContainer = nullptr;
    m_screensLayout = nullptr;
    m_shareButton = nullptr;
    m_statusLabel = nullptr;

    // Ensure listeners reset toggle if dialog closed without active capture
    if (!m_isCapturing)
    {
        emit captureStopped();
    }
}

void ScreenCaptureController::startCapture()
{
    if (m_isCapturing || m_selectedScreenIndex == -1) return;

    m_isCapturing = true;
    m_captureTimer->start(100);

    if (m_shareButton) {
        m_shareButton->setEnabled(true);
    }
    if (m_statusLabel) {
        QScreen* screen = m_availableScreens[m_selectedScreenIndex];
        QString status = QString("Sharing: Screen %1")
            .arg(m_selectedScreenIndex + 1);
        m_statusLabel->setText(status);
    }

    emit captureStarted();
}

void ScreenCaptureController::stopCapture()
{
    if (!m_isCapturing) return;

    m_isCapturing = false;
    m_captureTimer->stop();

    if (m_shareButton) {
        m_shareButton->setEnabled(true);
    }
    if (m_statusLabel && m_selectedScreenIndex != -1) {
        QScreen* screen = m_availableScreens[m_selectedScreenIndex];
        QString status = QString("Selected: Screen %1 - Click Share to start streaming")
            .arg(m_selectedScreenIndex + 1);
        m_statusLabel->setText(status);
    }
    else if (m_statusLabel) {
        m_statusLabel->setText("Status: Select a screen to share");
    }

    emit captureStopped();
}

void ScreenCaptureController::captureScreen()
{
    if (m_selectedScreenIndex == -1 || m_selectedScreenIndex >= m_availableScreens.size())
        return;

    QScreen* screen = m_availableScreens[m_selectedScreenIndex];
    if (!screen) return;

    QPixmap screenshot = screen->grabWindow(0);
    std::vector<unsigned char> imageData;
    if (!screenshot.isNull())
    {
        imageData = pixmapToBytes(screenshot);
    }

    emit screenCaptured(screenshot, imageData);
}

void ScreenCaptureController::onShareButtonClicked()
{
    if (m_selectedScreenIndex == -1) return;

    if (m_isCapturing) {
        stopCapture();
    }
    else {
        startCapture();
        closeCaptureUIOnly();
    }
}

void ScreenCaptureController::onScreenSelected(int screenIndex, bool currentlySelected)
{
    if (screenIndex >= 0 && screenIndex < m_availableScreens.size()) {
        if (currentlySelected) {
            // ���� ����� ��� ������� - ����������� ���
            m_selectedScreenIndex = -1;
            if (m_shareButton) {
                m_shareButton->setEnabled(false);
            }
            if (m_statusLabel) {
                m_statusLabel->setText("Select a screen to share");
            }
        }
        else {
            // ���� ����� �� ������� - �������� ���
            m_selectedScreenIndex = screenIndex;
            if (m_shareButton) {
                m_shareButton->setEnabled(true);
            }
            if (m_statusLabel) {
                QScreen* screen = m_availableScreens[m_selectedScreenIndex];
                QString status = QString("Selected: Screen %1 - Click Share to start streaming")
                    .arg(m_selectedScreenIndex + 1);
                m_statusLabel->setText(status);
            }
        }

        // ��������� ����������� ���������
        updateSelectedScreen();
    }
}

QWidget* ScreenCaptureController::createCaptureDialog(OverlayWidget* overlay)
{
    QFont font("Outfit", scale(12), QFont::Normal);
    QFont titleFont("Outfit", scale(16), QFont::Bold);

    QWidget* dialog = new QWidget(overlay);
    // Fit dialog into overlay viewport with scaled margins
    const int margin = scale(40);
    const QSize overlaySize = overlay->size();
    const int maxW = std::max(0, overlaySize.width() - margin);
    const int maxH = std::max(0, overlaySize.height() - margin);
    const int desiredMinW = scale(1200);
    const int desiredMinH = scale(800);
    dialog->setMaximumSize(maxW, maxH);
    dialog->setMinimumWidth(std::min(desiredMinW, maxW));
    dialog->setMinimumHeight(std::min(desiredMinH, maxH));
    dialog->setAttribute(Qt::WA_TranslucentBackground);

    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
    shadowEffect->setBlurRadius(scale(30));
    shadowEffect->setXOffset(0);
    shadowEffect->setYOffset(0);
    shadowEffect->setColor(QColor(0, 0, 0, 150));

    QWidget* mainWidget = new QWidget(dialog);
    mainWidget->setGraphicsEffect(shadowEffect);
    mainWidget->setObjectName("mainWidget");

    QString mainWidgetStyle = QString(
        "QWidget#mainWidget {"
        "   background-color: rgb(248, 250, 252);"
        "   border-radius: %1px;"
        "   border: %2px solid rgb(210, 210, 210);"
        "}")
        .arg(scale(16))
        .arg(scale(1));
    mainWidget->setStyleSheet(mainWidgetStyle);

    QVBoxLayout* mainLayout = new QVBoxLayout(dialog);
    mainLayout->addWidget(mainWidget);

    QVBoxLayout* contentLayout = new QVBoxLayout(mainWidget);
    contentLayout->setContentsMargins(scale(30), scale(30), scale(30), scale(30));
    contentLayout->setSpacing(scale(12));

    m_titleLabel = new QLabel("Share Your Screen");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet(QString(
        "color: rgb(60, 60, 60);"
        "font-size: %1px;"
        "font-family: 'Outfit';"
        "font-weight: bold;"
        "padding: %2px;"
    ).arg(scale(20)).arg(scale(10)));
    m_titleLabel->setFont(titleFont);

    m_instructionLabel = new QLabel("Click on a screen to select it, then press Share to start streaming");
    m_instructionLabel->setAlignment(Qt::AlignCenter);
    m_instructionLabel->setStyleSheet(QString(
        "color: rgb(100, 100, 100);"
        "font-family: 'Outfit';"
        "font-size: %1px;"
        "padding: %2px;"
    ).arg(scale(13)).arg(scale(2)));

    // Status (moved above screens)
    m_statusLabel = new QLabel("Select a screen to share");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet(QString(
        "color: rgb(100, 100, 100);"
        "font-family: 'Outfit';"
        "font-size: %1px;"
        "padding: %2px;"
        "background-color: transparent;"
        "border-radius: %3px;"
    ).arg(scale(12)).arg(scale(2)).arg(scale(8)));

    // Screens area in Scroll Area
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setStyleSheet(QString(
        "QScrollArea {"
        "   border: none;"
        "   background-color: transparent;"
        "}"
        "QScrollBar:vertical {"
        "   border: none;"
        "   background: rgb(240, 240, 240);"
        "   width: %1px;"
        "   margin: 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "   background: rgb(200, 200, 200);"
        "   border-radius: %2px;"
        "   min-height: %3px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "   background: rgb(180, 180, 180);"
        "}"
    ).arg(scale(10)).arg(scale(5)).arg(scale(20)));

    m_screensContainer = new QWidget();
    m_screensContainer->setStyleSheet("background-color: transparent;");
    m_screensLayout = new QGridLayout(m_screensContainer);
    m_screensLayout->setSpacing(scale(25));
    m_screensLayout->setAlignment(Qt::AlignCenter);

    m_screensLayout->setHorizontalSpacing(scale(25));
    m_screensLayout->setVerticalSpacing(scale(25));

    QHBoxLayout* scrollLayout = new QHBoxLayout();
    scrollLayout->addStretch();
    scrollLayout->addWidget(m_screensContainer);
    scrollLayout->addStretch();

    QWidget* scrollContent = new QWidget();
    scrollContent->setLayout(scrollLayout);
    scrollArea->setWidget(scrollContent);

    // ������ Share
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setAlignment(Qt::AlignCenter);

    m_shareButton = new QPushButton("Share Screen");
    m_shareButton->setCursor(Qt::PointingHandCursor);
    m_shareButton->setFixedWidth(scale(200));
    m_shareButton->setMinimumHeight(scale(44));
    m_shareButton->setEnabled(false);
    m_shareButton->setStyleSheet(QString(
        "QPushButton {"
        "   background-color: rgb(21, 119, 232);"
        "   color: white;"
        "   border-radius: %1px;"
        "   padding: %2px %3px;"
        "   font-family: 'Outfit';"
        "   font-size: %4px;"
        "   font-weight: bold;"
        "   border: none;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgb(18, 113, 222);"
        "}"
        "QPushButton:pressed {"
        "   background-color: rgb(16, 103, 202);"
        "}"
        "QPushButton:disabled {"
        "   background-color: rgba(150, 150, 150, 0.20);"
        "   color: rgba(90, 90, 90, 0.75);"
        "}"
    ).arg(scale(10)).arg(scale(10)).arg(scale(20)).arg(scale(14)));
    m_shareButton->setFont(font);

    QPushButton* closeButton = new QPushButton("Close");
    closeButton->setCursor(Qt::PointingHandCursor);
    closeButton->setFixedWidth(scale(120));
    closeButton->setMinimumHeight(scale(44));
    closeButton->setStyleSheet(QString(
        "QPushButton {"
        "   background-color: rgba(21, 119, 232, 0.08);"
        "   color: rgb(21, 119, 232);"
        "   border-radius: %1px;"
        "   padding: %2px %3px;"
        "   font-family: 'Outfit';"
        "   font-size: %4px;"
        "   border: none;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgba(21, 119, 232, 0.14);"
        "   color: rgb(18, 113, 222);"
        "}"
        "QPushButton:pressed {"
        "   background-color: rgba(21, 119, 232, 0.20);"
        "   color: rgb(16, 103, 202);"
        "}"
    ).arg(scale(10)).arg(scale(10)).arg(scale(20)).arg(scale(13)));
    closeButton->setFont(font);

    buttonLayout->addWidget(m_shareButton);
    buttonLayout->addSpacing(scale(20));
    buttonLayout->addWidget(closeButton);

    contentLayout->addWidget(m_titleLabel);
    contentLayout->addWidget(m_instructionLabel);
    contentLayout->addWidget(m_statusLabel);
    contentLayout->addWidget(scrollArea, 1);
    contentLayout->addLayout(buttonLayout);

    connect(m_shareButton, &QPushButton::clicked, this, &ScreenCaptureController::onShareButtonClicked);
    connect(closeButton, &QPushButton::clicked, this, &ScreenCaptureController::hideCaptureDialog);

    return dialog;
}

void ScreenCaptureController::closeCaptureUIOnly()
{
    if (m_captureDialog)
    {
        m_captureDialog->disconnect();
        m_captureDialog->hide();
        m_captureDialog->deleteLater();
        m_captureDialog = nullptr;
    }

    if (m_captureOverlay)
    {
        m_captureOverlay->close();
        m_captureOverlay->deleteLater();
        m_captureOverlay = nullptr;
    }

    for (ScreenPreviewWidget* preview : m_previewWidgets) {
        preview->deleteLater();
    }
    m_previewWidgets.clear();

    // Nullify dangling UI pointers (dialog is destroyed)
    m_titleLabel = nullptr;
    m_instructionLabel = nullptr;
    m_screensContainer = nullptr;
    m_screensLayout = nullptr;
    m_shareButton = nullptr;
    m_statusLabel = nullptr;
}

void ScreenCaptureController::refreshScreensPreview()
{
    // ������� ������ ������
    for (ScreenPreviewWidget* preview : m_previewWidgets) {
        preview->deleteLater();
    }
    m_previewWidgets.clear();

    // �������� ��������� ������
    m_availableScreens = QGuiApplication::screens();

    if (m_screensLayout) {
        // ������� layout
        QLayoutItem* item;
        while ((item = m_screensLayout->takeAt(0)) != nullptr) {
            delete item;
        }

        // Determine responsive columns based on dialog width
        int row = 0;
        int col = 0;
        int maxCols = 3;
        if (m_captureDialog)
        {
            const int w = m_captureDialog->width();
            if (w < scale(700))
                maxCols = 1;
            else if (w < scale(1000))
                maxCols = 2;
            else
                maxCols = 3;
        }

        for (int i = 0; i < m_availableScreens.size(); ++i) {
            ScreenPreviewWidget* preview = new ScreenPreviewWidget(i, m_availableScreens[i], m_screensContainer);
            m_screensLayout->addWidget(preview, row, col, Qt::AlignCenter);
            m_previewWidgets.append(preview);

            connect(preview, &ScreenPreviewWidget::screenClicked,
                this, &ScreenCaptureController::onScreenSelected);

            col++;
            if (col >= maxCols) {
                col = 0;
                row++;
            }
        }

        // ��������� ������������� �������� ��� ����������� ������������
        for (int c = 0; c < maxCols; c++) {
            m_screensLayout->setColumnStretch(c, 1);
        }
        for (int r = 0; r <= row; r++) {
            m_screensLayout->setRowStretch(r, 1);
        }
    }

    m_selectedScreenIndex = -1;
    if (m_shareButton) {
        m_shareButton->setEnabled(false);
        m_shareButton->setText("Share Screen");
    }
    if (m_statusLabel) {
        m_statusLabel->setText("Status: Select a screen to share");
    }

    if (m_screensContainer) {
        m_screensContainer->adjustSize();
    }
}

std::vector<unsigned char> ScreenCaptureController::pixmapToBytes(const QPixmap& pixmap)
{
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);

    pixmap.save(&buffer, "JPG", 40);

    return std::vector<unsigned char>(byteArray.begin(), byteArray.end());
}

void ScreenCaptureController::updateSelectedScreen()
{
    for (int i = 0; i < m_previewWidgets.size(); ++i) {
        ScreenPreviewWidget* preview = m_previewWidgets[i];
        preview->setSelected(i == m_selectedScreenIndex);
    }
}