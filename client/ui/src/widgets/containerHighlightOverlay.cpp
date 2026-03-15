#include "containerHighlightOverlay.h"
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

ContainerHighlightOverlay::ContainerHighlightOverlay(QWidget* parent, int cornerRadiusPx,
    double highlightFade, int highlightAlpha)
    : QWidget(parent)
    , m_cornerRadiusPx(cornerRadiusPx)
    , m_highlightFade(highlightFade)
    , m_highlightAlpha(highlightAlpha)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
}

void ContainerHighlightOverlay::paintEvent(QPaintEvent* /*event*/) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    const int w = width();
    const int h = height();
    const int thickness = 1;

    QPainterPath clipPath;
    clipPath.addRoundedRect(rect(), m_cornerRadiusPx, m_cornerRadiusPx);
    p.setClipPath(clipPath);

    const QColor highlightColor(255, 255, 255, m_highlightAlpha);

    QLinearGradient topGrad(0, 0, w, 0);
    topGrad.setColorAt(0.0, Qt::transparent);
    topGrad.setColorAt(0.5 - m_highlightFade, Qt::transparent);
    topGrad.setColorAt(0.5, highlightColor);
    topGrad.setColorAt(0.5 + m_highlightFade, Qt::transparent);
    topGrad.setColorAt(1.0, Qt::transparent);
    p.fillRect(0, 0, w, thickness, topGrad);

    QLinearGradient bottomGrad(0, 0, w, 0);
    bottomGrad.setColorAt(0.0, Qt::transparent);
    bottomGrad.setColorAt(0.5 - m_highlightFade, Qt::transparent);
    bottomGrad.setColorAt(0.5, highlightColor);
    bottomGrad.setColorAt(0.5 + m_highlightFade, Qt::transparent);
    bottomGrad.setColorAt(1.0, Qt::transparent);
    p.fillRect(0, h - thickness, w, thickness, bottomGrad);

    QLinearGradient leftGrad(0, 0, 0, h);
    leftGrad.setColorAt(0.0, Qt::transparent);
    leftGrad.setColorAt(0.5 - m_highlightFade, Qt::transparent);
    leftGrad.setColorAt(0.5, highlightColor);
    leftGrad.setColorAt(0.5 + m_highlightFade, Qt::transparent);
    leftGrad.setColorAt(1.0, Qt::transparent);
    p.fillRect(0, 0, thickness, h, leftGrad);

    QLinearGradient rightGrad(0, 0, 0, h);
    rightGrad.setColorAt(0.0, Qt::transparent);
    rightGrad.setColorAt(0.5 - m_highlightFade, Qt::transparent);
    rightGrad.setColorAt(0.5, highlightColor);
    rightGrad.setColorAt(0.5 + m_highlightFade, Qt::transparent);
    rightGrad.setColorAt(1.0, Qt::transparent);
    p.fillRect(w - thickness, 0, thickness, h, rightGrad);
}
