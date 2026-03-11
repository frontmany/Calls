#pragma once

#include <QWidget>

class ContainerHighlightOverlay : public QWidget {
public:
    explicit ContainerHighlightOverlay(QWidget* parent = nullptr, int cornerRadiusPx = 20);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int m_cornerRadiusPx;
};
