#pragma once

#include <QWidget>

class ContainerHighlightOverlay : public QWidget {
public:
    explicit ContainerHighlightOverlay(QWidget* parent = nullptr, int cornerRadiusPx = 20,
        double highlightFade = 0.35, int highlightAlpha = 200);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int m_cornerRadiusPx;
    double m_highlightFade;
    int m_highlightAlpha;
};
