#include "buttons.h"
/*

AvatarIcon::AvatarIcon(QWidget* parent, int iconSize, int size, bool needHover, Theme theme)
    : QWidget(parent), m_iconSize(iconSize, iconSize), m_size(iconSize), m_needHover(needHover), m_theme(theme)
{
    setFixedSize(size, size);
    setAttribute(Qt::WA_Hover);
}

void AvatarIcon::setIcon(const QIcon& icon)
{
    m_icon = icon;
    update();
}

void AvatarIcon::setIconSize(QSize size)
{
    m_iconSize = size;
    update();
}

void AvatarIcon::setOnlineIndicator(bool isOnline)
{
    m_isOnline = isOnline;
    update();
}

bool AvatarIcon::event(QEvent* event)
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

void AvatarIcon::hoverEnter(QHoverEvent* event)
{
    m_hovered = true;
    update();
}

void AvatarIcon::hoverLeave(QHoverEvent* event)
{
    m_hovered = false;
    update();
}

void AvatarIcon::hoverMove(QHoverEvent* event)
{
    update();
}

void AvatarIcon::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked();
    }
}

void AvatarIcon::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    qreal dpr = devicePixelRatioF();
    qreal scaleFactor = dpr;

    if (m_hovered && m_needHover)
    {
        qreal hoverMargin = 4;
        QRectF circleRect = rect().adjusted(hoverMargin, hoverMargin, -hoverMargin, -hoverMargin);

        QRadialGradient gradient(circleRect.center(), circleRect.width());
        if (m_theme == Theme::LIGHT) {
            gradient.setColorAt(0, QColor(63, 139, 252, 150));
            gradient.setColorAt(0.7, QColor(127, 179, 255, 50));
            gradient.setColorAt(1, QColor(189, 215, 255, 0));
        }
        else {
            gradient.setColorAt(0, QColor(255, 255, 255, 150));
            gradient.setColorAt(0.7, QColor(255, 255, 255, 50));
            gradient.setColorAt(1, QColor(255, 255, 255, 0));
        }

        painter.setBrush(gradient);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(circleRect);
    }

    if (!m_icon.isNull())
    {
        QSize scaledSize(m_size, m_size);
        QPixmap pixmap = m_icon.pixmap(scaledSize);

        int x = (width() - m_size) / 2;
        int y = (height() - m_size) / 2;

        painter.drawPixmap(x, y, pixmap);
    }

    if (m_isOnline) {
        int indicatorSize = 10;
        int indicatorX = width() - indicatorSize;
        int indicatorY = height() - indicatorSize - 5;
        
        painter.setBrush(Qt::white);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(indicatorX, indicatorY, indicatorSize, indicatorSize);
    }
}

void AvatarIcon::setTheme(Theme theme) {
    m_theme = theme;
    update();
}

ToggleSwitch::ToggleSwitch(QWidget* parent, Theme theme)
    : QWidget(parent), m_isChecked(false), m_radius(20), m_indicatorX(5) {
    setFixedSize(58, 31);
    m_theme = theme;

    m_sunIcon = QIcon(":/resources/ChatsWidget/sun.png");
    m_moonIcon = QIcon(":/resources/ChatsWidget/moon.png");

    m_animation = new QPropertyAnimation(this, "indicatorX");
    m_animation->setDuration(200);
    m_animation->setEasingCurve(QEasingCurve::InOutQuad);
}

bool ToggleSwitch::isChecked() const {
    return m_isChecked;
}

void ToggleSwitch::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setPen(Qt::NoPen);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setBrush(m_backgroundColor);
    painter.drawRoundedRect(0, 0, width(), height(), 15, 15);

    int iconSize = 24; 
    int yOffset = ((height() - iconSize) / 2); 
    QRect iconRect(m_indicatorX, yOffset, iconSize, iconSize);

    QIcon icon = (m_theme == DARK) ? m_moonIcon : m_sunIcon;
    QPixmap pixmap = icon.pixmap(iconSize, iconSize);
    painter.drawPixmap(iconRect, pixmap);
}

void ToggleSwitch::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        setTheme(m_theme == DARK ? LIGHT : DARK);
        m_isChecked = !m_isChecked;
        updateAnimation();
        emit toggled(m_isChecked);
    }
}

void ToggleSwitch::updateAnimation() {
    int iconSize = 24;
    int padding = 6; 

    if (m_isChecked) {
        m_animation->setStartValue(m_indicatorX);
        m_animation->setEndValue(width() - iconSize - padding);
    }
    else {
        m_animation->setStartValue(m_indicatorX);
        m_animation->setEndValue(padding);
    }

    m_animation->start();

    connect(m_animation, &QPropertyAnimation::valueChanged, [this](const QVariant& value) {
        m_indicatorX = value.toInt();
        update();
        });
}

void ToggleSwitch::setTheme(Theme theme) {
    m_theme = theme;
    if (m_theme == DARK) {
        m_backgroundColor = QColor(30, 30, 30);
    }
    else {
        m_backgroundColor = QColor(240, 238, 237);
    }
    update();
}

QColor ToggleSwitch::backgroundColor() const {
    return m_backgroundColor;
}

void ToggleSwitch::setBackgroundColor(const QColor& color) {
    m_backgroundColor = color;
    update();
}

ButtonCursor::ButtonCursor(QWidget* parent, Theme theme)
    : QWidget(parent),  m_theme(theme){
    setAttribute(Qt::WA_Hover);
}

QSize ButtonCursor::sizeHint() const
{
    return QSize(50, 50);
}

void ButtonCursor::setTheme(Theme theme)
{
    m_theme = theme;
    update();
    
}

void ButtonCursor::uploadIconsLight(QIcon light, QIcon lightHover)
{
    m_iconLight = light;
    m_hoverIconLight = lightHover;
    m_currentIcon = m_iconLight;
}

void ButtonCursor::uploadIconsDark(QIcon dark, QIcon darkHover)
{
    m_iconDark = dark;
    m_hoverIconDark = darkHover;
    m_currentIcon = m_iconDark; 
}

void ButtonCursor::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setBrush(Qt::transparent);
    painter.setPen(Qt::NoPen);
    painter.drawRect(rect());

    QPixmap pixmap = m_currentIcon.pixmap(32, 32);
    int x = (width() - pixmap.width()) / 2;
    int y = (height() - pixmap.height()) / 2;
    painter.drawPixmap(x, y, pixmap);
}

bool ButtonCursor::event(QEvent* event)
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

void ButtonCursor::hoverEnter(QHoverEvent* event)
{
    setCursor(Qt::PointingHandCursor); 
    update();
}

void ButtonCursor::hoverLeave(QHoverEvent* event)
{
    unsetCursor(); 
    update();
}

void ButtonCursor::hoverMove(QHoverEvent* event)
{
    update();
}

void ButtonCursor::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked();
    }
}
*/










ButtonIcon::ButtonIcon(QWidget* parent, int width, int height)
    : QWidget(parent)
{
    setSize(width, height);
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover);
}

ButtonIcon::ButtonIcon(QWidget* parent, const QIcon& icon, const QIcon& iconHover, int width, int height)
    : QWidget(parent), m_icon(icon), m_iconHover(iconHover)
{
    setSize(width, height);
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover);
    updateIcon();
}

void ButtonIcon::setIcons(const QIcon& icon, const QIcon& iconHover)
{
    m_icon = icon;
    m_iconHover = iconHover;
    updateIcon();
}

void ButtonIcon::setSize(int width, int height)
{
    setFixedSize(width, height);
    m_iconSize = QSize(width, height);
    update();
}

void ButtonIcon::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (!m_currentIcon->isNull())
    {
        int x = (width() - m_iconSize.width()) / 2;
        int y = (height() - m_iconSize.height()) / 2;
        painter.drawPixmap(x, y, m_currentIcon->pixmap(m_iconSize));
    }
}

bool ButtonIcon::event(QEvent* event)
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

void ButtonIcon::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && rect().contains(event->pos())) {
        emit clicked();
    }
}

void ButtonIcon::updateIcon() {
    if (m_hovered) {
        m_currentIcon = &m_iconHover;
    }
    else {
        m_currentIcon = &m_icon;
    }
}

void ButtonIcon::hoverEnter(QHoverEvent* event)
{
    m_hovered = true;
    updateIcon();
    update();
}

void ButtonIcon::hoverLeave(QHoverEvent* event)
{
    m_hovered = false;
    updateIcon();
    update();
}

void ButtonIcon::hoverMove(QHoverEvent* event)
{
    update();
}




























ToggleButtonIcon::ToggleButtonIcon(QWidget* parent, int width, int height)
    : QWidget(parent)
{
    setSize(width, height);
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover);
}

ToggleButtonIcon::ToggleButtonIcon(QWidget* parent, const QIcon& icon, const QIcon& iconHover, const QIcon& iconToggled, const QIcon& iconToggledHover, int width, int height)
    : QWidget(parent),
    m_icon(icon),
    m_iconHover(iconHover),
    m_iconToggled(iconToggled),
    m_iconToggledHover(iconToggledHover)
{
    setSize(width, height);
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover);
    updateIcon();
}

void ToggleButtonIcon::setIcons(const QIcon& icon, const QIcon& iconHover, const QIcon& iconToggled, const QIcon& iconToggledHover)
{
    m_icon = icon;
    m_iconHover = iconHover;
    m_iconToggled = iconToggled;
    m_iconToggledHover = iconToggledHover;
    updateIcon();
}

void ToggleButtonIcon::setSize(int width, int height)
{
    setFixedSize(width, height);
    m_iconSize = QSize(width, height);
    update();
}

void ToggleButtonIcon::setToggled(bool isToggled)
{
    if (m_toggled != isToggled) {
        m_toggled = isToggled;
        updateIcon();
        update();
        emit toggled(m_toggled);
    }
}

void ToggleButtonIcon::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (m_currentIcon && !m_currentIcon->isNull())
    {
        int x = (width() - m_iconSize.width()) / 2;
        int y = (height() - m_iconSize.height()) / 2;
        painter.drawPixmap(x, y, m_currentIcon->pixmap(m_iconSize));
    }
}

bool ToggleButtonIcon::event(QEvent* event)
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

void ToggleButtonIcon::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && rect().contains(event->pos())) {
        setToggled(!m_toggled);
        emit clicked();
    }
}

void ToggleButtonIcon::updateIcon() {
    if (m_toggled) {
        m_currentIcon = m_hovered ? &m_iconToggledHover : &m_iconToggled;
    }
    else {
        m_currentIcon = m_hovered ? &m_iconHover : &m_icon;
    }
}

void ToggleButtonIcon::hoverEnter(QHoverEvent* event)
{
    m_hovered = true;
    updateIcon();
    update();
}

void ToggleButtonIcon::hoverLeave(QHoverEvent* event)
{
    m_hovered = false;
    updateIcon();
    update();
}

void ToggleButtonIcon::hoverMove(QHoverEvent* event)
{
    update();
}

/*
void RoundIconButton::setTheme(Theme theme) {
    if (theme == DARK) {
        QIcon icon(":/resources/ChatsWidget/userDark.png");
        QIcon iconHover(":/resources/ChatsWidget/userDarkHover.png");
        m_normalIcon = icon;
        m_hoverIcon = iconHover;
        m_currentIcon = m_normalIcon;
        update();
    }
    else {
        QIcon icon(":/resources/ChatsWidget/userLight.png");
        QIcon iconHover(":/resources/ChatsWidget/userLightHover.png");
        m_normalIcon = icon;
        m_hoverIcon = iconHover;
        m_currentIcon = m_normalIcon;
        update();
    }
}
*/