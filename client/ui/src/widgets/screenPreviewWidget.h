#pragma once

#include <QWidget>
#include <QLabel>
#include <QPixmap>
#include <QMouseEvent>
#include <QGraphicsDropShadowEffect>

class ScreenPreviewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ScreenPreviewWidget(int screenIndex, QScreen* screen, QWidget* parent = nullptr);

    void setSelected(bool selected);
    bool isSelected() const { return m_isSelected; }
    int screenIndex() const { return m_screenIndex; }
    void updatePreview();
    QSize calculatePreviewSize() const;

signals:
    void screenClicked(int screenIndex, bool currentlySelected);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    int m_screenIndex;
    QScreen* m_screen;
    QLabel* m_previewLabel;
    bool m_isSelected;
    QSize m_screenSize;
};
