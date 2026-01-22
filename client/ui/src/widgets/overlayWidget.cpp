#include "overlayWidget.h"
#include "utilities/color.h"
#include <QResizeEvent>

OverlayWidget::OverlayWidget(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);
    // Make this a true child overlay so it does not become a separate top-level window

    if (parent) {
        parent->installEventFilter(this);
        updateGeometryToParent();
    }
}

bool OverlayWidget::eventFilter(QObject* obj, QEvent* event) {
    if (obj == parentWidget()) {
        if (event->type() == QEvent::Move ||
            event->type() == QEvent::Resize ||
            event->type() == QEvent::WindowStateChange) {
            updateGeometryToParent();
            emit geometryChanged();
        }
    }
    return QWidget::eventFilter(obj, event);
}

void OverlayWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.fillRect(rect(), COLOR_SHADOW_BLACK_160);
}

void OverlayWidget::updateGeometryToParent() {
    if (auto* parent = parentWidget()) {
        // Cover the parent area only (not the native title bar)
        setGeometry(parent->rect());
        raise();
        emit geometryChanged();
    }
}

void OverlayWidget::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    emit geometryChanged();
}