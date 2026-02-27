#include "meetingManagementDialog.h"
#include "utilities/utility.h"
#include "constants/color.h"
#include "widgets/mainMenuWidget.h"
#include "widgets/components/button.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QFont>
#include <QShortcut>
#include <QRandomGenerator>
#include <QIcon>
#include <QMovie>
#include <QPixmap>

MeetingManagementDialog::MeetingManagementDialog(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);

    const int shadowMargin = scale(24);

    QWidget* mainWidget = new QWidget(this);
    mainWidget->setObjectName("mainWidget");
    mainWidget->setStyleSheet(QString(
        "QWidget#mainWidget {"
        "   background-color: rgb(248, 250, 252);"
        "   border-radius: %1px;"
        "   border: %2px solid rgb(210, 210, 210);"
        "}")
        .arg(scale(16))
        .arg(scale(1)));
    mainWidget->setAutoFillBackground(true);

    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
    shadowEffect->setBlurRadius(scale(30));
    shadowEffect->setXOffset(0);
    shadowEffect->setYOffset(0);
    shadowEffect->setColor(COLOR_SHADOW_STRONG_150);
    mainWidget->setGraphicsEffect(shadowEffect);

    m_stackedWidget = new QStackedWidget(mainWidget);

    // ========== Initial state ==========
    m_initialWidget = new QWidget();
    QVBoxLayout* initialLayout = new QVBoxLayout(m_initialWidget);
    initialLayout->setContentsMargins(scale(32), scale(20), scale(32), scale(20));
    initialLayout->setSpacing(0);

    QFont titleFont("Outfit", scale(18), QFont::Bold);
    QFont sectionFont("Outfit", scale(12), QFont::Normal);

    QWidget* headerWidget = new QWidget();
    QVBoxLayout* headerBlockLayout = new QVBoxLayout(headerWidget);
    headerBlockLayout->setContentsMargins(0, 0, 0, 0);
    headerBlockLayout->setSpacing(scale(2));

    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(scale(6));
    QLabel* titleLabel = new QLabel("Meetings");
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    titleLabel->setStyleSheet(QString(
        "color: rgb(60, 60, 60);"
        "font-size: %1px;"
        "font-family: 'Outfit';"
        "font-weight: bold;"
        "padding: 0px;"
        "margin: 0px;")
        .arg(scale(18)));
    titleLabel->setFont(titleFont);

    m_meetingHeartsIcon = new QLabel();
    m_meetingHeartsIcon->setPixmap(QPixmap(":/resources/meetingHearts.png").scaled(scale(24), scale(24), Qt::KeepAspectRatio, Qt::SmoothTransformation));

    ButtonIcon* closeButton = new ButtonIcon(headerWidget, scale(28), scale(28));
    closeButton->setIcons(QIcon(":/resources/close.png"), QIcon(":/resources/closeHover.png"));
    closeButton->setSize(scale(28), scale(28));
    closeButton->setCursor(Qt::PointingHandCursor);

    headerLayout->addWidget(titleLabel);
    headerLayout->addWidget(m_meetingHeartsIcon);
    headerLayout->addStretch();
    headerLayout->addWidget(closeButton);

    QLabel* subtitleLabel = new QLabel("Start or join a video conference session.");
    subtitleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    subtitleLabel->setStyleSheet(QString(
        "color: rgb(100, 100, 100);"
        "font-size: %1px;"
        "font-family: 'Outfit';"
        "padding: 0px;"
        "margin: 0px;")
        .arg(scale(12)));
    headerBlockLayout->addLayout(headerLayout);
    headerBlockLayout->addWidget(subtitleLabel);

    // START NEW MEETING section
    QLabel* createSectionTitle = new QLabel("START NEW MEETING");
    createSectionTitle->setStyleSheet(QString(
        "color: rgb(80, 80, 80);"
        "font-size: %1px;"
        "font-family: 'Outfit';"
        "font-weight: bold;")
        .arg(scale(11)));
    createSectionTitle->setFont(sectionFont);

    QLabel* createDescription = new QLabel(
        "Create a new meeting. Instantly generate a unique meeting ID and start a secure session for your team.");
    createDescription->setWordWrap(true);
    createDescription->setStyleSheet(QString(
        "color: rgb(100, 100, 100);"
        "font-size: %1px;"
        "font-family: 'Outfit';")
        .arg(scale(12)));

    m_createMeetingButton = new QPushButton("Create Meeting");
    m_createMeetingButton->setCursor(Qt::PointingHandCursor);
    m_createMeetingButton->setFixedHeight(scale(44));
    m_createMeetingButton->setStyleSheet(QString(
        "QPushButton {"
        "   background-color: %1;"
        "   color: white;"
        "   border: none;"
        "   border-radius: %3px;"
        "   padding: %4px %5px;"
        "   margin: 0px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:focus { outline: none; border: none; }"
        "QPushButton:hover { background-color: %2; }")
        .arg(COLOR_ACCENT.name())
        .arg(COLOR_ACCENT_HOVER.name())
        .arg(scale(8))
        .arg(scale(12))
        .arg(scale(24)));

    QHBoxLayout* createRow = new QHBoxLayout();
    createRow->setSpacing(scale(18));
    createRow->addWidget(createDescription, 1);
    createRow->addWidget(m_createMeetingButton, 0, Qt::AlignRight);

    QVBoxLayout* createSection = new QVBoxLayout();
    createSection->setSpacing(scale(8));
    createSection->addWidget(createSectionTitle);
    createSection->addLayout(createRow);

    // JOIN EXISTING MEETING section
    QLabel* joinSectionTitle = new QLabel("JOIN EXISTING MEETING");
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
    m_meetingIdEdit->setStyleSheet(StyleMainMenuWidget::lineEditStyle());
    m_meetingIdEdit->setMaxLength(32);

    m_joinMeetingButton = new QPushButton("Join Meeting");
    m_joinMeetingButton->setCursor(Qt::PointingHandCursor);
    m_joinMeetingButton->setFixedHeight(scale(44));
    m_joinMeetingButton->setStyleSheet(QString(
        "QPushButton {"
        "   background-color: %1;"
        "   color: white;"
        "   border: none;"
        "   border-radius: %3px;"
        "   padding: %4px %5px;"
        "   margin: 0px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:focus { outline: none; border: none; }"
        "QPushButton:hover { background-color: %2; }")
        .arg(COLOR_ACCENT.name())
        .arg(COLOR_ACCENT_HOVER.name())
        .arg(scale(8))
        .arg(scale(12))
        .arg(scale(24)));

    QVBoxLayout* joinSection = new QVBoxLayout();
    joinSection->setSpacing(scale(8));
    joinSection->addWidget(joinSectionTitle);
    joinSection->addWidget(m_meetingIdEdit);
    joinSection->addWidget(m_joinMeetingButton);

    connect(closeButton, &ButtonIcon::clicked, this, [this]() { emit closeRequested(); });

    QFrame* separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet("background-color: rgb(210, 210, 210); max-height: 1px;");

    initialLayout->addWidget(headerWidget);
    initialLayout->addSpacing(scale(32));
    initialLayout->addStretch(1);
    initialLayout->addLayout(createSection);
    initialLayout->addSpacing(scale(32));
    initialLayout->addWidget(separator);
    initialLayout->addSpacing(scale(32));
    initialLayout->addLayout(joinSection);
    initialLayout->addStretch(1);

    connect(m_createMeetingButton, &QPushButton::clicked, this, [this]() {
        emit createMeetingRequested(generateMeetingUid());
    });
    connect(m_joinMeetingButton, &QPushButton::clicked, this, [this]() {
        QString uid = m_meetingIdEdit->text().trimmed();
        if (uid.isEmpty()) {
            return;
        }
        emit joinMeetingRequested(uid);
    });
    connect(m_meetingIdEdit, &QLineEdit::returnPressed, m_joinMeetingButton, &QPushButton::click);

    // ========== Connecting state ==========
    m_connectingWidget = new QWidget();
    QVBoxLayout* connectingLayout = new QVBoxLayout(m_connectingWidget);
    connectingLayout->setContentsMargins(scale(32), scale(32), scale(32), scale(32));
    connectingLayout->setSpacing(scale(20));

    QLabel* joiningTitle = new QLabel("Joining Meeting");
    joiningTitle->setAlignment(Qt::AlignCenter);
    joiningTitle->setStyleSheet(QString(
        "color: rgb(60, 60, 60);"
        "font-size: %1px;"
        "font-family: 'Outfit';"
        "font-weight: bold;")
        .arg(scale(18)));
    joiningTitle->setFont(titleFont);

    m_joinMeetingHeartsIcon = new QLabel();
    m_joinMeetingHeartsIcon->setPixmap(QPixmap(":/resources/joinMeetingHearts.png").scaled(scale(28), scale(28), Qt::KeepAspectRatio, Qt::SmoothTransformation));

    QHBoxLayout* joiningHeaderLayout = new QHBoxLayout();
    joiningHeaderLayout->setContentsMargins(0, 0, 0, 0);
    joiningHeaderLayout->setSpacing(scale(6));
    joiningHeaderLayout->addStretch();
    joiningHeaderLayout->addWidget(joiningTitle);
    joiningHeaderLayout->addWidget(m_joinMeetingHeartsIcon);
    joiningHeaderLayout->addStretch();

    m_roomIdLabel = new QLabel("Room ID: ");
    m_roomIdLabel->setAlignment(Qt::AlignCenter);
    m_roomIdLabel->setStyleSheet(QString(
        "color: rgb(80, 80, 80);"
        "font-size: %1px;"
        "font-family: 'Outfit';")
        .arg(scale(14)));

    m_waitingGifLabel = new QLabel();
    m_waitingGifLabel->setAlignment(Qt::AlignCenter);
    m_waitingGifLabel->setFixedSize(scale(40), scale(40));
    m_waitingGifLabel->setScaledContents(true);
    QMovie* waitingMovie = new QMovie(":/resources/waiting.gif");
    if (waitingMovie->isValid())
    {
        m_waitingGifLabel->setMovie(waitingMovie);
        waitingMovie->start();
    }

    m_statusLabel = new QLabel("Waiting for host's approval...");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet(QString(
        "color: rgb(100, 100, 100);"
        "font-size: %1px;"
        "font-family: 'Outfit';")
        .arg(scale(12)));

    QLabel* waitLabel = new QLabel(
        "Your request to join has been sent. The meeting host will allow you to enter when ready.");
    waitLabel->setWordWrap(true);
    waitLabel->setAlignment(Qt::AlignCenter);
    waitLabel->setStyleSheet(QString(
        "color: rgb(120, 120, 120);"
        "font-size: %1px;"
        "font-family: 'Outfit';")
        .arg(scale(11)));

    m_cancelRequestButton = new QPushButton("Cancel Request");
    m_cancelRequestButton->setCursor(Qt::PointingHandCursor);
    m_cancelRequestButton->setFixedHeight(scale(56));
    m_cancelRequestButton->setStyleSheet(QString(
        "QPushButton {"
        "   background-color: rgb(235, 238, 242);"
        "   color: rgb(80, 80, 80);"
        "   border: none;"
        "   border-bottom-left-radius: %1px;"
        "   border-bottom-right-radius: %1px;"
        "   padding: %2px %3px;"
        "   font-family: 'Outfit';"
        "   font-size: %4px;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgb(250, 240, 240);"
        "   color: rgb(180, 60, 60);"
        "}"
        "QPushButton:pressed {"
        "   background-color: rgb(245, 235, 235);"
        "}")
        .arg(scale(16)).arg(scale(12)).arg(scale(24)).arg(scale(14)));
    m_cancelRequestButton->hide();

    connectingLayout->addLayout(joiningHeaderLayout);
    connectingLayout->addWidget(m_roomIdLabel);
    connectingLayout->addSpacing(scale(16));
    connectingLayout->addWidget(m_waitingGifLabel, 0, Qt::AlignHCenter);
    connectingLayout->addWidget(m_statusLabel);
    connectingLayout->addWidget(waitLabel);
    connectingLayout->addStretch();

    connect(m_cancelRequestButton, &QPushButton::clicked, this, [this]() {
        emit joinCancelled();
    });

    m_stackedWidget->addWidget(m_initialWidget);
    m_stackedWidget->addWidget(m_connectingWidget);
    m_stackedWidget->setCurrentWidget(m_initialWidget);

    QVBoxLayout* mainWidgetLayout = new QVBoxLayout(mainWidget);
    mainWidgetLayout->setContentsMargins(0, 0, 0, 0);
    mainWidgetLayout->addWidget(m_stackedWidget);
    mainWidgetLayout->addWidget(m_cancelRequestButton);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(shadowMargin, shadowMargin, shadowMargin, shadowMargin);
    mainLayout->addWidget(mainWidget);

    m_initialHeight = scale(440) + shadowMargin * 2;
    m_connectingHeight = scale(320) + shadowMargin * 2;
    setFixedSize(scale(520) + shadowMargin * 2, m_initialHeight);

    auto* escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this, [this]() {
        if (m_stackedWidget->currentWidget() == m_connectingWidget) {
            emit joinCancelled();
        } else {
            emit closeRequested();
        }
    });
    escShortcut->setContext(Qt::ApplicationShortcut);
}

QString MeetingManagementDialog::generateMeetingUid() const
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

void MeetingManagementDialog::showConnectingState(const QString& roomId)
{
    m_roomIdLabel->setText("Room ID: " + roomId);
    m_statusLabel->setText("Waiting for host's approval...");
    if (m_waitingGifLabel && m_waitingGifLabel->movie())
    {
        m_waitingGifLabel->movie()->start();
    }
    m_cancelRequestButton->show();
    m_stackedWidget->setCurrentWidget(m_connectingWidget);
    setFixedHeight(m_connectingHeight);
}

void MeetingManagementDialog::showInitialState()
{
    m_meetingIdEdit->clear();
    m_cancelRequestButton->hide();
    m_stackedWidget->setCurrentWidget(m_initialWidget);
    setFixedHeight(m_initialHeight);
}

void MeetingManagementDialog::setJoinStatus(const QString& status)
{
    m_statusLabel->setText(status);
}
