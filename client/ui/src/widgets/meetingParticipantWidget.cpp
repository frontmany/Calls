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

    m_connectionDownLabel = new QLabel(this);
    m_connectionDownLabel->setAlignment(Qt::AlignCenter);
    m_connectionDownLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_connectionDownLabel->setText(QStringLiteral("Connection down"));
    QFont connectionDownFont(QStringLiteral("Outfit"), scale(11));
    m_connectionDownLabel->setFont(connectionDownFont);
    m_connectionDownLabel->setStyleSheet(QString(
        "QLabel {"
        "   color: %1;"
        "   font-weight: normal;"
        "   background-color: transparent;"
        "   border: none;"
        "   padding: 2px 10px;"
        "}"
    ).arg(COLOR_TEXT_MUTED.name()));
    m_connectionDownLabel->hide();

    m_mutedLabel = new QLabel(this);
    m_mutedLabel->setAlignment(Qt::AlignCenter);
    m_mutedLabel->setText(QStringLiteral("Muted"));
    QFont mutedFont(QStringLiteral("Outfit"), scale(11));
    m_mutedLabel->setFont(mutedFont);
    m_mutedLabel->setStyleSheet(QString(
        "QLabel {"
        "   color: %1;"
        "   background-color: rgba(111, 124, 142, 135);"
        "   border: none;"
        "   border-radius: %2px;"
        "   padding: 2px 10px;"
        "   font-weight: 600;"
        "}"
    ).arg(COLOR_TEXT_SECONDARY.name()).arg(scale(8)));
    m_mutedLabel->hide();

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
    overlayLayout->setSpacing(scale(4));
    overlayLayout->addStretch();
    overlayLayout->addWidget(m_nameLabel, 0, Qt::AlignCenter);
    overlayLayout->addWidget(m_mutedLabel, 0, Qt::AlignCenter);
    overlayLayout->addWidget(m_connectionDownLabel, 0, Qt::AlignCenter);
    overlayLayout->addStretch();

    m_mainLayout->addWidget(m_videoScreen, 0, 0);
    m_mainLayout->addWidget(overlayWidget, 0, 0);

    setDisplayMode(DisplayMode::DisplayName);

    m_shadowEffect = new QGraphicsDropShadowEffect(this);
    m_shadowEffect->setBlurRadius(scale(8));
    m_shadowEffect->setColor(QColor(0, 0, 0, 25));
    m_shadowEffect->setOffset(0, scale(2));
    setGraphicsEffect(m_shadowEffect);
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
        if (m_connectionDownLabel) {
            QFont connectionDownFont(QStringLiteral("Outfit"), scale(10));
            m_connectionDownLabel->setFont(connectionDownFont);
        }
    } else {
        setFixedSize(scale(320), scale(180));
        QFont nameFont(QStringLiteral("Outfit"), scale(20));
        m_nameLabel->setFont(nameFont);
        if (m_connectionDownLabel) {
            QFont connectionDownFont(QStringLiteral("Outfit"), scale(11));
            m_connectionDownLabel->setFont(connectionDownFont);
        }
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

void MeetingParticipantWidget::applyShadowForSpeaking(bool speaking)
{
    if (m_shadowEffect) {
        if (speaking) {
            m_shadowEffect->setColor(QColor(21, 119, 232));
            m_shadowEffect->setBlurRadius(scale(16));
            m_shadowEffect->setOffset(0, 0);
        } else {
            m_shadowEffect->setColor(QColor(0, 0, 0, 25));
            m_shadowEffect->setBlurRadius(scale(8));
            m_shadowEffect->setOffset(0, scale(2));
        }
    }
    update();
}

void MeetingParticipantWidget::setMuted(bool muted)
{
    if (m_muted == muted)
        return;

    m_muted = muted;
    if (m_mutedLabel) {
        m_mutedLabel->setVisible(m_muted);
    }

    if (muted && m_speaking) {
        m_speaking = false;
    }
    applyShadowForSpeaking(m_speaking);
    update();
}

void MeetingParticipantWidget::setSpeaking(bool speaking)
{
    if (m_muted && speaking)
        speaking = false;

    if (m_speaking == speaking) return;
    m_speaking = speaking;
    applyShadowForSpeaking(m_speaking);
}

void MeetingParticipantWidget::setConnectionDown(bool down)
{
    if (m_connectionDown == down) return;
    m_connectionDown = down;

    if (down) {
        setDisplayMode(DisplayMode::DisplayName);
        if (m_nameLabel) {
            m_nameLabel->setText(m_nickname);
        }
        if (m_connectionDownLabel) {
            m_connectionDownLabel->show();
        }
    } else {
        if (m_nameLabel) {
            m_nameLabel->setText(m_nickname);
        }
        if (m_connectionDownLabel) {
            m_connectionDownLabel->hide();
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
    const bool nicknameModeActive = (m_displayMode == DisplayMode::DisplayName);

    if (nicknameModeActive) {
        // Match the "glass" look used in main menu buttons:
        // semi-transparent white fill + soft light border.
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 255, 45));
        painter.drawRoundedRect(glassRect, 12, 12);

        QLinearGradient gloss(glassRect.topLeft(), glassRect.bottomLeft());
        gloss.setColorAt(0.0, QColor(255, 255, 255, 26));
        gloss.setColorAt(0.45, QColor(255, 255, 255, 10));
        gloss.setColorAt(1.0, QColor(255, 255, 255, 0));
        painter.setPen(Qt::NoPen);
        painter.setBrush(gloss);
        painter.drawRoundedRect(glassRect, 12, 12);

        painter.setPen(QPen(QColor(255, 255, 255, 140), 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(glassRect, 12, 12);
    } else {
        painter.setPen(Qt::NoPen);
        painter.setBrush(COLOR_OVERLAY_PURE_180);
        painter.drawRoundedRect(glassRect, 12, 12);
    }

    if (m_connectionDown) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 28));
        painter.drawRoundedRect(glassRect, 12, 12);
        painter.setPen(QPen(QColor(0, 0, 0, 18), 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(glassRect, 11, 11);
    }

    if (m_screenSharing) {
        painter.setPen(QPen(QColor(21, 119, 232), 3));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 11, 11);
    }

    painter.setBrush(Qt::NoBrush);
    QWidget::paintEvent(event);
}
