#pragma once

#include <QWidget>
#include <QPixmap>
#include <QImage>

#include "screenDiffEncoder.h"

class QPaintEvent;

class Screen : public QWidget
{

public:
    explicit Screen(QWidget* parent = nullptr);

    void setFrame(const QPixmap& frame);
    void clearFrame();
    void applyDiff(const ScreenDiffData& diffData);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void ensureFrameSize(const QSize& frameSize);
    QRect mapFrameRectToWidget(const QRect& frameRect) const;

    QImage m_currentFrame;
    static constexpr qreal m_cornerRadius = 4.0;
};

