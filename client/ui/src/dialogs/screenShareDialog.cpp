#include "dialogs/screenShareDialog.h"
#include "widgets/components/screenPreviewWidget.h"
#include "utilities/utility.h"
#include "constants/color.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QGraphicsDropShadowEffect>
#include <QGuiApplication>
#include <QTimer>
#include <QResizeEvent>

QString StyleScreenShareDialog::mainWidgetStyle(int radius, int border)
{
    return QString(
        "QWidget#mainWidget {"
        "   background-color: %1;"
        "   border-radius: %2px;"
        "   border: %3px solid %4;"
        "}")
        .arg(COLOR_BACKGROUND_SUBTLE.name())
        .arg(radius)
        .arg(border)
        .arg(COLOR_BORDER_NEUTRAL.name());
}

QString StyleScreenShareDialog::titleStyle(int fontSize, int padding)
{
    return QString(
        "color: %1;"
        "font-size: %2px;"
        "font-family: 'Outfit';"
        "font-weight: bold;"
        "padding: %3px;"
    ).arg(COLOR_TEXT_TERTIARY.name()).arg(fontSize).arg(padding);
}

QString StyleScreenShareDialog::instructionStyle(int fontSize, int padding)
{
    return QString(
        "color: %1;"
        "font-family: 'Outfit';"
        "font-size: %2px;"
        "padding: %3px;"
    ).arg(COLOR_TEXT_DISABLED.name()).arg(fontSize).arg(padding);
}

QString StyleScreenShareDialog::statusStyle(int fontSize, int padding, int radius)
{
    return QString(
        "color: %1;"
        "font-family: 'Outfit';"
        "font-size: %2px;"
        "padding: %3px;"
        "background-color: transparent;"
        "border-radius: %4px;"
    ).arg(COLOR_TEXT_DISABLED.name()).arg(fontSize).arg(padding).arg(radius);
}

QString StyleScreenShareDialog::shareButtonStyle(int radius, int paddingH, int paddingV, int fontSize)
{
    return QString(
        "QPushButton {"
        "   background-color: %1;"
        "   color: %2;"
        "   border-radius: %3px;"
        "   padding: %4px %5px;"
        "   font-family: 'Outfit';"
        "   font-size: %6px;"
        "   font-weight: bold;"
        "   border: none;"
        "}"
        "QPushButton:hover {"
        "   background-color: %7;"
        "}"
        "QPushButton:pressed {"
        "   background-color: %8;"
        "}"
        "QPushButton:disabled {"
        "   background-color: %9;"
        "   color: %10;"
        "}"
    ).arg(COLOR_ACCENT.name())
     .arg(COLOR_BACKGROUND_PURE.name())
     .arg(radius)
     .arg(paddingV)
     .arg(paddingH)
     .arg(fontSize)
     .arg(COLOR_ACCENT_HOVER.name())
     .arg(COLOR_ACCENT_PRESSED.name())
     .arg(COLOR_OVERLAY_DISABLED_20.name(QColor::HexArgb))
     .arg(COLOR_OVERLAY_DISABLED_75.name(QColor::HexArgb));
}

QString StyleScreenShareDialog::closeButtonStyle(int radius, int paddingH, int paddingV, int fontSize)
{
    return QString(
        "QPushButton {"
        "   background-color: %1;"
        "   color: %2;"
        "   border-radius: %3px;"
        "   padding: %4px %5px;"
        "   font-family: 'Outfit';"
        "   font-size: %6px;"
        "   border: none;"
        "}"
        "QPushButton:hover {"
        "   background-color: %7;"
        "   color: %8;"
        "}"
        "QPushButton:pressed {"
        "   background-color: %9;"
        "   color: %10;"
        "}"
    ).arg(COLOR_OVERLAY_ACCENT_8.name(QColor::HexArgb))
     .arg(COLOR_ACCENT.name())
     .arg(radius)
     .arg(paddingV)
     .arg(paddingH)
     .arg(fontSize)
     .arg(COLOR_OVERLAY_ACCENT_36.name(QColor::HexArgb))
     .arg(COLOR_ACCENT_HOVER.name())
     .arg(COLOR_OVERLAY_ACCENT_20.name(QColor::HexArgb))
     .arg(COLOR_ACCENT_PRESSED.name());
}

QString StyleScreenShareDialog::scrollAreaStyle(int barWidth, int handleRadius, int handleMinHeight)
{
    return QString(
        "QScrollArea {"
        "   border: none;"
        "   background: transparent;"
        "}"
        "QScrollBar:vertical {"
        "   background-color: transparent;"
        "   width: %1px;"
        "   margin: 0px;"
        "   border-radius: %2px;"
        "}"
        "QScrollBar::handle:vertical {"
        "   background-color: %4;"
        "   border-radius: %2px;"
        "   min-height: %3px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "   background-color: %5;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "   height: 0px;"
        "}"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
        "   background: transparent;"
        "}"
    ).arg(barWidth)
     .arg(handleRadius)
     .arg(handleMinHeight)
     .arg(COLOR_SHADOW_MEDIUM_60.name(QColor::HexArgb))
     .arg(COLOR_SHADOW_STRONG_80.name(QColor::HexArgb));
}

ScreenShareDialog::ScreenShareDialog(QWidget* parent)
    : QWidget(parent)
{
    QFont font("Outfit", scale(12), QFont::Normal);
    QFont titleFont("Outfit", scale(16), QFont::Bold);

    setAttribute(Qt::WA_TranslucentBackground);
    
    // Set maximum and minimum sizes based on parent overlay, similar to old implementation
    if (parent)
    {
        const int margin = scale(40);
        const QSize parentSize = parent->size();
        const int maxW = std::max(0, parentSize.width() - margin);
        const int maxH = std::max(0, parentSize.height() - margin);
        const int desiredMinW = extraScale(1200, 3);
        const int desiredMinH = extraScale(800, 3);
        setMaximumSize(maxW, maxH);
        setMinimumWidth(std::min(desiredMinW, maxW));
        setMinimumHeight(std::min(desiredMinH, maxH));
    }
    else
    {
        setMinimumSize(scale(400), scale(300));
    }

    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
    shadowEffect->setBlurRadius(scale(30));
    shadowEffect->setXOffset(0);
    shadowEffect->setYOffset(0);
    shadowEffect->setColor(COLOR_SHADOW_STRONG_150);

    QWidget* mainWidget = new QWidget(this);
    mainWidget->setGraphicsEffect(shadowEffect);
    mainWidget->setObjectName("mainWidget");
    mainWidget->setStyleSheet(StyleScreenShareDialog::mainWidgetStyle(scale(16), scale(1)));

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(mainWidget);

    QVBoxLayout* contentLayout = new QVBoxLayout(mainWidget);
    contentLayout->setContentsMargins(scale(30), scale(30), scale(30), scale(30));
    contentLayout->setSpacing(scale(12));

    QLabel* titleLabel = new QLabel("Share Your Screen");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(StyleScreenShareDialog::titleStyle(scale(20), scale(10)));
    titleLabel->setFont(titleFont);

    QLabel* instructionLabel = new QLabel("Click on a screen to select it, then press Share to start streaming");
    instructionLabel->setAlignment(Qt::AlignCenter);
    instructionLabel->setStyleSheet(StyleScreenShareDialog::instructionStyle(scale(13), scale(2)));

    m_statusLabel = new QLabel("Status: Select a screen to share");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet(StyleScreenShareDialog::statusStyle(scale(12), scale(2), scale(8)));

    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setStyleSheet(StyleScreenShareDialog::scrollAreaStyle(scale(6), scale(3), scale(20)));

    m_screensContainer = new QWidget();
    m_screensContainer->setStyleSheet("background-color: transparent;");
    m_screensLayout = new QGridLayout(m_screensContainer);
    m_screensLayout->setSpacing(scale(25));
    m_screensLayout->setAlignment(Qt::AlignCenter);
    m_screensLayout->setHorizontalSpacing(scale(25));
    m_screensLayout->setVerticalSpacing(scale(25));

    // Wrap container in a layout for centering, like in old implementation
    QHBoxLayout* scrollLayout = new QHBoxLayout();
    scrollLayout->addStretch();
    scrollLayout->addWidget(m_screensContainer);
    scrollLayout->addStretch();

    QWidget* scrollContent = new QWidget();
    scrollContent->setLayout(scrollLayout);
    m_scrollArea->setWidget(scrollContent);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setAlignment(Qt::AlignCenter);

    m_shareButton = new QPushButton("Share Screen");
    m_shareButton->setCursor(Qt::PointingHandCursor);
    m_shareButton->setFixedWidth(scale(200));
    m_shareButton->setMinimumHeight(scale(44));
    m_shareButton->setEnabled(false);
    m_shareButton->setStyleSheet(StyleScreenShareDialog::shareButtonStyle(scale(10), scale(20), scale(10), scale(14)));
    m_shareButton->setFont(font);

    QPushButton* closeButton = new QPushButton("Close");
    closeButton->setCursor(Qt::PointingHandCursor);
    closeButton->setFixedWidth(scale(120));
    closeButton->setMinimumHeight(scale(44));
    closeButton->setStyleSheet(StyleScreenShareDialog::closeButtonStyle(scale(10), scale(20), scale(10), scale(13)));
    closeButton->setFont(font);

    buttonLayout->addWidget(m_shareButton);
    buttonLayout->addSpacing(scale(20));
    buttonLayout->addWidget(closeButton);

    contentLayout->addWidget(titleLabel);
    contentLayout->addWidget(instructionLabel);
    contentLayout->addWidget(m_statusLabel);
    contentLayout->addWidget(m_scrollArea, 1);
    contentLayout->addLayout(buttonLayout);

    connect(m_shareButton, &QPushButton::clicked, this, [this]() {
        if (m_selectedIndex >= 0 && m_selectedIndex < m_screens.size()) {
            emit screenSelected(m_selectedIndex);
        }
        });
    connect(closeButton, &QPushButton::clicked, this, [this]() {
        emit cancelled();
        hide();
        });
}

void ScreenShareDialog::setScreens(const QList<QScreen*>& screens)
{
    m_screens = screens;
    m_selectedIndex = -1;
    refreshScreenSharePreviews();
    updateScreenShareSelectionState();
}

void ScreenShareDialog::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    // Recalculate column layout when dialog width changes
    if (!m_screens.isEmpty())
    {
        refreshScreenSharePreviews();
    }
}

void ScreenShareDialog::refreshScreenSharePreviews()
{
    for (ScreenPreviewWidget* preview : m_previewWidgets)
    {
        preview->deleteLater();
    }
    m_previewWidgets.clear();

    if (!m_screensLayout)
    {
        return;
    }

    QLayoutItem* item = nullptr;
    while ((item = m_screensLayout->takeAt(0)) != nullptr)
    {
        delete item;
    }

    if (m_screens.isEmpty())
    {
        if (m_screensContainer)
        {
            m_screensContainer->adjustSize();
        }
        return;
    }

    // Calculate optimal column count based on dialog width, like in old implementation
    int maxCols = 3;
    const int width = this->width();
    if (width < scale(700))
    {
        maxCols = 1;
    }
    else if (width < scale(1000))
    {
        maxCols = 2;
    }

    int row = 0;
    int col = 0;

    for (int i = 0; i < m_screens.size(); ++i)
    {
        ScreenPreviewWidget* preview = new ScreenPreviewWidget(i, m_screens[i], m_screensContainer);
        m_screensLayout->addWidget(preview, row, col, Qt::AlignCenter);
        m_previewWidgets.append(preview);

        connect(preview, &ScreenPreviewWidget::screenClicked, this, [this](int index, bool currentlySelected)
            {
                handleScreenPreviewClick(index, currentlySelected);
            });

        ++col;
        if (col >= maxCols)
        {
            col = 0;
            ++row;
        }
    }

    // Set stretch factors
    for (int c = 0; c < maxCols; ++c)
    {
        m_screensLayout->setColumnStretch(c, 1);
    }
    for (int r = 0; r <= row; ++r)
    {
        m_screensLayout->setRowStretch(r, 1);
    }

    if (m_screensContainer)
    {
        m_screensContainer->adjustSize();
    }
}

void ScreenShareDialog::handleScreenPreviewClick(int screenIndex, bool currentlySelected)
{
    if (screenIndex < 0 || screenIndex >= m_screens.size())
    {
        return;
    }

    if (currentlySelected)
    {
        m_selectedIndex = -1;
    }
    else
    {
        m_selectedIndex = screenIndex;
    }

    updateScreenShareSelectionState();
}

void ScreenShareDialog::updateScreenShareSelectionState()
{
    const bool hasSelection = m_selectedIndex >= 0 && m_selectedIndex < m_screens.size();

    if (!hasSelection)
    {
        if (m_statusLabel)
        {
            if (m_screens.isEmpty())
            {
                m_statusLabel->setText("Status: No screens detected");
            }
            else
            {
                m_statusLabel->setText("Status: Select a screen to share");
            }
        }

        if (m_shareButton)
        {
            m_shareButton->setEnabled(false);
        }
    }
    else
    {
        if (m_statusLabel)
        {
            QString status = QString("Selected: Screen %1 - Click Share to start streaming")
                .arg(m_selectedIndex + 1);
            m_statusLabel->setText(status);
        }

        if (m_shareButton)
        {
            m_shareButton->setEnabled(true);
        }
    }

    for (int i = 0; i < m_previewWidgets.size(); ++i)
    {
        if (m_previewWidgets[i])
        {
            m_previewWidgets[i]->setSelected(hasSelection && i == m_selectedIndex);
        }
    }
}
