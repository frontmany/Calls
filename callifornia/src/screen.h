#pragma once

#include <QWidget>
#include <QPixmap>

class Screen : public QWidget
{
public:
    explicit Screen(QWidget* parent = nullptr);

    void setPixmap(const QPixmap& pixmap);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void updateGeometryWithAspectRatio();

    QPixmap m_pixmap;
    bool m_clearToWhite = false;
    static constexpr qreal m_cornerRadius = 4.0;
};