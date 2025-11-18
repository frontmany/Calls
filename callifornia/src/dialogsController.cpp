#include "dialogsController.h"
#include "overlayWidget.h"
#include "scaleFactor.h"
#include "screenPreviewWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QMovie>
#include <QWindow>
#include <QPixmap>
#include <QGraphicsDropShadowEffect>
#include <QScreen>
#include <QGuiApplication>
#include <QFont>
#include <QTimer>
#include <algorithm>

DialogsController::DialogsController(QWidget* parent)
    : QObject(parent), m_parent(parent), m_updatingOverlay(nullptr), m_updatingDialog(nullptr),
    m_updatingProgressLabel(nullptr), m_updatingLabel(nullptr), m_updatingGifLabel(nullptr),
    m_connectionErrorOverlay(nullptr), m_connectionErrorDialog(nullptr),
    m_screenShareOverlay(nullptr), m_screenShareDialog(nullptr),
    m_screenShareScreensContainer(nullptr), m_screenShareScreensLayout(nullptr),
    m_screenShareButton(nullptr), m_screenShareStatusLabel(nullptr),
    m_screenShareSelectedIndex(-1)
{
}

DialogsController::~DialogsController()
{
	hideUpdatingDialog();
	hideUpdatingErrorDialog();
    hideScreenShareDialog();
}

void DialogsController::showUpdatingDialog()
{
	if (m_updatingDialog)
	{
		return;
	}

	m_updatingOverlay = new OverlayWidget(m_parent);
	m_updatingOverlay->setAttribute(Qt::WA_TranslucentBackground);
    m_updatingOverlay->show();
    m_updatingOverlay->raise();

	m_updatingDialog = createUpdatingDialog(m_updatingOverlay);

    // Center inside overlay (not across the whole screen). Do it now and once after layout pass.
    auto centerUpdating = [this]()
    {
        if (!m_updatingDialog || !m_updatingOverlay)
            return;
        m_updatingDialog->adjustSize();
        QSize dialogSize = m_updatingDialog->size();
        QRect overlayRect = m_updatingOverlay->rect();
        int x = overlayRect.center().x() - dialogSize.width() / 2;
        int y = overlayRect.center().y() - dialogSize.height() / 2;
        m_updatingDialog->move(x, y);
        m_updatingDialog->raise();
    };
    centerUpdating();
    m_updatingDialog->show();
    QTimer::singleShot(0, this, centerUpdating);
    QObject::connect(m_updatingOverlay, &OverlayWidget::geometryChanged, this, centerUpdating);
}

void DialogsController::hideUpdatingDialog()
{
	if (m_updatingDialog)
	{
        m_updatingDialog->disconnect();
        m_updatingDialog->hide();
        m_updatingDialog->deleteLater();
		m_updatingDialog = nullptr;
	}

	if (m_updatingOverlay)
	{
		m_updatingOverlay->close();
		m_updatingOverlay->deleteLater();
		m_updatingOverlay = nullptr;
	}

	m_updatingProgressLabel = nullptr;
	m_updatingLabel = nullptr;
	m_updatingGifLabel = nullptr;
}

void DialogsController::showScreenShareDialog(const QList<QScreen*>& screens)
{
    m_screenShareScreens = screens;
    m_screenShareSelectedIndex = -1;

    if (m_screenShareDialog)
    {
        m_screenShareDialog->raise();
        refreshScreenSharePreviews();
        updateScreenShareSelectionState();
        return;
    }

    m_screenShareOverlay = new OverlayWidget(m_parent);
    m_screenShareOverlay->setStyleSheet("background-color: transparent;");
    m_screenShareOverlay->setAttribute(Qt::WA_TranslucentBackground);
    m_screenShareOverlay->show();
    m_screenShareOverlay->raise();

    m_screenShareDialog = createScreenShareDialog(m_screenShareOverlay);

    auto centerDialog = [this]()
    {
        if (!m_screenShareDialog || !m_screenShareOverlay)
        {
            return;
        }

        m_screenShareDialog->adjustSize();
        const QSize dialogSize = m_screenShareDialog->size();

        QWidget* referenceWidget = m_screenShareOverlay->window();
        if (!referenceWidget && m_parent)
        {
            referenceWidget = m_parent->window();
        }

        QRect targetRect;
        if (referenceWidget)
        {
            const QRect windowRect = referenceWidget->frameGeometry();
            const QPoint windowCenter = windowRect.center();

            QScreen* screen = QGuiApplication::screenAt(windowCenter);
            if (!screen && referenceWidget->windowHandle())
            {
                screen = referenceWidget->windowHandle()->screen();
            }
            if (!screen)
            {
                screen = QGuiApplication::primaryScreen();
            }
            if (screen)
            {
                targetRect = screen->availableGeometry();
            }
        }

        if (!targetRect.isValid())
        {
            if (QScreen* primaryScreen = QGuiApplication::primaryScreen())
            {
                targetRect = primaryScreen->availableGeometry();
            }
            else if (referenceWidget)
            {
                targetRect = referenceWidget->frameGeometry();
            }
            else
            {
                const QPoint overlayTopLeft = m_screenShareOverlay->mapToGlobal(QPoint(0, 0));
                targetRect = QRect(overlayTopLeft, m_screenShareOverlay->size());
            }
        }

        const QPoint desiredGlobal(
            targetRect.x() + (targetRect.width() - dialogSize.width()) / 2,
            targetRect.y() + (targetRect.height() - dialogSize.height()) / 2);

        QPoint localPos = m_screenShareOverlay->mapFromGlobal(desiredGlobal);
        const QRect overlayRect = m_screenShareOverlay->rect();
        const QRect desiredLocalRect(localPos, dialogSize);

        if (!overlayRect.contains(desiredLocalRect))
        {
            const int maxX = std::max(0, overlayRect.width() - dialogSize.width());
            const int maxY = std::max(0, overlayRect.height() - dialogSize.height());

            const int boundedX = std::clamp(localPos.x(), 0, maxX);
            const int boundedY = std::clamp(localPos.y(), 0, maxY);

            localPos = QPoint(boundedX, boundedY);

            if (!overlayRect.contains(QRect(localPos, dialogSize)))
            {
                const QPoint overlayCenter = overlayRect.center();
                localPos.setX(std::clamp(overlayCenter.x() - dialogSize.width() / 2, 0, maxX));
                localPos.setY(std::clamp(overlayCenter.y() - dialogSize.height() / 2, 0, maxY));
            }
        }

        m_screenShareDialog->move(localPos);
        m_screenShareDialog->raise();
    };

    centerDialog();
    m_screenShareDialog->show();
    QTimer::singleShot(0, this, centerDialog);
    QObject::connect(m_screenShareOverlay, &OverlayWidget::geometryChanged, this, centerDialog);

    refreshScreenSharePreviews();
    updateScreenShareSelectionState();
}

void DialogsController::hideScreenShareDialog()
{
    if (m_screenShareDialog)
    {
        m_screenShareDialog->disconnect();
        m_screenShareDialog->hide();
        m_screenShareDialog->deleteLater();
        m_screenShareDialog = nullptr;
    }

    if (m_screenShareOverlay)
    {
        m_screenShareOverlay->close();
        m_screenShareOverlay->deleteLater();
        m_screenShareOverlay = nullptr;
    }

    for (ScreenPreviewWidget* preview : m_screenSharePreviewWidgets)
    {
        preview->deleteLater();
    }
    m_screenSharePreviewWidgets.clear();

    m_screenShareScreensContainer = nullptr;
    m_screenShareScreensLayout = nullptr;
    m_screenShareButton = nullptr;
    m_screenShareStatusLabel = nullptr;
    m_screenShareScreens.clear();
    m_screenShareSelectedIndex = -1;
}

void DialogsController::showConnectionErrorDialog()
{
	if (m_connectionErrorDialog)
	{
		return;
	}

	m_connectionErrorOverlay = new OverlayWidget(m_parent);
	m_connectionErrorOverlay->setAttribute(Qt::WA_TranslucentBackground);
    m_connectionErrorOverlay->show();
    m_connectionErrorOverlay->raise();

	m_connectionErrorDialog = createConnectionErrorDialog(m_connectionErrorOverlay);

    // Center inside overlay (not across the whole screen). Do it now and once after layout pass.
    auto centerError = [this]()
    {
        if (!m_connectionErrorDialog || !m_connectionErrorOverlay)
            return;
        m_connectionErrorDialog->adjustSize();
        QSize dialogSize = m_connectionErrorDialog->size();
        QRect overlayRect = m_connectionErrorOverlay->rect();
        int x = overlayRect.center().x() - dialogSize.width() / 2;
        int y = overlayRect.center().y() - dialogSize.height() / 2;
        m_connectionErrorDialog->move(x, y);
        m_connectionErrorDialog->raise();
    };
    centerError();
    m_connectionErrorDialog->show();
    QTimer::singleShot(0, this, centerError);
    QObject::connect(m_connectionErrorOverlay, &OverlayWidget::geometryChanged, this, centerError);
}

void DialogsController::hideUpdatingErrorDialog()
{
	if (m_connectionErrorDialog)
	{
        m_connectionErrorDialog->disconnect();
        m_connectionErrorDialog->hide();
        m_connectionErrorDialog->deleteLater();
		m_connectionErrorDialog = nullptr;
	}

	if (m_connectionErrorOverlay)
	{
		m_connectionErrorOverlay->close();
		m_connectionErrorOverlay->deleteLater();
		m_connectionErrorOverlay = nullptr;
	}
}

void DialogsController::updateLoadingProgress(double progress)
{
	if (m_updatingProgressLabel)
	{
		QString progressText = QString("%1%").arg(progress, 0, 'f', 2);
		m_updatingProgressLabel->setText(progressText);
	}
}

QWidget* DialogsController::createScreenShareDialog(OverlayWidget* overlay)
{
    QFont font("Outfit", scale(12), QFont::Normal);
    QFont titleFont("Outfit", scale(16), QFont::Bold);

    QWidget* dialog = new QWidget(overlay);
    const int margin = scale(40);
    const QSize overlaySize = overlay->size();
    const int maxW = std::max(0, overlaySize.width() - margin);
    const int maxH = std::max(0, overlaySize.height() - margin);
    const int desiredMinW = scale(1200);
    const int desiredMinH = scale(800);
    dialog->setMaximumSize(maxW, maxH);
    dialog->setMinimumWidth(std::min(desiredMinW, maxW));
    dialog->setMinimumHeight(std::min(desiredMinH, maxH));
    dialog->setAttribute(Qt::WA_TranslucentBackground);

    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
    shadowEffect->setBlurRadius(scale(30));
    shadowEffect->setXOffset(0);
    shadowEffect->setYOffset(0);
    shadowEffect->setColor(QColor(0, 0, 0, 150));

    QWidget* mainWidget = new QWidget(dialog);
    mainWidget->setGraphicsEffect(shadowEffect);
    mainWidget->setObjectName("mainWidget");

    QString mainWidgetStyle = QString(
        "QWidget#mainWidget {"
        "   background-color: rgb(248, 250, 252);"
        "   border-radius: %1px;"
        "   border: %2px solid rgb(210, 210, 210);"
        "}")
        .arg(scale(16))
        .arg(scale(1));
    mainWidget->setStyleSheet(mainWidgetStyle);

    QVBoxLayout* mainLayout = new QVBoxLayout(dialog);
    mainLayout->addWidget(mainWidget);

    QVBoxLayout* contentLayout = new QVBoxLayout(mainWidget);
    contentLayout->setContentsMargins(scale(30), scale(30), scale(30), scale(30));
    contentLayout->setSpacing(scale(12));

    QLabel* titleLabel = new QLabel("Share Your Screen");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(QString(
        "color: rgb(60, 60, 60);"
        "font-size: %1px;"
        "font-family: 'Outfit';"
        "font-weight: bold;"
        "padding: %2px;"
    ).arg(scale(20)).arg(scale(10)));
    titleLabel->setFont(titleFont);

    QLabel* instructionLabel = new QLabel("Click on a screen to select it, then press Share to start streaming");
    instructionLabel->setAlignment(Qt::AlignCenter);
    instructionLabel->setStyleSheet(QString(
        "color: rgb(100, 100, 100);"
        "font-family: 'Outfit';"
        "font-size: %1px;"
        "padding: %2px;"
    ).arg(scale(13)).arg(scale(2)));

    m_screenShareStatusLabel = new QLabel("Status: Select a screen to share");
    m_screenShareStatusLabel->setAlignment(Qt::AlignCenter);
    m_screenShareStatusLabel->setStyleSheet(QString(
        "color: rgb(100, 100, 100);"
        "font-family: 'Outfit';"
        "font-size: %1px;"
        "padding: %2px;"
        "background-color: transparent;"
        "border-radius: %3px;"
    ).arg(scale(12)).arg(scale(2)).arg(scale(8)));

    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setStyleSheet(QString(
        "QScrollArea {"
        "   border: none;"
        "   background-color: transparent;"
        "}"
        "QScrollBar:vertical {"
        "   border: none;"
        "   background: rgb(240, 240, 240);"
        "   width: %1px;"
        "   margin: 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "   background: rgb(200, 200, 200);"
        "   border-radius: %2px;"
        "   min-height: %3px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "   background-color: rgb(180, 180, 180);"
        "}"
    ).arg(scale(10)).arg(scale(5)).arg(scale(20)));

    m_screenShareScreensContainer = new QWidget();
    m_screenShareScreensContainer->setStyleSheet("background-color: transparent;");
    m_screenShareScreensLayout = new QGridLayout(m_screenShareScreensContainer);
    m_screenShareScreensLayout->setSpacing(scale(25));
    m_screenShareScreensLayout->setAlignment(Qt::AlignCenter);
    m_screenShareScreensLayout->setHorizontalSpacing(scale(25));
    m_screenShareScreensLayout->setVerticalSpacing(scale(25));

    QHBoxLayout* scrollLayout = new QHBoxLayout();
    scrollLayout->addStretch();
    scrollLayout->addWidget(m_screenShareScreensContainer);
    scrollLayout->addStretch();

    QWidget* scrollContent = new QWidget();
    scrollContent->setLayout(scrollLayout);
    scrollArea->setWidget(scrollContent);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setAlignment(Qt::AlignCenter);

    m_screenShareButton = new QPushButton("Share Screen");
    m_screenShareButton->setCursor(Qt::PointingHandCursor);
    m_screenShareButton->setFixedWidth(scale(200));
    m_screenShareButton->setMinimumHeight(scale(44));
    m_screenShareButton->setEnabled(false);
    m_screenShareButton->setStyleSheet(QString(
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
    ).arg(scale(10)).arg(scale(10)).arg(scale(20)).arg(scale(14)));
    m_screenShareButton->setFont(font);

    QPushButton* closeButton = new QPushButton("Close");
    closeButton->setCursor(Qt::PointingHandCursor);
    closeButton->setFixedWidth(scale(120));
    closeButton->setMinimumHeight(scale(44));
    closeButton->setStyleSheet(QString(
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
    ).arg(scale(10)).arg(scale(10)).arg(scale(20)).arg(scale(13)));
    closeButton->setFont(font);

    buttonLayout->addWidget(m_screenShareButton);
    buttonLayout->addSpacing(scale(20));
    buttonLayout->addWidget(closeButton);

    contentLayout->addWidget(titleLabel);
    contentLayout->addWidget(instructionLabel);
    contentLayout->addWidget(m_screenShareStatusLabel);
    contentLayout->addWidget(scrollArea, 1);
    contentLayout->addLayout(buttonLayout);

    connect(m_screenShareButton, &QPushButton::clicked, this, &DialogsController::onScreenShareButtonClicked);
    connect(closeButton, &QPushButton::clicked, this, &DialogsController::hideScreenShareDialog);

    return dialog;
}

void DialogsController::refreshScreenSharePreviews()
{
    for (ScreenPreviewWidget* preview : m_screenSharePreviewWidgets)
    {
        preview->deleteLater();
    }
    m_screenSharePreviewWidgets.clear();

    if (!m_screenShareScreensLayout)
    {
        return;
    }

    QLayoutItem* item = nullptr;
    while ((item = m_screenShareScreensLayout->takeAt(0)) != nullptr)
    {
        delete item;
    }

    int row = 0;
    int col = 0;
    int maxCols = 3;
    if (m_screenShareDialog)
    {
        const int width = m_screenShareDialog->width();
        if (width < scale(700))
        {
            maxCols = 1;
        }
        else if (width < scale(1000))
        {
            maxCols = 2;
        }
    }

    for (int i = 0; i < m_screenShareScreens.size(); ++i)
    {
        ScreenPreviewWidget* preview = new ScreenPreviewWidget(i, m_screenShareScreens[i], m_screenShareScreensContainer);
        m_screenShareScreensLayout->addWidget(preview, row, col, Qt::AlignCenter);
        m_screenSharePreviewWidgets.append(preview);

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
        m_screenShareScreensLayout->setColumnStretch(c, 1);
    }
    for (int r = 0; r <= row; ++r)
    {
        m_screenShareScreensLayout->setRowStretch(r, 1);
    }

    if (m_screenShareScreensContainer)
    {
        m_screenShareScreensContainer->adjustSize();
    }
}

void DialogsController::handleScreenPreviewClick(int screenIndex, bool currentlySelected)
{
    if (screenIndex < 0 || screenIndex >= m_screenShareScreens.size())
    {
        return;
    }

    if (currentlySelected)
    {
        m_screenShareSelectedIndex = -1;
    }
    else
    {
        m_screenShareSelectedIndex = screenIndex;
    }

    updateScreenShareSelectionState();
}

void DialogsController::updateScreenShareSelectionState()
{
    const bool hasSelection = m_screenShareSelectedIndex >= 0 && m_screenShareSelectedIndex < m_screenShareScreens.size();

    if (!hasSelection)
    {
        if (m_screenShareStatusLabel)
        {
            if (m_screenShareScreens.isEmpty())
            {
                m_screenShareStatusLabel->setText("Status: No screens detected");
            }
            else
            {
                m_screenShareStatusLabel->setText("Status: Select a screen to share");
            }
        }

        if (m_screenShareButton)
        {
            m_screenShareButton->setEnabled(false);
        }
    }
    else
    {
        if (m_screenShareStatusLabel)
        {
            QString status = QString("Selected: Screen %1 - Click Share to start streaming")
                .arg(m_screenShareSelectedIndex + 1);
            m_screenShareStatusLabel->setText(status);
        }

        if (m_screenShareButton)
        {
            m_screenShareButton->setEnabled(true);
        }
    }

    for (int i = 0; i < m_screenSharePreviewWidgets.size(); ++i)
    {
        if (m_screenSharePreviewWidgets[i])
        {
            m_screenSharePreviewWidgets[i]->setSelected(hasSelection && i == m_screenShareSelectedIndex);
        }
    }
}

void DialogsController::onScreenShareButtonClicked()
{
    if (m_screenShareSelectedIndex < 0 || m_screenShareSelectedIndex >= m_screenShareScreens.size())
    {
        return;
    }

    emit screenSelected(m_screenShareSelectedIndex);
    hideScreenShareDialog();
}

QWidget* DialogsController::createUpdatingDialog(OverlayWidget* overlay)
{
	QFont font("Outfit", 14, QFont::Normal);

    QWidget* dialog = new QWidget(overlay);
    dialog->setMinimumWidth(300);
    dialog->setMinimumHeight(250);
    dialog->setAttribute(Qt::WA_TranslucentBackground);

	QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
	shadowEffect->setBlurRadius(30);
	shadowEffect->setXOffset(0);
	shadowEffect->setYOffset(0);
	shadowEffect->setColor(QColor(0, 0, 0, 150));

    QWidget* mainWidget = new QWidget(dialog);
	mainWidget->setGraphicsEffect(shadowEffect);
	mainWidget->setObjectName("mainWidget");

	QString mainWidgetStyle =
		"QWidget#mainWidget {"
		"   background-color: rgb(226, 243, 231);"
		"   border-radius: 16px;"
		"   border: 1px solid rgb(210, 210, 210);"
		"}";
	mainWidget->setStyleSheet(mainWidgetStyle);

    QVBoxLayout* mainLayout = new QVBoxLayout(dialog);
	mainLayout->addWidget(mainWidget);

	QVBoxLayout* contentLayout = new QVBoxLayout(mainWidget);
	contentLayout->setContentsMargins(24, 24, 24, 24);
	contentLayout->setSpacing(16);

	m_updatingGifLabel = new QLabel();
	m_updatingGifLabel->setAlignment(Qt::AlignCenter);
	m_updatingGifLabel->setMinimumHeight(120);

	QMovie* movie = new QMovie(":/resources/updating.gif");
	if (movie->isValid())
	{
		m_updatingGifLabel->setMovie(movie);
		movie->start();
	}

	m_updatingProgressLabel = new QLabel("0.00%");
	m_updatingProgressLabel->setAlignment(Qt::AlignCenter);
	m_updatingProgressLabel->setStyleSheet(
		"color: rgb(80, 80, 80);"
		"font-size: 14px;"
		"font-family: 'Outfit';"
		"font-weight: bold;"
	);

	m_updatingLabel = new QLabel("Updating...");
	m_updatingLabel->setAlignment(Qt::AlignCenter);
	m_updatingLabel->setStyleSheet(
		"color: rgb(60, 60, 60);"
		"font-size: 16px;"
		"font-family: 'Outfit';"
		"font-weight: bold;"
	);
	m_updatingLabel->setFont(font);

	QHBoxLayout* buttonLayout = new QHBoxLayout();
	buttonLayout->setAlignment(Qt::AlignCenter);

	QPushButton* exitButton = new QPushButton("Exit");
	exitButton->setFixedWidth(120);
	exitButton->setMinimumHeight(36);
	exitButton->setStyleSheet(
		"QPushButton {"
		"   background-color: transparent;"
		"   color: rgb(120, 120, 120);"
		"   border-radius: 6px;"
		"   padding: 6px 12px;"
		"   font-family: 'Outfit';"
		"   font-size: 13px;"
		"   border: none;"
		"}"
		"QPushButton:hover {"
		"   background-color: rgba(0, 0, 0, 8);"
		"   color: rgb(100, 100, 100);"
		"}"
		"QPushButton:pressed {"
		"   background-color: rgba(0, 0, 0, 15);"
		"}"
	);
	exitButton->setFont(font);

	buttonLayout->addWidget(exitButton);

	contentLayout->addWidget(m_updatingGifLabel);
	contentLayout->addWidget(m_updatingProgressLabel);
	contentLayout->addWidget(m_updatingLabel);
	contentLayout->addLayout(buttonLayout);

    connect(exitButton, &QPushButton::clicked, this, &DialogsController::exitButtonClicked);

	return dialog;
}

void DialogsController::swapUpdatingToRestarting()
{
	if (m_updatingGifLabel && m_updatingGifLabel->movie())
	{
		m_updatingGifLabel->movie()->stop();
	}

	if (m_updatingLabel)
	{
		m_updatingLabel->setText("Restarting...");
	}

	if (m_updatingProgressLabel)
	{
		m_updatingProgressLabel->setVisible(false);
	}
}

void DialogsController::swapUpdatingToUpToDate()
{
	if (m_updatingGifLabel && m_updatingGifLabel->movie())
	{
		m_updatingGifLabel->movie()->stop();
	}

	if (m_updatingLabel)
	{
		m_updatingLabel->setText("Already up to date");
	}

	if (m_updatingProgressLabel)
	{
		m_updatingProgressLabel->setVisible(false);
	}
}

QWidget* DialogsController::createConnectionErrorDialog(OverlayWidget* overlay)
{
	QFont font("Outfit", 14, QFont::Normal);

    QWidget* dialog = new QWidget(overlay);
    dialog->setMinimumWidth(520);
    dialog->setMinimumHeight(360);
    dialog->setAttribute(Qt::WA_TranslucentBackground);

	QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
	shadowEffect->setBlurRadius(30);
	shadowEffect->setXOffset(0);
	shadowEffect->setYOffset(0);
	shadowEffect->setColor(QColor(0, 0, 0, 150));

    QWidget* mainWidget = new QWidget(dialog);
	mainWidget->setGraphicsEffect(shadowEffect);
	mainWidget->setObjectName("mainWidget");

	QString mainWidgetStyle =
		"QWidget#mainWidget {"
		"   background-color: rgb(255, 240, 240);"
		"   border-radius: 16px;"
		"   border: 1px solid rgb(210, 210, 210);"
		"}";
	mainWidget->setStyleSheet(mainWidgetStyle);

    QVBoxLayout* mainLayout = new QVBoxLayout(dialog);
	mainLayout->addWidget(mainWidget);

	QVBoxLayout* contentLayout = new QVBoxLayout(mainWidget);
	contentLayout->setContentsMargins(32, 32, 32, 32);
	contentLayout->setSpacing(20);

	QLabel* imageLabel = new QLabel();
	imageLabel->setAlignment(Qt::AlignCenter);
	imageLabel->setMinimumHeight(140);

	QPixmap errorPixmap(":/resources/error.png");
	imageLabel->setPixmap(errorPixmap.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation));

	QLabel* errorLabel = new QLabel("Connection Error");
	errorLabel->setAlignment(Qt::AlignCenter);
	errorLabel->setStyleSheet(
		"color: rgb(180, 60, 60);"
		"font-size: 18px;"
		"font-family: 'Outfit';"
		"font-weight: bold;"
	);
	errorLabel->setFont(font);

	QLabel* messageLabel = new QLabel("An error occurred. Please restart and try again.");
	messageLabel->setAlignment(Qt::AlignCenter);
	messageLabel->setWordWrap(true);
	messageLabel->setStyleSheet(
		"color: rgb(100, 100, 100);"
		"font-size: 14px;"
		"font-family: 'Outfit';"
	);

	QHBoxLayout* buttonLayout = new QHBoxLayout();
	buttonLayout->setAlignment(Qt::AlignCenter);

	QPushButton* closeButton = new QPushButton("Close Calllifornia");
	closeButton->setFixedWidth(140);
	closeButton->setMinimumHeight(42);
	closeButton->setStyleSheet(
		"QPushButton {"
		"   background-color: rgba(180, 60, 60, 30);"
		"   color: rgb(180, 60, 60);"
		"   border-radius: 8px;"
		"   padding: 8px 16px;"
		"   font-family: 'Outfit';"
		"   font-size: 14px;"
		"   border: none;"
		"}"
		"QPushButton:hover {"
		"   background-color: rgba(180, 60, 60, 40);"
		"   color: rgb(160, 50, 50);"
		"}"
		"QPushButton:pressed {"
		"   background-color: rgba(180, 60, 60, 60);"
		"}"
	);
	closeButton->setFont(font);

	buttonLayout->addWidget(closeButton);

	contentLayout->addWidget(imageLabel);
	contentLayout->addWidget(errorLabel);
	contentLayout->addWidget(messageLabel);
	contentLayout->addLayout(buttonLayout);

    connect(closeButton, &QPushButton::clicked, this, &DialogsController::exitButtonClicked);

	return dialog;
}

