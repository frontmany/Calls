#include "dialogs/screenShareDialog.h"
#include "widgets/screenPreviewWidget.h"
#include "utilities/utilities.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QGraphicsDropShadowEffect>
#include <QGuiApplication>

QString StyleScreenShareDialog::mainWidgetStyle(int radius, int border)
{
    return QString(
        "QWidget#mainWidget {"
        "   background-color: rgb(248, 250, 252);"
        "   border-radius: %1px;"
        "   border: %2px solid rgb(210, 210, 210);"
        "}")
        .arg(radius)
        .arg(border);
}

QString StyleScreenShareDialog::titleStyle(int fontSize, int padding)
{
    return QString(
        "color: rgb(60, 60, 60);"
        "font-size: %1px;"
        "font-family: 'Outfit';"
        "font-weight: bold;"
        "padding: %2px;"
    ).arg(fontSize).arg(padding);
}

QString StyleScreenShareDialog::instructionStyle(int fontSize, int padding)
{
    return QString(
        "color: rgb(100, 100, 100);"
        "font-family: 'Outfit';"
        "font-size: %1px;"
        "padding: %2px;"
    ).arg(fontSize).arg(padding);
}

QString StyleScreenShareDialog::statusStyle(int fontSize, int padding, int radius)
{
    return QString(
        "color: rgb(100, 100, 100);"
        "font-family: 'Outfit';"
        "font-size: %1px;"
        "padding: %2px;"
        "background-color: transparent;"
        "border-radius: %3px;"
    ).arg(fontSize).arg(padding).arg(radius);
}

QString StyleScreenShareDialog::shareButtonStyle(int radius, int paddingH, int paddingV, int fontSize)
{
    return QString(
        "QPushButton {"
        "   background-color: rgb(21, 119, 232);"
        "   color: white;"
        "   border-radius: %1px;"
        "   padding: %2px %3px;"
        "   font-family: 'Outfit';"
        "   font-size: %4px;"
        "   font-weight: bold;"
        "   border: none;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgb(18, 113, 222);"
        "}"
        "QPushButton:pressed {"
        "   background-color: rgb(16, 103, 202);"
        "}"
        "QPushButton:disabled {"
        "   background-color: rgba(150, 150, 150, 0.20);"
        "   color: rgba(90, 90, 90, 0.75);"
        "}"
    ).arg(radius).arg(paddingV).arg(paddingH).arg(fontSize);
}

QString StyleScreenShareDialog::closeButtonStyle(int radius, int paddingH, int paddingV, int fontSize)
{
    return QString(
        "QPushButton {"
        "   background-color: rgba(21, 119, 232, 0.08);"
        "   color: rgb(21, 119, 232);"
        "   border-radius: %1px;"
        "   padding: %2px %3px;"
        "   font-family: 'Outfit';"
        "   font-size: %4px;"
        "   border: none;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgba(21, 119, 232, 0.14);"
        "   color: rgb(18, 113, 222);"
        "}"
        "QPushButton:pressed {"
        "   background-color: rgba(21, 119, 232, 0.20);"
        "   color: rgb(16, 103, 202);"
        "}"
    ).arg(radius).arg(paddingV).arg(paddingH).arg(fontSize);
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
        "   background-color: rgba(0, 0, 0, 60);"
        "   border-radius: %2px;"
        "   min-height: %3px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "   background-color: rgba(0, 0, 0, 80);"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "   height: 0px;"
        "}"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
        "   background: transparent;"
        "}"
    ).arg(barWidth).arg(handleRadius).arg(handleMinHeight);
}

ScreenShareDialog::ScreenShareDialog(QWidget* parent)
    : QWidget(parent)
{
    QFont font("Outfit", scale(12), QFont::Normal);
    QFont titleFont("Outfit", scale(16), QFont::Bold);

    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumWidth(scale(600));
    setMinimumHeight(scale(720));
    resize(scale(1280), scale(820));

    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
    shadowEffect->setBlurRadius(scale(30));
    shadowEffect->setXOffset(0);
    shadowEffect->setYOffset(0);
    shadowEffect->setColor(QColor(0, 0, 0, 150));

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
    m_scrollArea->setStyleSheet(StyleScreenShareDialog::scrollAreaStyle(scale(10), scale(5), scale(20)));

    m_screensContainer = new QWidget();
    m_screensContainer->setStyleSheet("background-color: transparent;");
    m_screensLayout = new QGridLayout(m_screensContainer);
    m_screensLayout->setSpacing(scale(25));
    m_screensLayout->setAlignment(Qt::AlignCenter);
    m_screensLayout->setHorizontalSpacing(scale(25));
    m_screensLayout->setVerticalSpacing(scale(25));

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

void ScreenShareDialog::refreshScreenSharePreviews()
{
    const int maxCols = 1;

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

