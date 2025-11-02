#include "dialogsController.h"
#include "overlayWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMovie>
#include <QPixmap>
#include <QGraphicsDropShadowEffect>
#include <QScreen>
#include <QGuiApplication>
#include <QFont>

DialogsController::DialogsController(QWidget* parent)
	: QObject(parent), m_parent(parent), m_updatingOverlay(nullptr), m_updatingDialog(nullptr),
	m_updatingProgressLabel(nullptr), m_updatingLabel(nullptr), m_updatingGifLabel(nullptr),
	m_connectionErrorOverlay(nullptr), m_connectionErrorDialog(nullptr)
{
}

DialogsController::~DialogsController()
{
	hideUpdatingDialog();
	hideUpdatingErrorDialog();
}

void DialogsController::showUpdatingDialog()
{
	if (m_updatingDialog)
	{
		return;
	}

	m_updatingOverlay = new OverlayWidget(m_parent);
	m_updatingOverlay->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
	m_updatingOverlay->setAttribute(Qt::WA_TranslucentBackground);
	m_updatingOverlay->showMaximized();

	m_updatingDialog = createUpdatingDialog(m_updatingOverlay);

	QObject::connect(m_updatingDialog, &QDialog::finished, m_updatingOverlay, &QWidget::deleteLater);
	QObject::connect(m_updatingDialog, &QDialog::finished, this, [this]()
	{
		m_updatingDialog = nullptr;
		m_updatingOverlay = nullptr;
		m_updatingProgressLabel = nullptr;
		m_updatingLabel = nullptr;
		m_updatingGifLabel = nullptr;
	});

	QScreen* targetScreen = qobject_cast<QWidget*>(parent())->screen();
	if (!targetScreen)
	{
		targetScreen = QGuiApplication::primaryScreen();
	}

	QRect screenGeometry = targetScreen->availableGeometry();
	m_updatingDialog->adjustSize();
	QSize dialogSize = m_updatingDialog->size();

	int x = screenGeometry.center().x() - dialogSize.width() / 2;
	int y = screenGeometry.center().y() - dialogSize.height() / 2;
	m_updatingDialog->move(x, y);

	m_updatingDialog->exec();
}

void DialogsController::hideUpdatingDialog()
{
	if (m_updatingDialog)
	{
		m_updatingDialog->disconnect();
		m_updatingDialog->close();
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

void DialogsController::showConnectionErrorDialog()
{
	if (m_connectionErrorDialog)
	{
		return;
	}

	m_connectionErrorOverlay = new OverlayWidget(m_parent);
	m_connectionErrorOverlay->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
	m_connectionErrorOverlay->setAttribute(Qt::WA_TranslucentBackground);
	m_connectionErrorOverlay->showMaximized();

	m_connectionErrorDialog = createConnectionErrorDialog(m_connectionErrorOverlay);

	QObject::connect(m_connectionErrorDialog, &QDialog::finished, m_connectionErrorOverlay, &QWidget::deleteLater);
	QObject::connect(m_connectionErrorDialog, &QDialog::finished, this, [this]()
	{
		m_connectionErrorDialog = nullptr;
		m_connectionErrorOverlay = nullptr;
	});

	QScreen* targetScreen = qobject_cast<QWidget*>(parent())->screen();
	if (!targetScreen)
	{
		targetScreen = QGuiApplication::primaryScreen();
	}

	QRect screenGeometry = targetScreen->availableGeometry();
	m_connectionErrorDialog->adjustSize();
	QSize dialogSize = m_connectionErrorDialog->size();

	int x = screenGeometry.center().x() - dialogSize.width() / 2;
	int y = screenGeometry.center().y() - dialogSize.height() / 2;
	m_connectionErrorDialog->move(x, y);

	m_connectionErrorDialog->exec();
}

void DialogsController::hideUpdatingErrorDialog()
{
	if (m_connectionErrorDialog)
	{
		m_connectionErrorDialog->disconnect();
		m_connectionErrorDialog->close();
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

QDialog* DialogsController::createUpdatingDialog(OverlayWidget* overlay)
{
	QFont font("Outfit", 14, QFont::Normal);

	QDialog* dialog = new QDialog(overlay);
	dialog->setWindowTitle("Updating");
	dialog->setMinimumWidth(300);
	dialog->setMinimumHeight(250);
	dialog->setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
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

QDialog* DialogsController::createConnectionErrorDialog(OverlayWidget* overlay)
{
	QFont font("Outfit", 14, QFont::Normal);

	QDialog* dialog = new QDialog(overlay);
	dialog->setWindowTitle("Update Error");
	dialog->setMinimumWidth(520);
	dialog->setMinimumHeight(360);
	dialog->setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
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

