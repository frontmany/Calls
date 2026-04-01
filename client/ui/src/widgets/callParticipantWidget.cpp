#include "callParticipantWidget.h"

#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLinearGradient>
#include <QPainter>
#include <QVBoxLayout>

#include "constants/color.h"
#include "utilities/utility.h"

CallParticipantWidget::CallParticipantWidget(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    setMinimumHeight(scale(156));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(scale(28), scale(18), scale(28), scale(18));
    const int rowGap = scale(10);
    const int mutedGap = scale(12);
    layout->setSpacing(rowGap);

    m_timerLabel = new QLabel("00:00", this);
    m_timerLabel->setAlignment(Qt::AlignCenter);
    m_timerLabel->setStyleSheet(QString(
        "QLabel {"
        " color: %1;"
        " background: transparent;"
        " font-weight: 700;"
        " letter-spacing: 0.4px;"
        " margin: 0px;"
        " padding: 0px;"
        "}"
    ).arg(COLOR_TEXT_SECONDARY.name()));
    QFont timerFont("Outfit", scale(54), QFont::Bold);
    m_timerLabel->setFont(timerFont);
    layout->addWidget(m_timerLabel, 0, Qt::AlignCenter);

    m_nicknameLabel = new QLabel("Friend", this);
    m_nicknameLabel->setAlignment(Qt::AlignCenter);
    m_nicknameLabel->setStyleSheet(QString(
        "QLabel {"
        " color: %1;"
        " background: transparent;"
        " font-size: %2px;"
        " font-weight: 600;"
        " letter-spacing: 0.2px;"
        " margin: 0px;"
        " padding: 0px;"
        "}"
    ).arg(COLOR_TEXT_SECONDARY.name())
     .arg(scale(20)));
    QFont nicknameFont("Outfit", scale(20), QFont::DemiBold);
    m_nicknameLabel->setFont(nicknameFont);
    layout->addWidget(m_nicknameLabel, 0, Qt::AlignCenter);
    layout->addSpacing(mutedGap);

    m_mutedLabel = new QLabel("Muted", this);
    m_mutedLabel->setAlignment(Qt::AlignCenter);
    m_mutedLabel->setStyleSheet(QString(
        "QLabel {"
        " color: %1;"
        " background-color: rgba(255, 255, 255, 62);"
        " border: 1px solid rgba(255, 255, 255, 165);"
        " border-radius: %2px;"
        " font-size: %3px;"
        " font-weight: 700;"
        " letter-spacing: 0.4px;"
        " padding: %4px %5px;"
        "}"
    ).arg(COLOR_TEXT_SECONDARY.name())
     .arg(scale(14))
     .arg(scale(13))
     .arg(scale(6))
     .arg(scale(16)));
    m_mutedLabel->hide();
    layout->addWidget(m_mutedLabel, 0, Qt::AlignCenter);

    m_shadowEffect = new QGraphicsDropShadowEffect(this);
    m_shadowEffect->setOffset(0, scale(2));
    m_shadowEffect->setBlurRadius(scale(8));
    m_shadowEffect->setColor(QColor(0, 0, 0, 35));
    setGraphicsEffect(m_shadowEffect);

    const int stableWidth = qMax(
        sizeHint().width(),
        m_mutedLabel->sizeHint().width() + layout->contentsMargins().left() + layout->contentsMargins().right());
    setMinimumWidth(stableWidth);
    setMaximumWidth(stableWidth);
}

void CallParticipantWidget::setNickname(const QString& nickname)
{
    if (m_nicknameLabel) {
        m_nicknameLabel->setText(nickname);
    }
}

void CallParticipantWidget::setSpeaking(bool speaking)
{
    if (m_muted && speaking) {
        speaking = false;
    }
    if (m_speaking == speaking) {
        return;
    }
    m_speaking = speaking;
    applyShadowForSpeaking(m_speaking);
    update();
}

void CallParticipantWidget::setMuted(bool muted)
{
    if (m_muted == muted) {
        return;
    }
    m_muted = muted;
    if (m_mutedLabel) {
        m_mutedLabel->setVisible(m_muted);
    }
    if (m_muted && m_speaking) {
        m_speaking = false;
    }
    applyShadowForSpeaking(m_speaking);
    update();
}

void CallParticipantWidget::setTimerText(const QString& timerText)
{
    if (m_timerLabel) {
        m_timerLabel->setText(timerText);
    }
}

void CallParticipantWidget::setTimerLongFormat(bool longFormat)
{
    if (!m_timerLabel) {
        return;
    }
    // Do not change font when crossing 1 hour mark; only the time format changes.
    Q_UNUSED(longFormat);
    QFont timerFont("Outfit", scale(54), QFont::Bold);
    m_timerLabel->setFont(timerFont);
}

void CallParticipantWidget::applyShadowForSpeaking(bool speaking)
{
    if (!m_shadowEffect) {
        return;
    }

    if (speaking) {
        m_shadowEffect->setColor(QColor(21, 119, 232, 210));
        m_shadowEffect->setBlurRadius(scale(18));
        m_shadowEffect->setOffset(0, 0);
    } else {
        m_shadowEffect->setColor(QColor(0, 0, 0, 35));
        m_shadowEffect->setBlurRadius(scale(8));
        m_shadowEffect->setOffset(0, scale(2));
    }
}

void CallParticipantWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF glassRect = rect().adjusted(1, 1, -1, -1);
    const qreal radius = scale(14);

    const QColor baseFill = m_speaking ? QColor(255, 255, 255, 56) : QColor(255, 255, 255, 40);
    const QColor borderColor = m_speaking ? QColor(120, 186, 255, 220) : QColor(255, 255, 255, 105);

    painter.setPen(Qt::NoPen);
    painter.setBrush(baseFill);
    painter.drawRoundedRect(glassRect, radius, radius);

    QLinearGradient gloss(glassRect.topLeft(), glassRect.bottomLeft());
    gloss.setColorAt(0.0, QColor(255, 255, 255, m_speaking ? 48 : 28));
    gloss.setColorAt(0.45, QColor(255, 255, 255, m_speaking ? 18 : 10));
    gloss.setColorAt(1.0, QColor(255, 255, 255, 0));
    painter.setBrush(gloss);
    painter.drawRoundedRect(glassRect.adjusted(scale(1), scale(1), -scale(1), -scale(1)), radius, radius);

    QPen borderPen(borderColor);
    borderPen.setWidth(1);
    painter.setPen(borderPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(glassRect, radius, radius);
}
