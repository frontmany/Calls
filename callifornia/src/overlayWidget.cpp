#include "overlayWidget.h"

OverlayWidget::OverlayWidget(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);

    if (parent) {
        parent->installEventFilter(this);
        updateGeometryToParent();
    }
}

bool OverlayWidget::eventFilter(QObject* obj, QEvent* event) {
    if (obj == parentWidget()) {
        if (event->type() == QEvent::Move ||
            event->type() == QEvent::Resize) {
            updateGeometryToParent();
        }
    }
    return QWidget::eventFilter(obj, event);
}

void OverlayWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor(25, 25, 25, 160));
}

void OverlayWidget::updateGeometryToParent() {
    if (auto* parent = parentWidget()) {
        QRect globalRect(parent->mapToGlobal(QPoint(0, 0)),
            parent->size());

        if (auto* screen = parent->screen()) {
            globalRect = globalRect.intersected(screen->geometry());
        }

        setGeometry(globalRect);
        raise();  
    }
}