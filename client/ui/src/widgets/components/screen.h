#pragma once

#include <QWidget>
#include <QPixmap>
#include <QMouseEvent>
#include <QRect>
#include <QResizeEvent>

class Screen : public QWidget
{
public:
    enum class ScaleMode {
        KeepAspectRatio,
        CropToFit
    };

    explicit Screen(QWidget* parent);
    void clear();
    void setPixmap(const QPixmap& pixmap);
    void setRoundedCornersEnabled(bool enabled);
    void setScaleMode(ScaleMode mode);
    ScaleMode scaleMode() const { return m_scaleMode; }
    QSize contentSize() const;

protected:
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    void rebuildScaledPixmap();

    QPixmap m_sourcePixmap;
    QPixmap m_scaledPixmap;
    QRect m_drawTargetRect;
    QRect m_drawSourceRect;
    bool m_hasPreparedDraw = false;
    bool m_clearToWhite = false;
    bool m_roundedCornersEnabled = true;
    ScaleMode m_scaleMode = ScaleMode::KeepAspectRatio;
    static constexpr qreal m_cornerRadius = 4.0;
};
