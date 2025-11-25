#pragma once

#include <QWidget>
#include <QPixmap>

class Screen : public QWidget
{
public:
    explicit Screen(QWidget* parent = nullptr);

    void setPixmap(const QPixmap& pixmap);
    void clear();
    void enableRoundedCorners(bool enabled);

protected:
    void paintEvent(QPaintEvent* event) override;

    QPixmap m_pixmap;
    bool m_clearToWhite = false;
    bool m_roundedCornersEnabled = true; 
    static constexpr qreal m_cornerRadius = 4.0;
};