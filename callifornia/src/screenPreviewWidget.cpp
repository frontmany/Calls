#include "screenPreviewWidget.h"

#include <QVBoxLayout>
#include <QPainter>

ScreenPreviewWidget::ScreenPreviewWidget(int screenIndex, QScreen* screen, QWidget* parent)
    : QWidget(parent), m_screenIndex(screenIndex), m_screen(screen), m_isSelected(false)
{
    if (m_screen) {
        m_screenSize = m_screen->size();
    }

    // Рассчитываем размер на основе пропорций экрана
    QSize previewSize = calculatePreviewSize();
    setFixedSize(previewSize);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(0);

    // Preview image - адаптивный размер
    m_previewLabel = new QLabel();
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setMinimumSize(previewSize.width() - 16, previewSize.height() - 16); // Учитываем margins
    m_previewLabel->setStyleSheet(
        "background-color: rgb(230, 230, 230);"
        "border: 2px solid rgb(200, 200, 200);"
        "border-radius: 6px;"
    );
    m_previewLabel->setScaledContents(true); // Включаем масштабирование содержимого

    layout->addWidget(m_previewLabel);

    updatePreview();
    setSelected(false);
}

QSize ScreenPreviewWidget::calculatePreviewSize() const
{
    if (!m_screen) {
        return QSize(300, 200); // Размер по умолчанию
    }

    QSize screenSize = m_screen->size();
    const int maxWidth = 400;  // Максимальная ширина превью
    const int maxHeight = 300; // Максимальная высота превью

    double aspectRatio = static_cast<double>(screenSize.width()) / screenSize.height();

    int width, height;

    if (aspectRatio > 1.0) {
        // Горизонтальный экран
        width = maxWidth;
        height = static_cast<int>(maxWidth / aspectRatio);
        if (height > maxHeight) {
            height = maxHeight;
            width = static_cast<int>(maxHeight * aspectRatio);
        }
    }
    else {
        // Вертикальный экран
        height = maxHeight;
        width = static_cast<int>(maxHeight * aspectRatio);
        if (width > maxWidth) {
            width = maxWidth;
            height = static_cast<int>(maxWidth / aspectRatio);
        }
    }

    // Гарантируем минимальный размер
    width = qMax(width, 200);
    height = qMax(height, 150);

    return QSize(width, height);
}

void ScreenPreviewWidget::setSelected(bool selected)
{
    m_isSelected = selected;

    if (selected) {
        setStyleSheet(
            "ScreenPreviewWidget {"
            "   background-color: rgb(225, 245, 254);"
            "   border: 3px solid rgb(33, 150, 243);"
            "   border-radius: 10px;"
            "}"
        );

        QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
        shadowEffect->setBlurRadius(20);
        shadowEffect->setColor(QColor(33, 150, 243, 120));
        shadowEffect->setOffset(0, 0);
        setGraphicsEffect(shadowEffect);
    }
    else {
        setStyleSheet(
            "ScreenPreviewWidget {"
            "   background-color: rgb(250, 250, 250);"
            "   border: 2px solid rgb(220, 220, 220);"
            "   border-radius: 10px;"
            "}"
        );
        setGraphicsEffect(nullptr);
    }

    update();
}

void ScreenPreviewWidget::updatePreview()
{
    if (!m_screen) return;

    // Capture screen preview
    QPixmap screenshot = m_screen->grabWindow(0);

    // Масштабируем с сохранением пропорций для отображения в превью
    QPixmap preview = screenshot.scaled(m_previewLabel->size(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation);
    m_previewLabel->setPixmap(preview);
}

void ScreenPreviewWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit screenClicked(m_screenIndex, m_isSelected);
    }
    QWidget::mousePressEvent(event);
}

void ScreenPreviewWidget::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);

    // Draw selection indicator
    if (m_isSelected) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(QColor(33, 150, 243), 3));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 8, 8);
    }
}