#pragma once
#include <QWidget>
#include <QPainter>
#include <QEvent>
#include <QScreen>

class OverlayWidget : public QWidget {
    Q_OBJECT

public:
    explicit OverlayWidget(QWidget* parent = nullptr);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private:
    void updateGeometryToParent();

signals:
    void geometryChanged();
};
