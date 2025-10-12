#pragma once
#include <QWidget>
#include <QPainter>
#include <QEvent>
#include <QScreen>

class OverlayWidget : public QWidget {
public:
    explicit OverlayWidget(QWidget* parent = nullptr);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void paintEvent(QPaintEvent*) override;

private:
    void updateGeometryToParent();
};