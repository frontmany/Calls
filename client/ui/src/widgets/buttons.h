#pragma once

#include <QWidget>
#include <QPixmap>
#include <QIcon>
#include <QHoverEvent>
#include <QMouseEvent>
#include <QEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include "utilities/utilities.h"

/*
class ToggleSwitch : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int indicatorX READ indicatorX WRITE setIndicatorX NOTIFY indicatorXChanged)

public:
    explicit ToggleSwitch(QWidget* parent, Theme theme);
    bool isChecked() const;
    void setBackgroundColor(const QColor& color);
    void setTheme(Theme theme);
    int indicatorX() const { return m_indicatorX; }
    void setIndicatorX(int value) {
        if (m_indicatorX != value) {
            m_indicatorX = value;
            emit indicatorXChanged(m_indicatorX);
            update();
        }
    }

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    void updateAnimation();
    QColor backgroundColor() const; 

signals:
    void toggled(bool checked);
    void indicatorXChanged(int newValue);

private:
    QIcon m_sunIcon;
    QIcon m_moonIcon;

    bool m_isChecked;
    int m_radius; 
    QPropertyAnimation* m_animation;
    int m_indicatorX;
    QColor m_backgroundColor;
    Theme m_theme;
};
*/

class ButtonIcon : public QWidget
{
    Q_OBJECT

public:
    ButtonIcon(QWidget* parent, const QIcon& icon, const QIcon& iconHover, int width = scale(32), int height = scale(32));
    ButtonIcon(QWidget* parent, int width = scale(32), int height = scale(32));
    void setIcons(const QIcon& icon, const QIcon& iconHover);
    void setSize(int width, int height);

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent* event) override;
    bool event(QEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void updateIcon();
    void hoverEnter(QHoverEvent* event);
    void hoverLeave(QHoverEvent* event);
    void hoverMove(QHoverEvent* event);

    QIcon m_icon;
    QIcon m_iconHover;
    QIcon* m_currentIcon = nullptr;
    QSize m_iconSize{ scale(32), scale(32) };
    bool m_hovered = false;
};

class ToggleButtonIcon : public QWidget
{
    Q_OBJECT

public:
    ToggleButtonIcon(QWidget* parent, const QIcon& icon, const QIcon& iconHover, const QIcon& iconToggled, const QIcon& iconToggledHover, int width = scale(32), int height = scale(32));
    ToggleButtonIcon(QWidget* parent, int width = scale(32), int height = scale(32));
    void setIcons(const QIcon& icon, const QIcon& iconHover, const QIcon& iconToggled, const QIcon& iconToggledHover);
    void setSize(int width, int height);
    void setToggled(bool toggled);
    bool isToggled() const { return m_toggled; }

signals:
    void toggled(bool state);

protected:
    void paintEvent(QPaintEvent* event) override;
    bool event(QEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void updateIcon();
    void hoverEnter(QHoverEvent* event);
    void hoverLeave(QHoverEvent* event);
    void hoverMove(QHoverEvent* event);

    QIcon m_icon;
    QIcon m_iconToggled;
    QIcon m_iconHover;
    QIcon m_iconToggledHover;
    QIcon* m_currentIcon = nullptr;
    QSize m_iconSize{ scale(32), scale(32) };
    bool m_hovered = false;
    bool m_toggled = false;
};

/*
class ButtonCursor : public QWidget
{
    Q_OBJECT

public:
    ButtonCursor(QWidget* parent, Theme theme);

    QSize sizeHint() const override;
    void setTheme(Theme theme);
    void uploadIconsLight(QIcon light, QIcon lightHover);
    void uploadIconsDark(QIcon dark, QIcon darkHover);

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent* event) override;
    bool event(QEvent* event) override;
    void hoverEnter(QHoverEvent* event);
    void hoverLeave(QHoverEvent* event);
    void hoverMove(QHoverEvent* event);
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    QIcon m_iconLight;
    QIcon m_iconDark;
    QIcon m_hoverIconLight;
    QIcon m_hoverIconDark;
    QIcon m_currentIcon;
    Theme m_theme;
};

class AvatarIcon : public QWidget
{
    Q_OBJECT

public:
    AvatarIcon(QWidget* parent, int iconSize, int size, bool needHover, Theme theme);

    void setTheme(Theme theme);

    void setIcon(const QIcon& icon);

    void setIconSize(QSize size);

    void setOnlineIndicator(bool isOnline);

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent* event) override;


    bool event(QEvent* event) override;

    void hoverEnter(QHoverEvent* event);

    void hoverLeave(QHoverEvent* event);

    void hoverMove(QHoverEvent* event);

    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    Theme m_theme;
    QIcon m_icon;
    QSize m_iconSize;
    int   m_gradient_size_index;
    int   m_size;
    bool m_hovered = false;
    bool m_needHover = false;
    bool m_isOnline = false;
};


class ButtonIcon : public QWidget
{
    Q_OBJECT

public:
    ButtonIcon(QWidget* parent, int x, int y);

    void setTheme(Theme theme);
    void uploadIconsLight(QIcon light, QIcon lightHover);
    void uploadIconsDark(QIcon dark, QIcon darkHover);
    void setIconSize(QSize size);
    void setTemporaryIcon(QIcon temporaryIcon, QIcon temporaryIconHover);

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent* event) override;
    bool event(QEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void updateIcon();
    void hoverEnter(QHoverEvent* event);
    void hoverLeave(QHoverEvent* event);
    void hoverMove(QHoverEvent* event);

    QIcon m_iconLight;
    QIcon m_iconDark;
    QIcon m_hoverIconLight;
    QIcon m_hoverIconDark;
    QIcon m_currentIcon;
    QIcon m_temporaryIcon;
    QIcon m_temporaryIconHover;
    QSize m_iconSize{ scale(32), scale(32) };
    Theme m_theme;
    bool m_hovered = false;
    bool m_isTemporaryIcon = false;
};


class RoundIconButton : public QWidget
{
    Q_OBJECT

public:
    RoundIconButton(QWidget* parent)
        : QWidget(parent)
    {
        setFixedSize(50, 50);
        setMouseTracking(true);
        setAttribute(Qt::WA_Hover);
    }

    QSize sizeHint() const override { return QSize(50, 50); }

    void setTheme(Theme theme);


protected:
    void paintEvent(QPaintEvent* event) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(Qt::gray);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(rect().adjusted(5, 5, -5, -5));

        QPixmap pixmap = m_currentIcon.pixmap(32, 32);
        int x = (width() - pixmap.width()) / 2;
        int y = (height() - pixmap.height()) / 2;
        painter.drawPixmap(x, y, pixmap);
    }

    bool event(QEvent* event) override
    {
        switch (event->type())
        {
        case QEvent::HoverEnter:
            hoverEnter(static_cast<QHoverEvent*>(event));
            return true;
        case QEvent::HoverLeave:
            hoverLeave(static_cast<QHoverEvent*>(event));
            return true;
        case QEvent::HoverMove:
            hoverMove(static_cast<QHoverEvent*>(event));
            return true;
        default:
            return QWidget::event(event);
        }
    }

    void hoverEnter(QHoverEvent* event)
    {
        m_currentIcon = m_hoverIcon;
        update();
    }

    void hoverLeave(QHoverEvent* event)
    {
        m_currentIcon = m_normalIcon;
        update();
    }

    void hoverMove(QHoverEvent* event)
    {
        update();
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            emit clicked();
        }
    }

signals:
    void clicked();

private:
    QIcon m_normalIcon;
    QIcon m_hoverIcon;
    QIcon m_currentIcon;
};

*/
