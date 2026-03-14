#include "widgets/meetingParticipantWidget.h"
#include "utilities/utility.h"
#include "constants/color.h"

#include <QFont>
#include <QPainter>
#include <QVBoxLayout>

MeetingParticipantWidget::MeetingParticipantWidget(const QString& nickname, QWidget* parent)
    : QWidget(parent)
    , m_nickname(nickname)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAutoFillBackground(false);
    setupUI();
}

void MeetingParticipantWidget::setupUI()
{
    const QString baseStyle = R"(
        QWidget {
            background-color: transparent;
            border-radius: 12px;
        }
    )";

    setStyleSheet(baseStyle);

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFixedSize(scale(320), scale(180));
    m_compactMode = false;

    m_mainLayout = new QGridLayout(this);
    // Видео должно заполнять всю карточку без внутренних отступов,
    // поэтому убираем margin у основного лейаута.
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    m_mainLayout->setColumnStretch(0, 1);
    m_mainLayout->setRowStretch(0, 1);

    m_nameLabel = new QLabel(this);
    m_nameLabel->setAlignment(Qt::AlignCenter);
    m_nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_nameLabel->setWordWrap(true);
    m_nameLabel->setText(m_nickname);
    QFont nameFont(QStringLiteral("Outfit"), scale(20));
    m_nameLabel->setFont(nameFont);

    m_nameLabel->setStyleSheet(QString(
        "QLabel {"
        "   color: %1;"
        "   font-weight: bold;"
        "   background-color: transparent;"
        "   border: none;"
        "   padding: 10px;"
        "}"
    ).arg(COLOR_TEXT_SECONDARY.name()));

    m_videoScreen = new Screen(this);
    m_videoScreen->setStyleSheet("background-color: rgba(240, 240, 240, 100); border: none; border-radius: 8px;");
    m_videoScreen->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_videoScreen->setScaleMode(Screen::ScaleMode::CropToFit);
    m_videoScreen->hide();

    QWidget* overlayWidget = new QWidget(this);
    overlayWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    overlayWidget->setStyleSheet("background: transparent;");
    overlayWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QVBoxLayout* overlayLayout = new QVBoxLayout(overlayWidget);
    overlayLayout->setContentsMargins(scale(12), scale(12), scale(12), scale(12));
    overlayLayout->setSpacing(0);
    overlayLayout->addStretch();
    overlayLayout->addWidget(m_nameLabel, 0, Qt::AlignCenter);
    overlayLayout->addStretch();

    m_mainLayout->addWidget(m_videoScreen, 0, 0);
    m_mainLayout->addWidget(overlayWidget, 0, 0);

    setDisplayMode(DisplayMode::DisplayName);

    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect(this);
    shadowEffect->setBlurRadius(scale(8));
    shadowEffect->setColor(QColor(0, 0, 0, 25));
    shadowEffect->setOffset(0, scale(2));
    setGraphicsEffect(shadowEffect);
}

void MeetingParticipantWidget::setDisplayMode(DisplayMode mode)
{
    if (m_displayMode == mode) return;

    m_displayMode = mode;

    switch (mode) {
    case DisplayMode::DisplayName:
        if (m_videoScreen) {
            m_videoScreen->hide();
            clearVideoFrame();
        }
        if (m_nameLabel) {
            m_nameLabel->show();
            m_nameLabel->setText(m_nickname);
        }
        break;

    case DisplayMode::DisplayCamera:
        if (m_nameLabel) {
            m_nameLabel->hide();
        }
        if (m_videoScreen) {
            m_videoScreen->show();
        }
        break;
    }

    updateAppearance();
}

void MeetingParticipantWidget::updateAppearance()
{
    update();
}

void MeetingParticipantWidget::setCompactSize(bool compact)
{
    if (m_compactMode == compact) return;

    m_compactMode = compact;

    if (compact) {
        setFixedSize(scale(240), scale(135));
        QFont nameFont(QStringLiteral("Outfit"), scale(16));
        m_nameLabel->setFont(nameFont);
    } else {
        setFixedSize(scale(320), scale(180));
        QFont nameFont(QStringLiteral("Outfit"), scale(20));
        m_nameLabel->setFont(nameFont);
    }

    update();
}

void MeetingParticipantWidget::updateVideoFrame(const QPixmap& frame)
{
    if (!m_videoScreen || frame.isNull()) return;

    setDisplayMode(DisplayMode::DisplayCamera);

    m_videoScreen->setPixmap(frame);
    m_cameraEnabled = true;
}

void MeetingParticipantWidget::clearVideoFrame()
{
    if (m_videoScreen) {
        m_videoScreen->clear();
        m_cameraEnabled = false;

        setDisplayMode(DisplayMode::DisplayName);
    }
}

void MeetingParticipantWidget::setCameraEnabled(bool enabled)
{
    if (enabled) {
        if (m_videoScreen) {
            setDisplayMode(DisplayMode::DisplayCamera);
            QPixmap emptyPixmap(m_videoScreen->size());
            emptyPixmap.fill(Qt::black);
            m_videoScreen->setPixmap(emptyPixmap);
        }
    } else {
        clearVideoFrame();
        setDisplayMode(DisplayMode::DisplayName);
    }

    m_cameraEnabled = enabled;
}

void MeetingParticipantWidget::setConnectionDown(bool down)
{
    if (m_connectionDown == down) return;
    m_connectionDown = down;

    if (down) {
        setDisplayMode(DisplayMode::DisplayName);
        if (m_nameLabel) {
            m_nameLabel->setText(m_nickname + QStringLiteral(" connection down"));
        }
    } else {
        if (m_nameLabel) {
            m_nameLabel->setText(m_nickname);
        }
        setDisplayMode(DisplayMode::DisplayName);
    }
    update();
}

void MeetingParticipantWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRectF glassRect = rect().adjusted(1, 1, -1, -1);
    painter.setPen(Qt::NoPen);
    painter.setBrush(COLOR_OVERLAY_PURE_180);
    painter.drawRoundedRect(glassRect, 12, 12);

    if (m_connectionDown) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(128, 128, 128, 100));
        painter.drawRoundedRect(glassRect, 12, 12);
        painter.setPen(QPen(QColor(220, 50, 50), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 12, 12);
    }

    painter.setBrush(Qt::NoBrush);
    QWidget::paintEvent(event);
    if (m_speaking) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(21, 119, 232, 50));
        painter.drawRoundedRect(rect().adjusted(2, 2, -2, -2), 10, 10);
    }
}
