#include "groupCallManagementDialog.h"
#include "utilities/utility.h"
#include "constants/color.h"
#include "widgets/mainMenuWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QFont>
#include <QRegularExpressionValidator>
#include <QShortcut>
#include <QRandomGenerator>
#include <QIcon>

GroupCallManagementDialog::GroupCallManagementDialog(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);

    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
    shadowEffect->setBlurRadius(scale(30));
    shadowEffect->setXOffset(0);
    shadowEffect->setYOffset(0);
    shadowEffect->setColor(QColor(0, 0, 0, 150));

    QWidget* mainWidget = new QWidget(this);
    mainWidget->setGraphicsEffect(shadowEffect);
    mainWidget->setObjectName("mainWidget");
    mainWidget->setStyleSheet(QString(
        "QWidget#mainWidget {"
        "   background-color: rgb(248, 250, 252);"
        "   border-radius: %1px;"
        "   border: %2px solid rgb(210, 210, 210);"
        "}")
        .arg(scale(16))
        .arg(scale(1)));

    m_stackedWidget = new QStackedWidget(mainWidget);

    // ========== Initial state ==========
    m_initialWidget = new QWidget();
    QVBoxLayout* initialLayout = new QVBoxLayout(m_initialWidget);
    initialLayout->setContentsMargins(scale(32), scale(32), scale(32), scale(32));
    initialLayout->setSpacing(scale(24));

    QFont titleFont("Outfit", scale(18), QFont::Bold);
    QFont sectionFont("Outfit", scale(12), QFont::Normal);

    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->addStretch();
    QPushButton* closeButton = new QPushButton();
    closeButton->setFixedSize(scale(32), scale(32));
    closeButton->setCursor(Qt::PointingHandCursor);
    closeButton->setStyleSheet(QString(
        "QPushButton {"
        "   background-color: transparent;"
        "   border: none;"
        "   border-radius: %1px;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgba(0, 0, 0, 0.08);"
        "}")
        .arg(scale(8)));
    closeButton->setIcon(QIcon(":/resources/close.png"));
    closeButton->setIconSize(QSize(scale(16), scale(16)));
    headerLayout->addWidget(closeButton);

    QLabel* titleLabel = new QLabel("Call Management");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(QString(
        "color: rgb(60, 60, 60);"
        "font-size: %1px;"
        "font-family: 'Outfit';"
        "font-weight: bold;"
        "padding: %2px;")
        .arg(scale(18)).arg(scale(5)));
    titleLabel->setFont(titleFont);

    QLabel* subtitleLabel = new QLabel("Start or join a video conference session.");
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setStyleSheet(QString(
        "color: rgb(100, 100, 100);"
        "font-size: %1px;"
        "font-family: 'Outfit';")
        .arg(scale(12)));

    // START NEW CALL section
    QLabel* createSectionTitle = new QLabel("START NEW CALL");
    createSectionTitle->setStyleSheet(QString(
        "color: rgb(80, 80, 80);"
        "font-size: %1px;"
        "font-family: 'Outfit';"
        "font-weight: bold;")
        .arg(scale(11)));
    createSectionTitle->setFont(sectionFont);

    QLabel* createDescription = new QLabel(
        "Create a new group call. Instantly generate a unique meeting ID and start a secure session for your team.");
    createDescription->setWordWrap(true);
    createDescription->setStyleSheet(QString(
        "color: rgb(100, 100, 100);"
        "font-size: %1px;"
        "font-family: 'Outfit';")
        .arg(scale(12)));

    m_createCallButton = new QPushButton("Create Call");
    m_createCallButton->setCursor(Qt::PointingHandCursor);
    m_createCallButton->setFixedHeight(scale(44));
    m_createCallButton->setStyleSheet(QString(
        "QPushButton {"
        "   background-color: rgb(21, 119, 232);"
        "   color: white;"
        "   border: none;"
        "   border-radius: %1px;"
        "   padding: %2px %3px;"
        "   font-family: 'Outfit';"
        "   font-size: %4px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgb(18, 113, 222);"
        "}"
        "QPushButton:pressed {"
        "   background-color: rgb(16, 103, 202);"
        "}")
        .arg(scale(10)).arg(scale(12)).arg(scale(24)).arg(scale(14)));

    QHBoxLayout* createRow = new QHBoxLayout();
    createRow->addWidget(createDescription, 1);
    createRow->addWidget(m_createCallButton, 0, Qt::AlignRight);

    QVBoxLayout* createSection = new QVBoxLayout();
    createSection->setSpacing(scale(8));
    createSection->addWidget(createSectionTitle);
    createSection->addLayout(createRow);

    // Separator
    QFrame* separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet("background-color: rgb(210, 210, 210); max-height: 1px;");
    QLabel* orLabel = new QLabel("OR JOIN EXISTING");
    orLabel->setAlignment(Qt::AlignCenter);
    orLabel->setStyleSheet(QString(
        "color: rgb(120, 120, 120);"
        "font-size: %1px;"
        "font-family: 'Outfit';")
        .arg(scale(11)));

    // JOIN EXISTING CALL section
    QLabel* joinSectionTitle = new QLabel("JOIN EXISTING CALL");
    joinSectionTitle->setStyleSheet(QString(
        "color: rgb(80, 80, 80);"
        "font-size: %1px;"
        "font-family: 'Outfit';"
        "font-weight: bold;")
        .arg(scale(11)));
    joinSectionTitle->setFont(sectionFont);

    m_meetingIdEdit = new QLineEdit();
    m_meetingIdEdit->setPlaceholderText("Paste meeting ID (e.g., abc-defg-hij)");
    m_meetingIdEdit->setFixedHeight(scale(50));
    QString lineEditStyle = StyleMainMenuWidget::lineEditStyle();
    QRegularExpression re("(QLineEdit \\{[^}]*background-color: )rgba\\([^)]+\\)(;[^}]*\\})");
    lineEditStyle.replace(re, "\\1rgba(255, 255, 255, 0.9)\\2");
    m_meetingIdEdit->setStyleSheet(lineEditStyle);
    m_meetingIdEdit->setMaxLength(32);

    m_joinMeetingButton = new QPushButton("Join Meeting");
    m_joinMeetingButton->setCursor(Qt::PointingHandCursor);
    m_joinMeetingButton->setFixedHeight(scale(44));
    m_joinMeetingButton->setStyleSheet(QString(
        "QPushButton {"
        "   background-color: white;"
        "   color: rgb(60, 60, 60);"
        "   border: 1px solid rgb(210, 210, 210);"
        "   border-radius: %1px;"
        "   padding: %2px %3px;"
        "   font-family: 'Outfit';"
        "   font-size: %4px;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgb(245, 245, 245);"
        "   border-color: rgb(180, 180, 180);"
        "}"
        "QPushButton:pressed {"
        "   background-color: rgb(235, 235, 235);"
        "}")
        .arg(scale(10)).arg(scale(12)).arg(scale(24)).arg(scale(14)));

    QVBoxLayout* joinSection = new QVBoxLayout();
    joinSection->setSpacing(scale(8));
    joinSection->addWidget(joinSectionTitle);
    joinSection->addWidget(m_meetingIdEdit);
    joinSection->addWidget(m_joinMeetingButton);

    connect(closeButton, &QPushButton::clicked, this, [this]() { emit closeRequested(); });

    initialLayout->addLayout(headerLayout);
    initialLayout->addWidget(titleLabel);
    initialLayout->addWidget(subtitleLabel);
    initialLayout->addSpacing(scale(8));
    initialLayout->addLayout(createSection);
    initialLayout->addWidget(separator);
    initialLayout->addWidget(orLabel);
    initialLayout->addLayout(joinSection);

    connect(m_createCallButton, &QPushButton::clicked, this, [this]() {
        emit createCallRequested(generateCallUid());
    });
    connect(m_joinMeetingButton, &QPushButton::clicked, this, [this]() {
        QString uid = m_meetingIdEdit->text().trimmed();
        if (uid.isEmpty()) {
            return;
        }
        emit joinCallRequested(uid);
    });
    connect(m_meetingIdEdit, &QLineEdit::returnPressed, m_joinMeetingButton, &QPushButton::click);

    // ========== Connecting state ==========
    m_connectingWidget = new QWidget();
    QVBoxLayout* connectingLayout = new QVBoxLayout(m_connectingWidget);
    connectingLayout->setContentsMargins(scale(32), scale(32), scale(32), scale(32));
    connectingLayout->setSpacing(scale(20));

    QLabel* joiningTitle = new QLabel("Joining Call");
    joiningTitle->setAlignment(Qt::AlignCenter);
    joiningTitle->setStyleSheet(QString(
        "color: rgb(60, 60, 60);"
        "font-size: %1px;"
        "font-family: 'Outfit';"
        "font-weight: bold;")
        .arg(scale(18)));
    joiningTitle->setFont(titleFont);

    m_roomIdLabel = new QLabel("Room ID: ");
    m_roomIdLabel->setAlignment(Qt::AlignCenter);
    m_roomIdLabel->setStyleSheet(QString(
        "color: rgb(80, 80, 80);"
        "font-size: %1px;"
        "font-family: 'Outfit';")
        .arg(scale(14)));

    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    m_progressBar->setFormat("%p%");
    m_progressBar->setFixedHeight(scale(12));
    m_progressBar->setStyleSheet(QString(
        "QProgressBar {"
        "   border: none;"
        "   border-radius: %1px;"
        "   background-color: rgb(230, 230, 230);"
        "   text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "   background-color: rgb(21, 119, 232);"
        "   border-radius: %1px;"
        "}")
        .arg(scale(6)));

    m_statusLabel = new QLabel("Connecting to call...");
    m_statusLabel->setStyleSheet(QString(
        "color: rgb(100, 100, 100);"
        "font-size: %1px;"
        "font-family: 'Outfit';")
        .arg(scale(12)));

    QLabel* waitLabel = new QLabel(
        "Please wait while we establish a secure end-to-end encrypted connection. This usually takes a few seconds.");
    waitLabel->setWordWrap(true);
    waitLabel->setAlignment(Qt::AlignCenter);
    waitLabel->setStyleSheet(QString(
        "color: rgb(120, 120, 120);"
        "font-size: %1px;"
        "font-family: 'Outfit';")
        .arg(scale(11)));

    m_cancelRequestButton = new QPushButton("Cancel Request");
    m_cancelRequestButton->setCursor(Qt::PointingHandCursor);
    m_cancelRequestButton->setFixedHeight(scale(44));
    m_cancelRequestButton->setStyleSheet(QString(
        "QPushButton {"
        "   background-color: white;"
        "   color: rgb(100, 100, 100);"
        "   border: 1px solid rgb(210, 210, 210);"
        "   border-radius: %1px;"
        "   padding: %2px %3px;"
        "   font-family: 'Outfit';"
        "   font-size: %4px;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgb(250, 240, 240);"
        "   color: rgb(180, 60, 60);"
        "   border-color: rgb(220, 180, 180);"
        "}"
        "QPushButton:pressed {"
        "   background-color: rgb(245, 235, 235);"
        "}")
        .arg(scale(10)).arg(scale(12)).arg(scale(24)).arg(scale(14)));

    connectingLayout->addWidget(joiningTitle);
    connectingLayout->addWidget(m_roomIdLabel);
    connectingLayout->addSpacing(scale(8));
    connectingLayout->addWidget(m_progressBar);
    connectingLayout->addWidget(m_statusLabel);
    connectingLayout->addWidget(waitLabel);
    connectingLayout->addStretch();
    connectingLayout->addWidget(m_cancelRequestButton);

    connect(m_cancelRequestButton, &QPushButton::clicked, this, [this]() {
        emit joinCancelled();
    });

    m_stackedWidget->addWidget(m_initialWidget);
    m_stackedWidget->addWidget(m_connectingWidget);
    m_stackedWidget->setCurrentWidget(m_initialWidget);

    QVBoxLayout* mainWidgetLayout = new QVBoxLayout(mainWidget);
    mainWidgetLayout->setContentsMargins(0, 0, 0, 0);
    mainWidgetLayout->addWidget(m_stackedWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(mainWidget);

    setFixedSize(scale(520), scale(480));

    auto* escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this, [this]() {
        if (m_stackedWidget->currentWidget() == m_connectingWidget) {
            emit joinCancelled();
        } else {
            emit closeRequested();
        }
    });
    escShortcut->setContext(Qt::ApplicationShortcut);
}

QString GroupCallManagementDialog::generateCallUid() const
{
    const QString chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    auto randomChar = [&chars]() {
        return chars[QRandomGenerator::global()->bounded(chars.size())];
    };
    return QString("%1%2%3-%4%5%6%7%8-%9%10%11")
        .arg(randomChar()).arg(randomChar()).arg(randomChar())
        .arg(randomChar()).arg(randomChar()).arg(randomChar()).arg(randomChar()).arg(randomChar())
        .arg(randomChar()).arg(randomChar()).arg(randomChar());
}

void GroupCallManagementDialog::showConnectingState(const QString& roomId)
{
    m_roomIdLabel->setText("Room ID: " + roomId);
    m_progressBar->setValue(0);
    m_statusLabel->setText("Connecting to call...");
    m_stackedWidget->setCurrentWidget(m_connectingWidget);
}

void GroupCallManagementDialog::showInitialState()
{
    m_meetingIdEdit->clear();
    m_stackedWidget->setCurrentWidget(m_initialWidget);
}

void GroupCallManagementDialog::setJoinProgress(int percent)
{
    m_progressBar->setValue(qBound(0, percent, 100));
}

void GroupCallManagementDialog::setJoinStatus(const QString& status)
{
    m_statusLabel->setText(status);
}
