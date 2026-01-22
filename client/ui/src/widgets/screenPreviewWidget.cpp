#include "screenPreviewWidget.h"
#include "utilities/utilities.h"
#include "utilities/color.h"

#include <QVBoxLayout>
#include <QPainter>

ScreenPreviewWidget::ScreenPreviewWidget(int screenIndex, QScreen* screen, QWidget* parent)
    : QWidget(parent), m_screenIndex(screenIndex), m_screen(screen), m_isSelected(false)
{
    setCursor(Qt::PointingHandCursor);

    if (m_screen) {
        m_screenSize = m_screen->size();
    }

    QSize previewSize = calculatePreviewSize();
    setFixedSize(previewSize);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(0);

    m_previewLabel = new QLabel();
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setMinimumSize(previewSize.width() - 16, previewSize.height() - 16); // ????????? margins
    m_previewLabel->setStyleSheet(QString(
        "background-color: %1;"
        "border: %2px solid %1;"
        "border-radius: %3px;"
    )
        .arg(COLOR_GRAY_200.name())
        .arg(scale(4))
        .arg(scale(4)));
    m_previewLabel->setScaledContents(true);

    layout->addWidget(m_previewLabel);

    updatePreview();
    setSelected(false);
}

QSize ScreenPreviewWidget::calculatePreviewSize() const
{
    if (!m_screen) {
        return QSize(scale(300), scale(200));
    }

    QSize screenSize = m_screen->size();
    const int maxWidth = 400;
    const int maxHeight = 300;

    double aspectRatio = static_cast<double>(screenSize.width()) / screenSize.height();

    int width, height;

    if (aspectRatio > 1.0) {
        width = maxWidth;
        height = static_cast<int>(maxWidth / aspectRatio);
        if (height > maxHeight) {
            height = maxHeight;
            width = static_cast<int>(maxHeight * aspectRatio);
        }
    }
    else {
        height = maxHeight;
        width = static_cast<int>(maxHeight * aspectRatio);
        if (width > maxWidth) {
            width = maxWidth;
            height = static_cast<int>(maxWidth / aspectRatio);
        }
    }

    width = qMax(width, scale(200));
    height = qMax(height, scale(150));

    return QSize(width, height);
}

void ScreenPreviewWidget::setSelected(bool selected)
{
    m_isSelected = selected;

    if (selected) {
        setStyleSheet(QString(
            "ScreenPreviewWidget {"
            "   background-color: %1;"
            "   border: none;"
            "}"
        ).arg(COLOR_SELECTION_BG.name()));

        QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
        shadowEffect->setBlurRadius(20);
        shadowEffect->setColor(COLOR_SHADOW_PRIMARY_120);
        shadowEffect->setOffset(0, 0);
        setGraphicsEffect(shadowEffect);
    }
    else {
        setStyleSheet(QString(
            "ScreenPreviewWidget {"
            "   background-color: %1;"
            "   border: none;"
            "}"
        ).arg(COLOR_SELECTION_BG_LIGHT.name()));
        setGraphicsEffect(nullptr);
    }

    update();
}

void ScreenPreviewWidget::updatePreview() {
    if (!m_screen) return;

    QPixmap screenshot = m_screen->grabWindow(0);

    QPixmap preview = screenshot.scaled(m_previewLabel->size(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );

    m_previewLabel->setPixmap(preview);
}

void ScreenPreviewWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit screenClicked(m_screenIndex, m_isSelected);
    }
    QWidget::mousePressEvent(event);
}

void ScreenPreviewWidget::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);

    if (m_isSelected) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(COLOR_PRIMARY_LIGHT, scale(3)));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(rect().adjusted(scale(1), scale(1), -scale(1), -scale(1)), scale(8), scale(8));
    }
}