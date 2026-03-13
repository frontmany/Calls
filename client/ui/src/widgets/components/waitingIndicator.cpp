#include "widgets/components/waitingIndicator.h"

#include <cmath>
#include <QHideEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QShowEvent>
#include <QTimer>

#include "constants/color.h"
#include "utilities/utility.h"

WaitingIndicator::WaitingIndicator(QWidget* parent)
    : QWidget(parent), m_color(COLOR_ACCENT)
{
    setFixedSize(scale(60), scale(60));
    setAttribute(Qt::WA_TransparentForMouseEvents);

    m_timer = new QTimer(this);
    m_timer->setInterval(90);
    connect(m_timer, &QTimer::timeout, this, &WaitingIndicator::advanceFrame);
}

void WaitingIndicator::start()
{
    if (m_timer && !m_timer->isActive())
    {
        m_timer->start();
    }
}

void WaitingIndicator::stop()
{
    if (m_timer && m_timer->isActive())
    {
        m_timer->stop();
    }
}

void WaitingIndicator::setColor(const QColor& color)
{
    m_color = color;
    update();
}

QSize WaitingIndicator::sizeHint() const
{
    return QSize(scale(60), scale(60));
}

void WaitingIndicator::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);

    const QPointF center = rect().center();
    const qreal orbitRadius = qMin(width(), height()) * 0.28;
    const qreal dotRadius = qMin(width(), height()) * 0.08;
    constexpr qreal kPi = 3.14159265358979323846;

    for (int i = 0; i < m_dotCount; ++i)
    {
        const int distance = (i - m_activeDotIndex + m_dotCount) % m_dotCount;
        QColor dotColor = m_color;
        dotColor.setAlpha(220 - distance * 18);

        const qreal angle = (2.0 * kPi * i) / static_cast<qreal>(m_dotCount) - (kPi / 2.0);
        const QPointF dotCenter(
            center.x() + std::cos(angle) * orbitRadius,
            center.y() + std::sin(angle) * orbitRadius);

        painter.setBrush(dotColor);
        painter.drawEllipse(dotCenter, dotRadius, dotRadius);
    }
}

void WaitingIndicator::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    start();
}

void WaitingIndicator::hideEvent(QHideEvent* event)
{
    stop();
    QWidget::hideEvent(event);
}

void WaitingIndicator::advanceFrame()
{
    m_activeDotIndex = (m_activeDotIndex + 1) % m_dotCount;
    update();
}
