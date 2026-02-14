#pragma once

#include <QWidget>
#include <QPixmap>
#include <QMouseEvent>

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
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    QPixmap m_pixmap;
    bool m_clearToWhite = false;
    bool m_roundedCornersEnabled = true;
    ScaleMode m_scaleMode = ScaleMode::KeepAspectRatio;
    static constexpr qreal m_cornerRadius = 4.0;
};
