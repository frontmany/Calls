#include "widgets/meetingWidget.h"
#include <QGraphicsDropShadowEffect>
#include <QDialog>
#include <cmath>
#include <algorithm>
#include <string>
#include <QPainter>
#include <QPolygon>
#include <QClipboard>
#include <QGuiApplication>
#include <QResizeEvent>

#include "widgets/meetingParticipantWidget.h"
#include "utilities/utility.h"

// Style definitions
const QColor StyleMeetingWidget::m_primaryColor = QColor(21, 119, 232);
const QColor StyleMeetingWidget::m_hoverColor = QColor(18, 113, 222);
const QColor StyleMeetingWidget::m_backgroundColor = QColor(245, 245, 247);
const QColor StyleMeetingWidget::m_textColor = QColor(1, 11, 19);
const QColor StyleMeetingWidget::m_containerColor = QColor(255, 255, 255, 50);
const QColor StyleMeetingWidget::m_whiteColor = QColor(255, 255, 255);
const QColor StyleMeetingWidget::m_controlButtonColor = QColor(255, 255, 255, 180);
const QColor StyleMeetingWidget::m_controlButtonHoverColor = QColor(255, 255, 255, 220);
const QColor StyleMeetingWidget::m_hangupButtonColor = QColor(232, 53, 53);
const QColor StyleMeetingWidget::m_hangupButtonHoverColor = QColor(212, 43, 43);
const QColor StyleMeetingWidget::m_sliderGrooveColor = QColor(77, 77, 77);
const QColor StyleMeetingWidget::m_sliderHandleColor = QColor(255, 255, 255);
const QColor StyleMeetingWidget::m_sliderSubPageColor = QColor(21, 119, 232);
const QColor StyleMeetingWidget::m_volumeLabelColor = QColor(51, 51, 51);
const QColor StyleMeetingWidget::m_scrollAreaBackgroundColor = QColor(0, 0, 0, 0);
const QColor StyleMeetingWidget::m_sliderContainerColor = QColor(255, 255, 255, 120);
const QColor StyleMeetingWidget::m_participantContainerColor = QColor(255, 255, 255, 180);
const QColor StyleMeetingWidget::m_participantBorderColor = QColor(220, 220, 220);
const QColor StyleMeetingWidget::m_joinRequestPanelBackgroundColor = QColor(255, 255, 255, 250);
const QColor StyleMeetingWidget::m_joinRequestPanelBorderColor = QColor(200, 200, 200, 204);
const QColor StyleMeetingWidget::m_acceptButtonColor = QColor(21, 119, 232);
const QColor StyleMeetingWidget::m_acceptButtonHoverColor = QColor(18, 104, 208);
const QColor StyleMeetingWidget::m_declineButtonColor = QColor(232, 53, 53);
const QColor StyleMeetingWidget::m_declineButtonHoverColor = QColor(212, 43, 43);

QString StyleMeetingWidget::containerStyle() {
    return QString("QWidget {"
        "   background-color: transparent;"
        "   border-radius: 20px;"
        "   padding: 0px;"
        "}");
}

QString StyleMeetingWidget::sliderContainerStyle() {
    return QString("#slidersContainer {"
        "   background-color: rgba(%1, %2, %3, %4);"
        "   border-radius: %5px;"
        "   padding: %6px;"
        "   margin: %7px;"
        "}").arg(m_sliderContainerColor.red())
        .arg(m_sliderContainerColor.green())
        .arg(m_sliderContainerColor.blue())
        .arg(m_sliderContainerColor.alpha())
        .arg(QString::fromStdString(std::to_string(scale(15))))
        .arg(QString::fromStdString(std::to_string(scale(20))))
        .arg(QString::fromStdString(std::to_string(scale(10))));
}

QString StyleMeetingWidget::titleStyle() {
    return QString("QLabel {"
        "   color: %1;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}").arg(m_textColor.name());
}

QString StyleMeetingWidget::timerStyle() {
    return QString("QLabel {"
        "   color: %1;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}").arg(m_whiteColor.name());
}

QString StyleMeetingWidget::controlButtonStyle() {
    return QString("QPushButton {"
        "   background-color: rgba(%1, %2, %3, %4);"
        "   border: none;"
        "   border-radius: %9px;"
        "   padding: %10px;"
        "   margin: %11px;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgba(%5, %6, %7, %8);"
        "}").arg(m_controlButtonColor.red()).arg(m_controlButtonColor.green())
        .arg(m_controlButtonColor.blue()).arg(m_controlButtonColor.alpha())
        .arg(m_controlButtonHoverColor.red()).arg(m_controlButtonHoverColor.green())
        .arg(m_controlButtonHoverColor.blue()).arg(m_controlButtonHoverColor.alpha())
        .arg(QString::fromStdString(std::to_string(scale(25))))
        .arg(QString::fromStdString(std::to_string(scale(15))))
        .arg(QString::fromStdString(std::to_string(scale(5))));
}

QString StyleMeetingWidget::hangupButtonStyle() {
    return QString("QPushButton {"
        "   background-color: %1;"
        "   border: none;"
        "   border-radius: %3px;"
        "   padding: %4px;"
        "   margin: %5px;"
        "}"
        "QPushButton:focus {"
        "   outline: none;"
        "   border: none;"
        "}"
        "QPushButton:hover {"
        "   background-color: %2;"
        "}").arg(m_hangupButtonColor.name()).arg(m_hangupButtonHoverColor.name())
        .arg(QString::fromStdString(std::to_string(scale(25))))
        .arg(QString::fromStdString(std::to_string(scale(15))))
        .arg(QString::fromStdString(std::to_string(scale(5))));
}

QString StyleMeetingWidget::disabledHangupButtonStyle() {
    return QString("QPushButton {"
        "   background-color: rgba(%1, %2, %3, 120);"
        "   border: none;"
        "   border-radius: %4px;"
        "   padding: %5px;"
        "   margin: %6px;"
        "   opacity: 0.6;"
        "}").arg(m_hangupButtonColor.red()).arg(m_hangupButtonColor.green()).arg(m_hangupButtonColor.blue())
        .arg(QString::fromStdString(std::to_string(scale(25))))
        .arg(QString::fromStdString(std::to_string(scale(15))))
        .arg(QString::fromStdString(std::to_string(scale(5))));
}

QString StyleMeetingWidget::panelStyle() {
    return QString("QWidget {"
        "   background-color: transparent;"
        "   border-radius: %1px;"
        "   margin: %2px;"
        "   padding: %3px;"
        "}")
        .arg(QString::fromStdString(std::to_string(scale(25))))
        .arg(QString::fromStdString(std::to_string(scale(5))))
        .arg(QString::fromStdString(std::to_string(scale(15))));
}

QString StyleMeetingWidget::volumeLabelStyle() {
    return QString(
        "QLabel {"
        "   color: %1;"
        "   font-size: %2px;"
        "   font-weight: bold;"
        "   margin: %3px %4px;"
        "}"
    ).arg(m_volumeLabelColor.name())
        .arg(QString::fromStdString(std::to_string(scale(12))))
        .arg(QString::fromStdString(std::to_string(scale(2))))
        .arg(QString::fromStdString(std::to_string(scale(0))));
}

QString StyleMeetingWidget::scrollAreaStyle() {
    return QString(
        "QScrollArea {"
        "   background-color: transparent;"
        "   border: none;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}"
        "QScrollArea > QWidget > QWidget {"
        "   background-color: transparent;"
        "}"
    );
}

QString StyleMeetingWidget::volumeSliderStyle() {
    return QString(R"(
        QSlider::groove:horizontal {
            height: %1px; 
            border-radius: %2px;
            margin: 0px 0px; 
        }
        QSlider::handle:horizontal {
            background-color: white;
            width: %3px;
            height: %4px; 
            border-radius: %5px;
            margin: -4px 0;
        }
        QSlider::add-page:horizontal {
            background-color: rgb(77, 77, 77);
            border-radius: %6px;
        }
        QSlider::sub-page:horizontal {
            background-color: rgb(21, 119, 232);
            border-radius: %6px;
        }
        QSlider::disabled {
            background-color: transparent;
        }
        QSlider::groove:horizontal:disabled {
            background-color: rgb(180, 180, 180);
        }
        QSlider::handle:horizontal:disabled {
            background-color: rgb(230, 230, 230);
        }
        QSlider::add-page:horizontal:disabled {
            background-color: rgb(180, 180, 180);
        }
        QSlider::sub-page:horizontal:disabled {
            background-color: rgb(150, 150, 150);
        }
    )")
        .arg(QString::fromStdString(std::to_string(scale(8))))
        .arg(QString::fromStdString(std::to_string(scale(4))))
        .arg(QString::fromStdString(std::to_string(scale(17))))
        .arg(QString::fromStdString(std::to_string(scale(17))))
        .arg(QString::fromStdString(std::to_string(scale(8))))
        .arg(QString::fromStdString(std::to_string(scale(4))));
}

QString StyleMeetingWidget::notificationRedLabelStyle() {
    return QString("QWidget {"
        "   background-color: rgba(252, 121, 121, 100);"
        "   border: none;"
        "   border-radius: %1px;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}").arg(QString::fromStdString(std::to_string(scale(8))));
}

QString StyleMeetingWidget::participantsScrollAreaStyle() {
    int paddingRight = scale(8);

    return QString(R"(
        QScrollArea {
            background-color: transparent;
            border: none;
            margin: 0px;
            padding: 0px;
        }
        
        QScrollArea > QWidget > QWidget {
            background-color: transparent;
            padding-right: %1px;
        }
    )").arg(paddingRight);
}

QString StyleMeetingWidget::participantsScrollBarStyle() {
    return QString(R"(
        QScrollBar:vertical {
            border: 2px solid rgb(250, 250, 250);      
            background: rgb(250, 250, 250);        
            width: 14px;                 
            margin: 0px;
            border-radius: 7px; 
        }

        QScrollBar::handle:vertical {
            background: rgb(218, 219, 227);   
            border: 2px solid rgb(218, 219, 227);      
            border-radius: 1px;           
            min-height: 30px;  /* пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ! */
            margin: 2px;
        }

        QScrollBar::add-line:vertical, 
        QScrollBar::sub-line:vertical { 
            height: 0px;
            background: none;             
        }
        
        QScrollBar::add-page:vertical,
        QScrollBar::sub-page:vertical {
            background: none;
        }
    )");
}

QString StyleMeetingWidget::participantContainerStyle() {
    return QString(R"(
        QWidget {
            background-color: rgba(%1, %2, %3, %4);
            border: 1px solid rgba(%5, %6, %7, 100);
            border-radius: %8px;
            margin: %9px;
            padding: %10px;
        }
    )").arg(m_participantContainerColor.red())
        .arg(m_participantContainerColor.green())
        .arg(m_participantContainerColor.blue())
        .arg(m_participantContainerColor.alpha())
        .arg(m_participantBorderColor.red())
        .arg(m_participantBorderColor.green())
        .arg(m_participantBorderColor.blue())
        .arg(QString::fromStdString(std::to_string(scale(12))))
        .arg(QString::fromStdString(std::to_string(scale(5))))
        .arg(QString::fromStdString(std::to_string(scale(8))));
}

QString StyleMeetingWidget::participantNameStyle()
{
    return QString(R"(
        QLabel {
            color: %1;
            font-size: %2px;
            font-weight: bold;
            margin: 0px;
            padding: 2px 0px;
            background-color: rgba(255, 255, 255, 180);
            border-radius: 4px;
        }
    )").arg(m_textColor.name())
        .arg(QString::fromStdString(std::to_string(scale(12))));
}

QString StyleMeetingWidget::joinRequestPanelStyle()
{
    return QString(
        "QWidget#joinRequestsPanel {"
        "   background: rgba(%1, %2, %3, %4);"
        "   border: 1px solid rgba(%5, %6, %7, %8);"
        "   border-radius: %9px;"
        "}"
        "QLabel {"
        "   color: #2C2C2C;"
        "   background: transparent;"
        "}")
        .arg(m_joinRequestPanelBackgroundColor.red())
        .arg(m_joinRequestPanelBackgroundColor.green())
        .arg(m_joinRequestPanelBackgroundColor.blue())
        .arg(m_joinRequestPanelBackgroundColor.alpha())
        .arg(m_joinRequestPanelBorderColor.red())
        .arg(m_joinRequestPanelBorderColor.green())
        .arg(m_joinRequestPanelBorderColor.blue())
        .arg(m_joinRequestPanelBorderColor.alpha())
        .arg(QString::fromStdString(std::to_string(scale(14))));
}

QString StyleMeetingWidget::joinRequestItemStyle()
{
    return QString(
        "QWidget#joinRequestItem {"
        "   background: transparent;"
        "   border: none;"
        "}");
}

QString StyleMeetingWidget::joinRequestScrollAreaStyle()
{
    return QString(
        "QScrollArea {"
        "   background: transparent;"
        "   border: none;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}"
        "QScrollArea > QWidget > QWidget {"
        "   background: transparent;"
        "}");
}

QString StyleMeetingWidget::joinRequestScrollBarStyle()
{
    return QString(
        "QScrollBar:vertical {"
        "   background: rgba(240, 240, 240, 0.3);"
        "   width: %1px;"
        "   margin: 0px;"
        "   border-radius: %2px;"
        "}"
        "QScrollBar::handle:vertical {"
        "   background: rgba(160, 160, 160, 0.6);"
        "   min-height: %3px;"
        "   border-radius: %2px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "   background: rgba(140, 140, 140, 0.8);"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "   height: 0px;"
        "}"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
        "   background: none;"
        "}")
        .arg(QString::fromStdString(std::to_string(scale(6))))
        .arg(QString::fromStdString(std::to_string(scale(3))))
        .arg(QString::fromStdString(std::to_string(scale(20))));
}

QString StyleMeetingWidget::joinRequestContentContainerStyle()
{
    return QString(
        "QWidget {"
        "   background: rgba(240, 244, 248, 180);"
        "   border-top-left-radius: %1px;"
        "   border-top-right-radius: %1px;"
        "   border-bottom-left-radius: 0px;"
        "   border-bottom-right-radius: 0px;"
        "   border: none;"
        "}")
        .arg(QString::fromStdString(std::to_string(scale(10))));
}

QString StyleMeetingWidget::joinRequestButtonsContainerStyle()
{
    return QString(
        "QWidget {"
        "   background: rgba(240, 244, 248, 180);"
        "   border-top-left-radius: 0px;"
        "   border-top-right-radius: 0px;"
        "   border-bottom-left-radius: %1px;"
        "   border-bottom-right-radius: %1px;"
        "   border: none;"
        "   border-top: 1px solid rgba(220, 220, 220, 0.5);"
        "}")
        .arg(QString::fromStdString(std::to_string(scale(10))));
}

QString StyleMeetingWidget::joinRequestAcceptButtonStyle()
{
    return QString(
        "QPushButton {"
        "   background-color: transparent;"
        "   color: %1;"
        "   border: none;"
        "   border-top-left-radius: 0px;"
        "   border-top-right-radius: 0px;"
        "   border-bottom-left-radius: %2px;"
        "   border-bottom-right-radius: 0px;"
        "   padding: %3px;"
        "   font-size: %4px;"
        "   font-weight: 600;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgba(%5, %6, %7, 0.12);"
        "}"
        "QPushButton:pressed {"
        "   background-color: rgba(%5, %6, %7, 0.2);"
        "}")
        .arg(m_acceptButtonColor.name())
        .arg(QString::fromStdString(std::to_string(scale(10))))
        .arg(QString::fromStdString(std::to_string(scale(10))))
        .arg(QString::fromStdString(std::to_string(scale(13))))
        .arg(m_acceptButtonColor.red())
        .arg(m_acceptButtonColor.green())
        .arg(m_acceptButtonColor.blue());
}

QString StyleMeetingWidget::joinRequestDeclineButtonStyle()
{
    return QString(
        "QPushButton {"
        "   background-color: transparent;"
        "   color: %1;"
        "   border: none;"
        "   border-top-left-radius: 0px;"
        "   border-top-right-radius: 0px;"
        "   border-bottom-left-radius: 0px;"
        "   border-bottom-right-radius: %2px;"
        "   padding: %3px;"
        "   font-size: %4px;"
        "   font-weight: 600;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgba(%5, %6, %7, 0.12);"
        "}"
        "QPushButton:pressed {"
        "   background-color: rgba(%5, %6, %7, 0.2);"
        "}")
        .arg(m_declineButtonColor.name())
        .arg(QString::fromStdString(std::to_string(scale(10))))
        .arg(QString::fromStdString(std::to_string(scale(10))))
        .arg(QString::fromStdString(std::to_string(scale(13))))
        .arg(m_declineButtonColor.red())
        .arg(m_declineButtonColor.green())
        .arg(m_declineButtonColor.blue());
}

QString StyleMeetingWidget::callNamePanelStyle()
{
    return QString(
        "QWidget#callNamePanel {"
        "   background: rgba(255, 255, 255, 60);"
        "   border: 1px solid rgba(255, 255, 255, 140);"
        "   border-radius: %5px;"
        "   backdrop-filter: blur(12px);"
        "}"
        "QLabel {"
        "   color: #1A1A1A;"
        "   background: transparent;"
        "}")
        .arg(QString::fromStdString(std::to_string(scale(20))))
        .arg(QString::fromStdString(std::to_string(scale(20))));
}

QString StyleMeetingWidget::callNameLabelStyle()
{
    return QString(
        "QLabel {"
        "   font-weight: 600;"
        "   font-size: %1px;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}")
        .arg(QString::fromStdString(std::to_string(scale(13))));
}

// MeetingWidget implementation
MeetingWidget::MeetingWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
    setupShadowEffect();

    m_callTimer = new QTimer(this);
    m_callDuration = new QTime(0, 0, 0);
    connect(m_callTimer, &QTimer::timeout, this, &MeetingWidget::updateCallTimer);
    m_callTimer->start(1000);

    m_exitFullscreenHideTimer = new QTimer(this);
    m_exitFullscreenHideTimer->setSingleShot(true);
    m_exitFullscreenHideTimer->setInterval(3000);
    connect(m_exitFullscreenHideTimer, &QTimer::timeout, this, &MeetingWidget::onExitFullscreenHideTimerTimeout);

    m_notificationTimer = new QTimer(this);
    m_notificationTimer->setSingleShot(true);
    connect(m_notificationTimer, &QTimer::timeout, [this]() { m_notificationWidget->hide(); });
}

void MeetingWidget::setupUI() {
    setFocusPolicy(Qt::StrongFocus);

    m_participantsContainer = new QWidget(this);
    m_participantsContainer->setStyleSheet("background-color: transparent;");
    m_participantsContainer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    m_participantsContainer->hide();

    m_participantsContainerLayout = new QHBoxLayout(m_participantsContainer);
    m_participantsContainerLayout->setContentsMargins(0, 0, 0, 0);
    m_participantsContainerLayout->setSpacing(scale(10));
    m_participantsContainerLayout->setAlignment(Qt::AlignCenter);

    m_prevPageButton = new ButtonIcon(m_participantsContainer, scale(40), scale(40));
    m_prevPageButton->setSize(scale(40), scale(40));
    m_prevPageButton->setToolTip("Previous page");
    m_prevPageButton->setCursor(Qt::PointingHandCursor);
    m_prevPageButton->hide();
    connect(m_prevPageButton, &ButtonIcon::clicked, this, &MeetingWidget::onPrevPageClicked);

    m_nextPageButton = new ButtonIcon(m_participantsContainer, scale(40), scale(40));
    m_nextPageButton->setSize(scale(40), scale(40));
    m_nextPageButton->setToolTip("Next page");
    m_nextPageButton->setCursor(Qt::PointingHandCursor);
    m_nextPageButton->hide();
    connect(m_nextPageButton, &ButtonIcon::clicked, this, &MeetingWidget::onNextPageClicked);

    QIcon prevIcon = createArrowIcon(true, false);
    QIcon prevIconHover = createArrowIcon(true, true);
    QIcon nextIcon = createArrowIcon(false, false);
    QIcon nextIconHover = createArrowIcon(false, true);
    m_prevPageButton->setIcons(prevIcon, prevIconHover);
    m_nextPageButton->setIcons(nextIcon, nextIconHover);

    // Кнопки страниц находятся в одном горизонтальном лейауте с панелями участников:
    // [prev] [panel_0/1/... по центру] [next]
    m_participantsContainerLayout->addWidget(m_prevPageButton);
    m_participantsContainerLayout->addWidget(m_nextPageButton);

    // Timer label (hidden, but timer still runs)
    m_timerLabel = new QLabel("00:00", this);
    m_timerLabel->setAlignment(Qt::AlignCenter);
    m_timerLabel->setStyleSheet(StyleMeetingWidget::timerStyle());
    m_timerLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    m_timerLabel->setMinimumHeight(scale(46));
    m_timerLabel->hide();
    QFont initialTimerFont("Outfit", scale(34), QFont::Bold);
    m_timerLabel->setFont(initialTimerFont);

    // Main screen
    m_mainScreen = new Screen(this);
    m_mainScreen->setStyleSheet("background-color: transparent; border: none;");
    m_mainScreen->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    m_mainScreen->setMinimumSize(100, 100);
    m_mainScreen->hide();
    applyStandardSize();

    // Buttons panel
    m_buttonsPanel = new QWidget(this);
    m_buttonsPanel->setStyleSheet(StyleMeetingWidget::panelStyle());
    m_buttonsPanel->setFixedHeight(scale(80));

    m_buttonsLayout = new QHBoxLayout(m_buttonsPanel);
    m_buttonsLayout->setSpacing(scale(15));
    m_buttonsLayout->setContentsMargins(scale(20), scale(10), scale(20), scale(10));
    m_buttonsLayout->setAlignment(Qt::AlignCenter);

    // Setup buttons (similar to CallWidget)
    m_screenShareIconNormal = QIcon(":/resources/screenShare.png");
    m_screenShareIconHover = QIcon(":/resources/screenShareHover.png");
    m_screenShareIconActive = QIcon(":/resources/screenShareActive.png");
    m_screenShareIconActiveHover = QIcon(":/resources/screenShareActiveHover.png");
    m_microphoneButton = new ToggleButtonIcon(m_buttonsPanel,
        QIcon(":/resources/microphone.png"),
        QIcon(":/resources/microphoneHover.png"),
        QIcon(":/resources/mute-enabled-microphone.png"),
        QIcon(":/resources/mute-enabled-microphoneHover.png"),
        scale(40), scale(40));
    m_microphoneButton->setSize(scale(37), scale(37));
    m_microphoneButton->setToolTip("Microphone mute");
    m_microphoneButton->setCursor(Qt::PointingHandCursor);

    m_screenShareIconRestricted = QIcon(":/resources/screenShareRestricted.png");
    m_screenShareButton = new ToggleButtonIcon(m_buttonsPanel,
        m_screenShareIconNormal,
        m_screenShareIconHover,
        m_screenShareIconActive,
        m_screenShareIconActiveHover,
        scale(48), scale(48));
    m_screenShareButton->setSize(scale(40), scale(40));
    m_screenShareButton->setToolTip("Share screen");
    m_screenShareButton->setCursor(Qt::PointingHandCursor);

    m_cameraIconActive = QIcon(":/resources/cameraActive.png");
    m_cameraIconActiveHover = QIcon(":/resources/cameraActiveHover.png");
    m_cameraIconRestricted = QIcon(":/resources/cameraRestricted.png");
    m_cameraIconDisabled = QIcon(":/resources/cameraDisabled.png");
    m_cameraIconDisabledHover = QIcon(":/resources/cameraDisabledHover.png");
    m_cameraButton = new ToggleButtonIcon(m_buttonsPanel,
        m_cameraIconDisabled,
        m_cameraIconDisabledHover,
        m_cameraIconActive,
        m_cameraIconActiveHover,
        scale(48), scale(48));
    m_cameraButton->setSize(scale(43), scale(43));
    m_cameraButton->setToolTip("Enable camera");
    m_cameraButton->setCursor(Qt::PointingHandCursor);

    QVBoxLayout* cameraLayout = new QVBoxLayout;
    cameraLayout->setContentsMargins(0, scale(7), 0, 0);
    cameraLayout->setSpacing(0);
    cameraLayout->addWidget(m_cameraButton);

    QWidget* cameraContainer = new QWidget(m_buttonsPanel);
    cameraContainer->setContentsMargins(0, 0, 0, 0);
    cameraContainer->setLayout(cameraLayout);

    m_fullscreenIconMaximize = QIcon(":/resources/maximize.png");
    m_fullscreenIconMaximizeHover = QIcon(":/resources/maximizeHover.png");
    m_enterFullscreenButton = new ButtonIcon(m_buttonsPanel, scale(32), scale(32));
    m_enterFullscreenButton->setIcons(m_fullscreenIconMaximize, m_fullscreenIconMaximizeHover);
    m_enterFullscreenButton->setSize(scale(28), scale(28));
    m_enterFullscreenButton->setToolTip("Fullscreen");
    m_enterFullscreenButton->setCursor(Qt::PointingHandCursor);
    m_enterFullscreenButton->hide();

    m_hangupButton = new QPushButton(m_buttonsPanel);
    m_hangupButton->setFixedSize(scale(60), scale(60));
    m_hangupButton->setStyleSheet(StyleMeetingWidget::hangupButtonStyle());
    m_hangupButton->setCursor(Qt::PointingHandCursor);
    m_hangupButton->setIcon(QIcon(":/resources/decline.png"));
    m_hangupButton->setIconSize(QSize(scale(30), scale(30)));
    m_hangupButton->setToolTip("Hang up");
    m_hangupButton->setCursor(Qt::PointingHandCursor);

    m_buttonsLayout->addWidget(cameraContainer);
    m_buttonsLayout->addWidget(m_microphoneButton);
    m_buttonsLayout->addWidget(m_screenShareButton);
    m_buttonsLayout->addWidget(m_enterFullscreenButton);
    m_buttonsLayout->addWidget(m_hangupButton);

    // Exit fullscreen button
    m_fullscreenIconMinimize = QIcon(":/resources/minimize.png");
    m_fullscreenIconMinimizeHover = QIcon(":/resources/minimizeHover.png");
    m_exitFullscreenButton = new ButtonIcon(this, scale(28), scale(28));
    m_exitFullscreenButton->setIcons(m_fullscreenIconMinimize, m_fullscreenIconMinimizeHover);
    m_exitFullscreenButton->setSize(scale(38), scale(38));
    m_exitFullscreenButton->setToolTip("Exit fullscreen");
    m_exitFullscreenButton->setCursor(Qt::PointingHandCursor);
    m_exitFullscreenButton->hide();

    m_settingsButton = new ButtonIcon(this, scale(24), scale(24));
    m_settingsButton->setIcons(QIcon(":/resources/settings.png"), QIcon(":/resources/settingsHover.png"));
    m_settingsButton->setSize(scale(32), scale(32));
    m_settingsButton->setToolTip("Audio settings");
    m_settingsButton->setCursor(Qt::PointingHandCursor);
    m_settingsButton->hide();

    // Notification widget
    m_notificationWidget = new QWidget(this);
    m_notificationWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    m_notificationWidget->hide();
    m_notificationWidget->setStyleSheet(StyleMeetingWidget::notificationRedLabelStyle());

    m_notificationLayout = new QHBoxLayout(m_notificationWidget);
    m_notificationLayout->setAlignment(Qt::AlignCenter);
    m_notificationLayout->setContentsMargins(scale(18), scale(8), scale(18), scale(8));

    m_notificationLabel = new QLabel(m_notificationWidget);
    QFont errorFont("Outfit", scale(12), QFont::Medium);
    m_notificationLabel->setFont(errorFont);
    m_notificationLabel->setStyleSheet("color: #DC5050; background: transparent; font-size: 14px; margin: 0px; padding: 0px;");
    m_notificationLayout->addWidget(m_notificationLabel);

    // Join requests panel
    m_joinRequestsPanel = new QWidget(this);
    m_joinRequestsPanel->setStyleSheet(StyleMeetingWidget::joinRequestPanelStyle());
    m_joinRequestsPanel->setObjectName("joinRequestsPanel");
    m_joinRequestsPanel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    m_joinRequestsPanel->setMinimumWidth(scale(360));
    m_joinRequestsPanel->setMaximumWidth(scale(400));
    m_joinRequestsPanel->hide();

    // Add shadow effect
    auto* shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(scale(24));
    shadow->setColor(QColor(0, 0, 0, 80));
    shadow->setOffset(0, scale(6));
    m_joinRequestsPanel->setGraphicsEffect(shadow);

    m_joinRequestsLayout = new QVBoxLayout(m_joinRequestsPanel);
    m_joinRequestsLayout->setContentsMargins(scale(16), scale(16), scale(16), scale(16));
    m_joinRequestsLayout->setSpacing(scale(12));

    // Add title label
    auto* titleLabel = new QLabel("Join Requests", m_joinRequestsPanel);
    QFont titleFont("Outfit", scale(14), QFont::Bold);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet(
        "color: #1A1A1A;"
        "font-weight: 700;"
        "font-size: 15px;"
        "padding: 0px 0px 8px 0px;"
        "margin: 0px;"
        "border-bottom: 1px solid rgba(220, 220, 220, 0.4);");
    m_joinRequestsLayout->addWidget(titleLabel);

    // Create scroll area for join requests
    m_joinRequestsScrollArea = new QScrollArea(m_joinRequestsPanel);
    m_joinRequestsScrollArea->setWidgetResizable(true);
    m_joinRequestsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_joinRequestsScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_joinRequestsScrollArea->setStyleSheet(StyleMeetingWidget::joinRequestScrollAreaStyle());
    m_joinRequestsScrollArea->verticalScrollBar()->setStyleSheet(StyleMeetingWidget::joinRequestScrollBarStyle());

    // Create container for join request items
    m_joinRequestsContainer = new QWidget(m_joinRequestsScrollArea);
    m_joinRequestsContainer->setStyleSheet("background: transparent; border: none;");
    
    m_joinRequestsContainerLayout = new QVBoxLayout(m_joinRequestsContainer);
    m_joinRequestsContainerLayout->setContentsMargins(0, 0, 0, 0);
    m_joinRequestsContainerLayout->setSpacing(scale(10));
    m_joinRequestsContainerLayout->addStretch();

    m_joinRequestsScrollArea->setWidget(m_joinRequestsContainer);
    m_joinRequestsLayout->addWidget(m_joinRequestsScrollArea);

    // Call name badge
    m_callNamePanel = new QWidget(this);
    m_callNamePanel->setObjectName("callNamePanel");
    m_callNamePanel->setStyleSheet(StyleMeetingWidget::callNamePanelStyle());
    m_callNamePanel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    m_callNamePanel->setMouseTracking(true);
    m_callNamePanel->hide();

    m_overlayHideTimer = new QTimer(this);
    m_overlayHideTimer->setSingleShot(true);
    m_overlayHideTimer->setInterval(3000);
    connect(m_overlayHideTimer, &QTimer::timeout, this, &MeetingWidget::onOverlayHideTimerTimeout);

    m_callNameLayout = new QHBoxLayout(m_callNamePanel);
    m_callNameLayout->setContentsMargins(scale(18), scale(12), scale(16), scale(12));
    m_callNameLayout->setSpacing(scale(12));
    m_callNameLayout->setAlignment(Qt::AlignVCenter);

    m_callNameLabel = new QLabel(m_callNamePanel);
    QFont callNameFont("Outfit", scale(12), QFont::Light);
    m_callNameLabel->setFont(callNameFont);
    m_callNameLabel->setStyleSheet(StyleMeetingWidget::callNameLabelStyle());
    m_callNameLabel->setWordWrap(false);
    m_callNameLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_callNameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_callNameLabel->setMouseTracking(true);

    m_copyCallNameButton = new ToggleButtonIcon(m_callNamePanel,
        QIcon(":/resources/copy.png"),
        QIcon(":/resources/copyHover.png"),
        QIcon(":/resources/copied.png"),
        QIcon(":/resources/copiedHover.png"),
        scale(20), scale(20));

    m_copyCallNameButton->setSize(scale(20), scale(20));
    m_copyCallNameButton->setCursor(Qt::PointingHandCursor);
    m_copyCallNameButton->setToolTip("Copy meeting id");

    m_callNameLayout->addWidget(m_callNameLabel);
    m_callNameLayout->addSpacing(scale(10));
    m_callNameLayout->addWidget(m_copyCallNameButton);

    // Spacers
    m_topMainLayoutSpacer = new QSpacerItem(0, scale(10), QSizePolicy::Minimum, QSizePolicy::Fixed);
    // Main layout
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(scale(10));
    m_mainLayout->setContentsMargins(scale(0), scale(0), scale(0), scale(0));
    m_mainLayout->setAlignment(Qt::AlignCenter);

    m_mainLayout->addSpacerItem(m_topMainLayoutSpacer);
    m_mainLayout->addWidget(m_notificationWidget, 0, Qt::AlignHCenter);
    m_mainLayout->addWidget(m_participantsContainer, 0, Qt::AlignHCenter);
    m_mainLayout->addWidget(m_mainScreen, 0, Qt::AlignHCenter);
    m_mainLayout->addWidget(m_buttonsPanel);

    setMouseTracking(true);

    // Ensure initial positioning of floating panels
    QTimer::singleShot(0, this, [this]()
    {
        updateCallNamePanelPosition();
        updateJoinRequestsPanelPosition();
    });

    // Connect signals
    connect(m_exitFullscreenButton, &ButtonIcon::clicked, [this]() { showOverlayWithTimeout(); emit requestExitFullscreen(); });
    connect(m_settingsButton, &ButtonIcon::clicked, [this]() { emit audioSettingsRequested(); });
    connect(m_enterFullscreenButton, &ButtonIcon::clicked, [this]() { showOverlayWithTimeout(); emit requestEnterFullscreen(); });
    connect(m_microphoneButton, &ToggleButtonIcon::toggled, [this](bool toggled) {
        showOverlayWithTimeout();
        if (!m_localParticipantNickname.isEmpty() && m_participantWidgets.contains(m_localParticipantNickname)) {
            m_participantWidgets[m_localParticipantNickname]->setMuted(toggled);
        }
        emit muteMicrophoneClicked(toggled);
    });
    connect(m_screenShareButton, &ToggleButtonIcon::toggled, [this](bool toggled) {
        showOverlayWithTimeout();
        m_hasScreenSharing = toggled;
        emit screenShareClicked(toggled);
    });
    connect(m_cameraButton, &ToggleButtonIcon::toggled, [this](bool toggled) { showOverlayWithTimeout(); emit cameraClicked(toggled); });
    connect(m_hangupButton, &QPushButton::clicked, [this]() 
    { 
        showOverlayWithTimeout(); 
        
        // Check if user is owner and has other participants (more than just the owner)
        if (m_isOwner && m_participantWidgets.size() > 1)
        {
            emit hangupConfirmationRequested();
        }
        else
        {
            emit hangupClicked();
        }
    });
    connect(m_copyCallNameButton, &ToggleButtonIcon::toggled, this, [this](bool toggled) {
        if (!toggled)
        {
            return;
        }

        if (m_callName.isEmpty())
        {
            m_copyCallNameButton->setToggled(false);
            return;
        }

        QClipboard* clipboard = QGuiApplication::clipboard();
        if (clipboard)
        {
            clipboard->setText(m_callName);
        }

        QTimer::singleShot(1200, this, [this]()
        {
            if (m_copyCallNameButton)
            {
                m_copyCallNameButton->setToggled(false);
            }
        });
    });
}

void MeetingWidget::setupShadowEffect() {
    setupElementShadow(m_enterFullscreenButton, 10, QColor(0, 0, 0, 30));
    setupElementShadow(m_settingsButton, 10, QColor(0, 0, 0, 30));
    setupElementShadow(m_microphoneButton, 10, QColor(0, 0, 0, 30));
    setupElementShadow(m_screenShareButton, 10, QColor(0, 0, 0, 30));
    setupElementShadow(m_cameraButton, 10, QColor(0, 0, 0, 30));
    setupElementShadow(m_hangupButton, 10, QColor(0, 0, 0, 30));
}

void MeetingWidget::setupElementShadow(QWidget* widget, int blurRadius, const QColor& color) {
    if (!widget) return;

    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect(this);
    shadowEffect->setBlurRadius(blurRadius);
    shadowEffect->setColor(color);
    shadowEffect->setOffset(0, 3);
    widget->setGraphicsEffect(shadowEffect);
}

void MeetingWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Use fancyBlur.png as background
    QPixmap background(":/resources/blur.png");
    painter.drawPixmap(rect(), background);

    QWidget::paintEvent(event);
}

void MeetingWidget::resizeEvent(QResizeEvent* event) {
    updateParticipantsContainerSize();
    updateOverlayButtonsPosition();
    updateParticipantPanels();

    if (m_mainScreen->isVisible())
        updateMainScreenSize();

    updateCallNamePanelPosition();
    updateJoinRequestsPanelPosition();

    QWidget::resizeEvent(event);
}

void MeetingWidget::setCallName(const QString& callName) {
    m_callName = callName;

    const bool hasName = !m_callName.isEmpty();

    if (m_callNameLabel)
    {
        QString displayName = m_callName;

        if (displayName.length() > 5)
        {
            displayName = displayName.left(5) + "...";
        }

        m_callNameLabel->setText(QString("%1").arg(displayName));
        m_callNameLabel->setToolTip(m_callName);
    }

    if (m_copyCallNameButton)
    {
        m_copyCallNameButton->setEnabled(hasName);
        m_copyCallNameButton->setToggled(false);
    }

    if (m_callNamePanel)
    {
        if (hasName)
        {
            updateCallNamePanelPosition();
            m_callNamePanel->adjustSize();
            updateCallNamePanelPosition();
        }
        else
        {
            m_callNamePanel->hide();
        }
    }

    *m_callDuration = QTime(0, 0, 0);

    if (m_callTimer->isActive()) {
        m_callTimer->stop();
    }

    m_callTimer->start(1000);
}

void MeetingWidget::updateCallTimer() {
    *m_callDuration = m_callDuration->addSecs(1);
}

bool MeetingWidget::isFullScreen() const {
    return m_screenFullscreenActive;
}

bool MeetingWidget::isMainScreenVisible() const {
    return !m_mainScreen->isHidden();
}

void MeetingWidget::setLocalParticipantNickname(const QString& nickname) {
    m_localParticipantNickname = nickname;
}

void MeetingWidget::setInputVolume(int newVolume) {
    Q_UNUSED(newVolume);
}

void MeetingWidget::setOutputVolume(int newVolume) {
    Q_UNUSED(newVolume);
}

void MeetingWidget::setMicrophoneMuted(bool muted) {
    if (m_microphoneButton && m_microphoneButton->isToggled() != muted) {
        m_microphoneButton->setToggled(muted);
    }

    if (!m_localParticipantNickname.isEmpty() && m_participantWidgets.contains(m_localParticipantNickname)) {
        m_participantWidgets[m_localParticipantNickname]->setMuted(muted);
    }
}

void MeetingWidget::setSpeakerMuted(bool muted) {
    Q_UNUSED(muted);
}

void MeetingWidget::addParticipant(const QString& nickname) {
    if (m_participantWidgets.contains(nickname)) return;

    MeetingParticipantWidget* participantWidget = new MeetingParticipantWidget(nickname, m_participantsContainer);
    m_participantWidgets[nickname] = participantWidget;

    updateParticipantPanels();
    updateParticipantsContainerSize();
}

QStringList MeetingWidget::getParticipantNicknames() const
{
    return m_participantWidgets.keys();
}


void MeetingWidget::removeParticipant(const QString& nickname) {
    if (m_participantWidgets.contains(nickname)) {
        MeetingParticipantWidget* widget = m_participantWidgets[nickname];
        
        for (QGridLayout* layout : m_participantPanelLayouts) {
            layout->removeWidget(widget);
        }
        
        widget->deleteLater();
        m_participantWidgets.remove(nickname);

        updateParticipantPanels();
        updateParticipantsContainerSize();
    }
}

void MeetingWidget::updateParticipantVideo(const QString& nickname, const QPixmap& frame) {
    if (m_participantWidgets.contains(nickname)) {
        m_participantWidgets[nickname]->updateVideoFrame(frame);
    }
}

void MeetingWidget::clearParticipantVideo(const QString& nickname) {
    if (m_participantWidgets.contains(nickname)) {
        m_participantWidgets[nickname]->clearVideoFrame();
    }
}

void MeetingWidget::clearAllParticipantVideos() {
    for (MeetingParticipantWidget* widget : m_participantWidgets) {
        if (widget) {
            widget->clearVideoFrame();
        }
    }
}

void MeetingWidget::setParticipantMuted(const QString& nickname, bool muted) {
    if (m_participantWidgets.contains(nickname)) {
        m_participantWidgets[nickname]->setMuted(muted);
    }
}

void MeetingWidget::setParticipantSpeaking(const QString& nickname, bool speaking) {
    if (m_participantWidgets.contains(nickname)) {
        m_participantWidgets[nickname]->setSpeaking(speaking);
    }
}

void MeetingWidget::setParticipantScreenSharing(const QString& nickname, bool sharing) {
    if (m_participantWidgets.contains(nickname)) {
        m_participantWidgets[nickname]->setScreenSharing(sharing);
    }
}

void MeetingWidget::setParticipantConnectionDown(const QString& nickname, bool down) {
    if (m_participantWidgets.contains(nickname)) {
        m_participantWidgets[nickname]->setConnectionDown(down);
    }
}

void MeetingWidget::setParticipantCameraEnabled(const QString& nickname, bool enabled) {
    if (m_participantWidgets.contains(nickname)) {
        m_participantWidgets[nickname]->setCameraEnabled(enabled);
    }
}

void MeetingWidget::clearParticipants() {
    for (MeetingParticipantWidget* widget : m_participantWidgets) {
        widget->deleteLater();
    }
    m_participantWidgets.clear();
    m_localParticipantNickname.clear();
    
    for (QWidget* panel : m_participantPanels) {
        panel->deleteLater();
    }
    m_participantPanels.clear();
    m_participantPanelLayouts.clear();
    m_currentPageIndex = 0;
    
    m_isOwner = true;
    
    updateParticipantsContainerSize();
}

void MeetingWidget::resetMeetingState() {
    if (m_screenFullscreenActive) {
        exitFullscreen();
    }

    if (m_overlayHideTimer) {
        m_overlayHideTimer->stop();
    }
    if (m_exitFullscreenHideTimer) {
        m_exitFullscreenHideTimer->stop();
    }

    m_audioSettingsDialogOpen = false;
    m_hasScreenSharing = false;

    hideMainScreen();
    clearAllParticipantVideos();
    clearParticipants();

    if (m_microphoneButton) {
        m_microphoneButton->setToggled(false);
    }
    setScreenShareButtonActive(false);
    setCameraButtonActive(false);

    if (m_notificationWidget) {
        m_notificationWidget->hide();
    }

    if (m_copyCallNameButton) {
        m_copyCallNameButton->setToggled(false);
        m_copyCallNameButton->setEnabled(false);
    }
    if (m_callNameLabel) {
        m_callNameLabel->clear();
        m_callNameLabel->setToolTip(QString());
    }
    m_callName.clear();
    if (m_callNamePanel) {
        m_callNamePanel->hide();
    }

    if (m_joinRequestsContainerLayout) {
        for (QWidget* item : m_joinRequestItems) {
            m_joinRequestsContainerLayout->removeWidget(item);
            item->deleteLater();
        }
    }
    m_joinRequestItems.clear();
    if (m_joinRequestsPanel) {
        m_joinRequestsPanel->hide();
    }

    if (m_settingsButton) {
        m_settingsButton->hide();
    }
    if (m_exitFullscreenButton) {
        m_exitFullscreenButton->hide();
    }
    if (m_enterFullscreenButton) {
        m_enterFullscreenButton->hide();
    }

    if (m_callDuration) {
        *m_callDuration = QTime(0, 0, 0);
    }
    if (m_callTimer && m_callTimer->isActive()) {
        m_callTimer->stop();
    }

    updateOverlayButtonsPosition();
    updateCallNamePanelPosition();
    updateJoinRequestsPanelPosition();
    updateParticipantsContainerSize();
    updateMainScreenSize();
}

void MeetingWidget::addJoinRequest(const QString& nickname)
{
    if (!m_joinRequestsLayout) return;

    if (m_joinRequestItems.contains(nickname))
    {
        return;
    }

    QWidget* item = new QWidget(m_joinRequestsPanel);
    item->setObjectName("joinRequestItem");
    item->setStyleSheet(StyleMeetingWidget::joinRequestItemStyle());
    
    auto* layout = new QVBoxLayout(item);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Content container with background
    auto* contentContainer = new QWidget(item);
    contentContainer->setStyleSheet(StyleMeetingWidget::joinRequestContentContainerStyle());
    
    auto* contentLayout = new QVBoxLayout(contentContainer);
    contentLayout->setContentsMargins(scale(16), scale(14), scale(16), scale(14));
    
    // Nickname label
    auto* label = new QLabel(QString("<b>%1</b> wants to join").arg(nickname), contentContainer);
    QFont labelFont("Outfit", scale(13));
    label->setFont(labelFont);
    label->setStyleSheet("color: #2C2C2C; font-size: 14px; padding: 0px; margin: 0px; background: transparent; border: none;");
    label->setWordWrap(true);
    contentLayout->addWidget(label);
    
    layout->addWidget(contentContainer);

    // Buttons container with background and bottom corners
    auto* buttonsContainer = new QWidget(item);
    buttonsContainer->setStyleSheet(StyleMeetingWidget::joinRequestButtonsContainerStyle());
    
    auto* buttonsLayout = new QHBoxLayout(buttonsContainer);
    buttonsLayout->setSpacing(0);
    buttonsLayout->setContentsMargins(0, 0, 0, 0);

    auto* acceptBtn = new QPushButton("Accept", buttonsContainer);
    auto* declineBtn = new QPushButton("Decline", buttonsContainer);
    
    acceptBtn->setStyleSheet(StyleMeetingWidget::joinRequestAcceptButtonStyle());
    declineBtn->setStyleSheet(StyleMeetingWidget::joinRequestDeclineButtonStyle());
    
    acceptBtn->setCursor(Qt::PointingHandCursor);
    declineBtn->setCursor(Qt::PointingHandCursor);
    
    // Make buttons equal width
    acceptBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    declineBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    acceptBtn->setFixedHeight(scale(38));
    declineBtn->setFixedHeight(scale(38));

    buttonsLayout->addWidget(acceptBtn);
    
    // Add vertical separator between buttons
    auto* separator = new QWidget(buttonsContainer);
    separator->setFixedWidth(1);
    separator->setStyleSheet("background-color: rgba(220, 220, 220, 0.6); border: none;");
    buttonsLayout->addWidget(separator);
    
    buttonsLayout->addWidget(declineBtn);
    
    layout->addWidget(buttonsContainer);

    connect(acceptBtn, &QPushButton::clicked, this, [this, nickname]() {
        emit joinRequestAccepted(nickname);
        removeJoinRequest(nickname);
    });
    connect(declineBtn, &QPushButton::clicked, this, [this, nickname]() {
        emit joinRequestDeclined(nickname);
        removeJoinRequest(nickname);
    });

    m_joinRequestItems.insert(nickname, item);
    
    // Insert before stretch item
    int count = m_joinRequestsContainerLayout->count();
    if (count > 0)
    {
        m_joinRequestsContainerLayout->insertWidget(count - 1, item);
    }
    else
    {
        m_joinRequestsContainerLayout->addWidget(item);
    }

    if (m_joinRequestsPanel)
        m_joinRequestsPanel->show();

    updateJoinRequestsPanelSize();
    updateJoinRequestsPanelPosition();
}

void MeetingWidget::removeJoinRequest(const QString& nickname)
{
    if (!m_joinRequestItems.contains(nickname)) return;
    
    QWidget* item = m_joinRequestItems.take(nickname);
    m_joinRequestsContainerLayout->removeWidget(item);
    item->deleteLater();
    
    if (m_joinRequestItems.isEmpty() && m_joinRequestsPanel)
    {
        m_joinRequestsPanel->hide();
    }
    else
    {
        updateJoinRequestsPanelSize();
    }
    
    updateJoinRequestsPanelPosition();
}

void MeetingWidget::updateCallNamePanelPosition()
{
    if (!m_callNamePanel)
    {
        return;
    }

    const int margin = scale(10);
    const int topMargin = scale(10);
    const int buttonSize = scale(38);
    const int spacing = scale(12);

    if (m_callNamePanel->isVisible())
    {
        m_callNamePanel->adjustSize();

        int maxWidth = width() - (margin * 2) - buttonSize - spacing;
        if (maxWidth > 0 && m_callNamePanel->sizeHint().width() > maxWidth)
        {
            m_callNamePanel->setMaximumWidth(maxWidth);
        }
        else
        {
            m_callNamePanel->setMaximumWidth(16777215);
        }
    }

    // Position to the left of settings button, same horizontal line
    m_callNamePanel->adjustSize();
    int panelWidth = m_callNamePanel->sizeHint().width();
    int x = width() - buttonSize - margin - spacing - panelWidth;
    int y = topMargin;

    if (x < margin)
        x = margin;

    m_callNamePanel->move(x, y);
    m_callNamePanel->raise();
}

void MeetingWidget::showOverlayWithTimeout()
{
    if (m_screenFullscreenActive)
    {
        if (m_exitFullscreenButton)
            m_exitFullscreenButton->show();
        if (m_settingsButton)
            m_settingsButton->hide();
        if (m_callNamePanel)
            m_callNamePanel->hide();
        if (m_exitFullscreenHideTimer)
            m_exitFullscreenHideTimer->start();
    }
    else
    {
        if (m_exitFullscreenButton)
            m_exitFullscreenButton->hide();
        if (m_settingsButton && !m_audioSettingsDialogOpen)
            m_settingsButton->show();
        if (m_callNamePanel && !m_callName.isEmpty())
        {
            m_callNamePanel->show();
            m_callNamePanel->adjustSize();
        }
        if (m_joinRequestsPanel && !m_joinRequestItems.isEmpty())
            m_joinRequestsPanel->show();
    }

    updateOverlayButtonsPosition();
    updateCallNamePanelPosition();
    updateJoinRequestsPanelPosition();

    if (m_overlayHideTimer)
    {
        m_overlayHideTimer->stop();
        m_overlayHideTimer->start();
    }
}

void MeetingWidget::onOverlayHideTimerTimeout()
{
    if (m_audioSettingsDialogOpen)
        return;
    if (m_screenFullscreenActive)
    {
        if (m_exitFullscreenButton)
            m_exitFullscreenButton->hide();
        return;
    }
    if (m_settingsButton)
        m_settingsButton->hide();
    if (m_callNamePanel)
        m_callNamePanel->hide();
}

void MeetingWidget::updateJoinRequestsPanelPosition()
{
    if (!m_joinRequestsPanel) return;

    const int margin = scale(10);
    const int topMargin = scale(10);
    
    m_joinRequestsPanel->adjustSize();
    QSize panelSize = m_joinRequestsPanel->size();
    
    // Position panel in top-left corner
    int x = margin;
    int y = topMargin;
    
    m_joinRequestsPanel->move(x, y);
    m_joinRequestsPanel->raise();
}

void MeetingWidget::updateJoinRequestsPanelSize()
{
    if (!m_joinRequestsScrollArea || !m_joinRequestsContainer) return;

    int itemCount = m_joinRequestItems.size();
    if (itemCount == 0)
    {
        return;
    }

    const int maxVisibleItems = 3;
    const int spacing = scale(10);
    
    // Get actual height from first item if available
    int itemHeight = scale(95); // fallback value
    if (!m_joinRequestItems.isEmpty())
    {
        QWidget* firstItem = m_joinRequestItems.first();
        if (firstItem)
        {
            firstItem->adjustSize();
            itemHeight = firstItem->sizeHint().height();
            if (itemHeight <= 0)
            {
                itemHeight = scale(95);
            }
        }
    }
    
    // Calculate desired height for scroll area
    int desiredHeight;
    if (itemCount <= maxVisibleItems)
    {
        // Show all items without scroll
        desiredHeight = itemCount * itemHeight + (itemCount - 1) * spacing;
    }
    else
    {
        // Show max 3 items with scroll
        desiredHeight = maxVisibleItems * itemHeight + (maxVisibleItems - 1) * spacing;
    }
    
    m_joinRequestsScrollArea->setMinimumHeight(desiredHeight);
    m_joinRequestsScrollArea->setMaximumHeight(desiredHeight);
}

void MeetingWidget::updateParticipantsLayout() {
    updateParticipantPanels();
}

void MeetingWidget::updateParticipantPanels() {
    if (m_participantWidgets.isEmpty()) {
        for (QWidget* panel : m_participantPanels) {
            m_participantsContainerLayout->removeWidget(panel);
            panel->deleteLater();
        }
        m_participantPanels.clear();
        m_participantPanelLayouts.clear();
        m_currentPageIndex = 0;
        return;
    }

    int participantsPerPage = calculateParticipantsPerPage();
    if (participantsPerPage <= 0) participantsPerPage = 1;

    int totalParticipants = m_participantWidgets.size();
    int totalPages = (totalParticipants + participantsPerPage - 1) / participantsPerPage;

    while (m_participantPanels.size() < totalPages) {
        QWidget* panel = new QWidget(m_participantsContainer);
        panel->setStyleSheet("background-color: transparent;");
        panel->hide();

        QGridLayout* gridLayout = new QGridLayout(panel);
        gridLayout->setContentsMargins(scale(10), scale(10), scale(10), scale(10));
        gridLayout->setSpacing(scale(15));
        gridLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
        
        for (int col = 0; col < kMaxParticipantsPerRow; ++col) {
            gridLayout->setColumnStretch(col, 0);
        }

        m_participantPanels.append(panel);
        m_participantPanelLayouts.append(gridLayout);
        
        // Вставляем панели между кнопками prev/next.
        int insertIndex = m_participantsContainerLayout->indexOf(m_nextPageButton);
        if (insertIndex < 0) {
            m_participantsContainerLayout->addWidget(panel);
        } else {
            m_participantsContainerLayout->insertWidget(insertIndex, panel);
        }
    }

    while (m_participantPanels.size() > totalPages) {
        QWidget* panel = m_participantPanels.takeLast();
        QGridLayout* layout = m_participantPanelLayouts.takeLast();
        m_participantsContainerLayout->removeWidget(panel);
        panel->deleteLater();
    }

    int maxPerRow = calculateMaxParticipantsPerRow();
    bool isMainScreenVisible = m_mainScreen && m_mainScreen->isVisible();
    int widgetWidth = isMainScreenVisible ? scale(240) : scale(320);
    int widgetHeight = isMainScreenVisible ? scale(135) : scale(180);

    QList<MeetingParticipantWidget*> widgetsList = m_participantWidgets.values();
    int widgetIndex = 0;
    
    for (MeetingParticipantWidget* widget : widgetsList) {
        widget->setCompactSize(isMainScreenVisible);
    }

    for (int pageIndex = 0; pageIndex < totalPages; ++pageIndex) {
        QGridLayout* gridLayout = m_participantPanelLayouts[pageIndex];
        
        QLayoutItem* item;
        while ((item = gridLayout->takeAt(0)) != nullptr) {
            delete item;
        }

        int row = 0, col = 0;
        int participantsInThisPage = qMin(participantsPerPage, totalParticipants - widgetIndex);

        for (int i = 0; i < participantsInThisPage && widgetIndex < widgetsList.size(); ++i) {
            MeetingParticipantWidget* widget = widgetsList[widgetIndex];
            gridLayout->addWidget(widget, row, col, Qt::AlignCenter);

            col++;
            if (col >= maxPerRow) {
                col = 0;
                row++;
            }
            widgetIndex++;
        }

        int actualParticipantsInRow = qMin(participantsInThisPage, maxPerRow);
        
        for (int col = 0; col < maxPerRow; ++col) {
            if (col < actualParticipantsInRow) {
                gridLayout->setColumnMinimumWidth(col, widgetWidth);
                gridLayout->setColumnStretch(col, 0);
            } else {
                gridLayout->setColumnMinimumWidth(col, 0);
                gridLayout->setColumnStretch(col, 1);
            }
        }
    }

    if (m_currentPageIndex >= totalPages) {
        m_currentPageIndex = qMax(0, totalPages - 1);
    }

    showPage(m_currentPageIndex);
    updateNavigationButtons();
}

int MeetingWidget::calculateParticipantsPerPage() const {
    int maxPerRow = calculateMaxParticipantsPerRow();
    if (maxPerRow <= 0) return 1;

    bool isMainScreenVisible = m_mainScreen && m_mainScreen->isVisible();
    int widgetHeight = isMainScreenVisible ? scale(135) : scale(180);
    int spacing = scale(15);
    int margins = scale(10) * 2;
    
    int maxAllowedHeight;
    if (isMainScreenVisible) {
        maxAllowedHeight = height() * 0.25;
    } else {
        maxAllowedHeight = height() * 0.6;
    }
    
    int availableHeight = maxAllowedHeight - margins;
    if (availableHeight <= 0) return maxPerRow;

    int rowsPerPage = qMax(1, availableHeight / (widgetHeight + spacing));
    return rowsPerPage * maxPerRow;
}

void MeetingWidget::showPage(int pageIndex) {
    if (pageIndex < 0 || pageIndex >= m_participantPanels.size()) return;

    for (int i = 0; i < m_participantPanels.size(); ++i) {
        if (i == pageIndex) {
            m_participantPanels[i]->show();
        } else {
            m_participantPanels[i]->hide();
        }
    }
}

void MeetingWidget::updateNavigationButtons() {
    int totalPages = m_participantPanels.size();
    
    if (totalPages <= 1) {
        // Нет пагинации — стрелки не нужны.
        m_prevPageButton->hide();
        m_nextPageButton->hide();
        return;
    }

    QIcon prevIcon = createArrowIcon(true, false);
    QIcon prevIconHover = createArrowIcon(true, true);
    QIcon nextIcon = createArrowIcon(false, false);
    QIcon nextIconHover = createArrowIcon(false, true);
    
    bool prevVisible = m_currentPageIndex > 0;
    bool nextVisible = m_currentPageIndex < totalPages - 1;
    
    // Оба слота стрелок всегда присутствуют в лейауте,
    // просто включаем/выключаем соответствующую сторону.
    m_prevPageButton->show();
    m_nextPageButton->show();

    m_prevPageButton->setEnabled(prevVisible);
    m_prevPageButton->setCursor(prevVisible ? Qt::PointingHandCursor : Qt::ArrowCursor);
    m_prevPageButton->setIcons(prevVisible ? prevIcon : QIcon(), prevVisible ? prevIconHover : QIcon());

    m_nextPageButton->setEnabled(nextVisible);
    m_nextPageButton->setCursor(nextVisible ? Qt::PointingHandCursor : Qt::ArrowCursor);
    m_nextPageButton->setIcons(nextVisible ? nextIcon : QIcon(), nextVisible ? nextIconHover : QIcon());
}

void MeetingWidget::onPrevPageClicked() {
    if (m_currentPageIndex > 0) {
        m_currentPageIndex--;
        showPage(m_currentPageIndex);
        updateNavigationButtons();
    }
}

void MeetingWidget::onNextPageClicked() {
    if (m_currentPageIndex < m_participantPanels.size() - 1) {
        m_currentPageIndex++;
        showPage(m_currentPageIndex);
        updateNavigationButtons();
    }
}

QIcon MeetingWidget::createArrowIcon(bool left, bool hover) const {
    QPixmap pixmap(scale(40), scale(40));
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QColor arrowColor = hover ? QColor(100, 100, 100) : QColor(150, 150, 150);
    painter.setPen(QPen(arrowColor, scale(3), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(arrowColor);
    
    int centerX = pixmap.width() / 2;
    int centerY = pixmap.height() / 2;
    int arrowSize = scale(14);
    
    QPolygon arrow;
    if (left) {
        arrow << QPoint(centerX + arrowSize / 2, centerY - arrowSize / 2)
              << QPoint(centerX - arrowSize / 2, centerY)
              << QPoint(centerX + arrowSize / 2, centerY + arrowSize / 2);
    } else {
        arrow << QPoint(centerX - arrowSize / 2, centerY - arrowSize / 2)
              << QPoint(centerX + arrowSize / 2, centerY)
              << QPoint(centerX - arrowSize / 2, centerY + arrowSize / 2);
    }
    
    painter.drawPolygon(arrow);
    
    QIcon icon(pixmap);
    return icon;
}

int MeetingWidget::calculateMaxParticipantsPerRow() const {
    bool isMainScreenVisible = m_mainScreen && m_mainScreen->isVisible();
    int widgetWidth = isMainScreenVisible ? scale(240) : scale(320);
    int spacing = scale(15);
    int buttonWidth = scale(40);
    int buttonSpacing = scale(10);
    // Доступная ширина — это ширина виджета минус стрелки по краям и их отступы.
    int availableWidth = width() - scale(40) - (buttonWidth * 2 + buttonSpacing * 2);

    if (availableWidth <= 0) return 1;

    int baseCount = qMax(1, availableWidth / (widgetWidth + spacing));
    
    if (baseCount == 2) {
        int widthForThree = 3 * widgetWidth + 2 * spacing;
        if (availableWidth >= widthForThree) {
            return 3;
        }
        else {
            return 1;
        }
    }
    
    return qMin(baseCount, kMaxParticipantsPerRow);
}

void MeetingWidget::updateParticipantsContainerSize() {
    if (m_screenFullscreenActive) {
        m_participantsContainer->hide();
        return;
    }

    if (m_participantWidgets.isEmpty()) {
        m_participantsContainer->setMaximumHeight(0);
        m_participantsContainer->setMinimumHeight(0);
        m_participantsContainer->hide();
        return;
    }

    m_participantsContainer->show();

    int maxPerRow = calculateMaxParticipantsPerRow();
    bool isMainScreenVisible = m_mainScreen && m_mainScreen->isVisible();
    int widgetWidth = isMainScreenVisible ? scale(240) : scale(320);
    int widgetHeight = isMainScreenVisible ? scale(135) : scale(180);
    int spacing = scale(15);
    int margins = scale(10) * 2;
    int horizontalMargins = scale(10) * 2;

    int participantsPerPage = calculateParticipantsPerPage();
    int rowsPerPage = (participantsPerPage + maxPerRow - 1) / maxPerRow;
    int maxHeight = rowsPerPage * widgetHeight + (rowsPerPage - 1) * spacing + margins;
    
    int totalParticipants = m_participantWidgets.size();
    int actualParticipantsInRow = qMin(totalParticipants, maxPerRow);
    int panelWidth = actualParticipantsInRow * widgetWidth
        + (actualParticipantsInRow > 0 ? (actualParticipantsInRow - 1) * spacing : 0)
        + horizontalMargins;
    
    int buttonSpacing = scale(10);
    int buttonWidth = scale(40);
    int totalWidth = panelWidth;
    if (m_participantPanels.size() > 1) {
        // Когда есть несколько страниц, учитываем стрелки по краям.
        totalWidth += (buttonWidth * 2) + (buttonSpacing * 2);
    }

    int maxAllowedHeight = height() * 0.6;

    // Контейнер по ширине подстраивается под общую ширину панелей + стрелок
    // и центрируется основным лейаутом MeetingWidget.
    m_participantsContainer->setMinimumWidth(totalWidth);
    m_participantsContainer->setMaximumWidth(totalWidth);
    m_participantsContainer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Maximum);

    m_participantsContainer->setMinimumHeight(qMin(maxHeight, maxAllowedHeight));
    m_participantsContainer->setMaximumHeight(qMin(maxHeight, maxAllowedHeight));

    for (QWidget* panel : m_participantPanels) {
        panel->setMinimumWidth(panelWidth);
        panel->setMaximumWidth(panelWidth);
        panel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    // Реальное позиционирование стрелок делаем в updateNavigationButtons(),
    // когда уже известен текущий pageIndex и применён layout.
}

void MeetingWidget::applyStandardSize() {
    QSize targetSize;

    if (m_hasScreenSharing || !m_participantWidgets.isEmpty()) {
        applyDecreasedSize();
        return;
    }
    { 
        QSize availableSize = size();
        targetSize = scaledScreenSize16by9(1440);

        int reservedHeight = scale(100);
        int availableHeight = availableSize.height() - reservedHeight;
        if (availableHeight > 0 && targetSize.height() > availableHeight) {
            int adjustedWidth = static_cast<int>(availableHeight * 16.0 / 9.0);
            targetSize = QSize(adjustedWidth, availableHeight);
        }

        m_mainScreen->setRoundedCornersEnabled(true);
        m_mainScreen->setMinimumSize(100, 100);
        m_mainScreen->setMaximumSize(targetSize);
    }
}

void MeetingWidget::applyDecreasedSize() {
    QSize targetSize = scaledScreenSize16by9(scale(1280));

    QSize availableSize = size();
    int reservedHeight = scale(100);
    if (!m_participantWidgets.isEmpty()) {
        int participantsHeight = m_participantsContainer->isVisible() ? m_participantsContainer->height() : 0;
        reservedHeight += participantsHeight > 0 ? participantsHeight : scale(200);
    }
    int availableHeight = availableSize.height() - reservedHeight;
    if (availableHeight > 0 && targetSize.height() > availableHeight) {
        int adjustedWidth = static_cast<int>(availableHeight * 16.0 / 9.0);
        targetSize = QSize(adjustedWidth, availableHeight);
    }

    m_mainScreen->setMinimumSize(100, 100);
    m_mainScreen->setMaximumSize(targetSize);
}

void MeetingWidget::applyFullscreenSize() {
    QSize availableSize = size();
    auto width = availableSize.width();
    auto height = availableSize.height();

    m_mainScreen->setRoundedCornersEnabled(false);
    m_mainScreen->setFixedSize(width, height);
}

void MeetingWidget::updateMainScreenSize() {
    if (m_screenFullscreenActive) {
        applyFullscreenSize();
    }
    else {
        if (m_participantWidgets.isEmpty())
            applyStandardSize();
        else
            applyDecreasedSize();
    }
}

QSize MeetingWidget::scaledScreenSize16by9(int baseWidth) {
    const int scaledWidth = extraScale(baseWidth, 4) - scale(15);
    if (scaledWidth <= 0) return QSize();

    const int scaledHeight = std::max(1, static_cast<int>(std::lround(scaledWidth * 9.0 / 16.0)));
    return QSize(scaledWidth, scaledHeight);
}

void MeetingWidget::showFrameInMainScreen(const QPixmap& frame, Screen::ScaleMode scaleMode) {
    if (frame.isNull()) return;

    m_mainScreen->setScaleMode(scaleMode);
    m_mainScreen->setPixmap(frame);

    if (!m_mainScreen->isVisible()) {
        m_mainScreen->show();
        if (!m_participantWidgets.isEmpty()) {
            m_participantsContainer->show();
            updateParticipantPanels();
            updateParticipantsContainerSize();
        }
        updateMainScreenSize();
    }
}

void MeetingWidget::setHangupButtonRestricted(bool restricted) {
    if (!m_hangupButton) return;
    m_hangupButton->setEnabled(!restricted);
    if (restricted) {
        m_hangupButton->setStyleSheet(StyleMeetingWidget::disabledHangupButtonStyle());
    } else {
        m_hangupButton->setStyleSheet(StyleMeetingWidget::hangupButtonStyle());
    }
}

void MeetingWidget::setScreenShareButtonRestricted(bool restricted) {
    if (restricted) {
        m_screenShareButton->setDisabled(true);
        m_screenShareButton->setToolTip("Share disabled: remote screen is being shared");
        m_screenShareButton->setIcons(m_screenShareIconRestricted, m_screenShareIconRestricted,
            m_screenShareIconRestricted, m_screenShareIconRestricted);
    } else {
        m_screenShareButton->setDisabled(false);
        m_screenShareButton->setIcons(m_screenShareIconNormal, m_screenShareIconHover,
            m_screenShareIconActive, m_screenShareIconActiveHover);
        // Don't force-toggle off here: restriction changes must not desync UI from actual sharing state.
        m_screenShareButton->setToolTip(m_screenShareButton->isToggled() ? "Stop screen share" : "Start screen share");
    }
}

void MeetingWidget::setScreenShareButtonActive(bool active) {
    m_screenShareButton->setDisabled(false);
    m_hasScreenSharing = active;

    if (active) {
        m_screenShareButton->setIcons(m_screenShareIconNormal, m_screenShareIconHover,
            m_screenShareIconActive, m_screenShareIconActiveHover);
        m_screenShareButton->setToggled(true);
        m_screenShareButton->setToolTip("Stop screen share");

        m_participantsContainer->hide();
    }
    else {
        m_screenShareButton->setIcons(m_screenShareIconNormal, m_screenShareIconHover,
            m_screenShareIconActive, m_screenShareIconActiveHover);
        m_screenShareButton->setToggled(false);
        m_screenShareButton->setToolTip("Start screen share");

        if (!m_participantWidgets.isEmpty()) {
            m_participantsContainer->show();
        }
    }

    updateMainScreenSize();
}

void MeetingWidget::enterFullscreen() {
    m_screenFullscreenActive = true;

    m_topMainLayoutSpacer->changeSize(0, 0, QSizePolicy::Minimum, QSizePolicy::Fixed);
    m_mainLayout->setSpacing(0);

    m_enterFullscreenButton->hide();
    m_buttonsPanel->hide();

    if (m_settingsButton)
        m_settingsButton->hide();
    if (m_callNamePanel)
        m_callNamePanel->hide();

    m_participantsContainer->hide();

    setMouseTracking(true);
    showOverlayWithTimeout();

    m_mainLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    m_mainLayout->invalidate();
    applyFullscreenSize();
}

void MeetingWidget::setAudioSettingsDialogOpen(bool open) {
    m_audioSettingsDialogOpen = open;
    if (open) {
        if (m_overlayHideTimer)
            m_overlayHideTimer->stop();
    } else {
        if (!m_screenFullscreenActive) {
            if (m_settingsButton) m_settingsButton->hide();
            if (m_callNamePanel) m_callNamePanel->hide();
            if (m_joinRequestsPanel) m_joinRequestsPanel->hide();
        }
    }
    updateOverlayButtonsPosition();
}

void MeetingWidget::exitFullscreen() {
    m_screenFullscreenActive = false;

    m_topMainLayoutSpacer->changeSize(0, 0, QSizePolicy::Minimum, QSizePolicy::Fixed);

    m_enterFullscreenButton->show();
    m_buttonsPanel->show();

    m_exitFullscreenButton->hide();
    updateOverlayButtonsPosition();

    setMouseTracking(true);
    m_exitFullscreenHideTimer->stop();
    showOverlayWithTimeout();

    if (!m_participantWidgets.isEmpty() && !m_hasScreenSharing) {
        m_participantsContainer->show();
    }

    m_mainLayout->setSpacing(scale(10));
    m_mainLayout->setAlignment(Qt::AlignCenter);
    m_mainLayout->invalidate();
    updateMainScreenSize();
}

void MeetingWidget::hideMainScreen() {
    if (m_mainScreen) {
        m_mainScreen->clear();
        m_mainScreen->hide();
    }

    if (!m_participantWidgets.isEmpty() && !m_hasScreenSharing) {
        m_participantsContainer->show();
        updateParticipantPanels();
        updateParticipantsContainerSize();
    }
    
    updateMainScreenSize();
}

void MeetingWidget::updateOverlayButtonsPosition() {
    int margin = scale(14);

    int buttonSize = m_settingsButton ? m_settingsButton->height() : scale(32);
    int x = width() - buttonSize - margin;
    int y = margin;

    // If call name badge is visible, align settings button vertically with its center
    if (m_callNamePanel && m_callNamePanel->isVisible()) {
        int panelY = m_callNamePanel->y();
        int panelH = m_callNamePanel->height();
        if (panelH > 0) {
            y = panelY + (panelH - buttonSize) / 2;
        }
    }

    if (m_exitFullscreenButton) {
        m_exitFullscreenButton->move(x, y);
    }
    if (m_settingsButton) {
        m_settingsButton->move(x, y);
    }
}

void MeetingWidget::keyPressEvent(QKeyEvent* event) {
    if (event && event->key() == Qt::Key_Escape && m_screenFullscreenActive) {
        emit requestExitFullscreen();
        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
}

void MeetingWidget::mousePressEvent(QMouseEvent* event) {
    showOverlayWithTimeout();
    QWidget::mousePressEvent(event);
}

void MeetingWidget::mouseMoveEvent(QMouseEvent* event)
{
    showOverlayWithTimeout();
    QWidget::mouseMoveEvent(event);
}

void MeetingWidget::onExitFullscreenHideTimerTimeout() {
    if (m_screenFullscreenActive && m_exitFullscreenButton)
        m_exitFullscreenButton->hide();
}

void MeetingWidget::setCameraButtonRestricted(bool restricted) {
    if (!m_cameraButton) return;
    if (restricted) {
        m_cameraButton->setDisabled(true);
        m_cameraButton->setToolTip("Camera disabled: screen is being shared or camera is active");
        m_cameraButton->setIcons(m_cameraIconRestricted, m_cameraIconRestricted,
            m_cameraIconRestricted, m_cameraIconRestricted);
    } else {
        m_cameraButton->setDisabled(false);
        m_cameraButton->setIcons(m_cameraIconDisabled, m_cameraIconDisabledHover,
            m_cameraIconActive, m_cameraIconActiveHover);
        // Don't force-toggle off here: restriction changes must not desync UI from actual camera sharing state.
        m_cameraButton->setToolTip(m_cameraButton->isToggled() ? "Disable camera" : "Enable camera");
    }
}

void MeetingWidget::showErrorNotification(const QString& text, int durationMs) {
    if (!m_notificationWidget || !m_notificationLabel) return;

    m_notificationLabel->setText(text);
    m_notificationWidget->show();
    m_notificationTimer->start(durationMs);
}

void MeetingWidget::setCameraButtonActive(bool active) {
    if (!m_cameraButton) return;

    m_cameraButton->setDisabled(false);

    if (active) {
        m_cameraButton->setIcons(m_cameraIconDisabled, m_cameraIconDisabledHover,
            m_cameraIconActive, m_cameraIconActiveHover);
        m_cameraButton->setToggled(true);
        m_cameraButton->setToolTip("Disable camera");
    }
    else {
        m_cameraButton->setIcons(m_cameraIconDisabled, m_cameraIconDisabledHover,
            m_cameraIconActive, m_cameraIconActiveHover);
        m_cameraButton->setToggled(false);
        m_cameraButton->setToolTip("Enable camera");
    }
}

void MeetingWidget::showEnterFullscreenButton() {
    m_enterFullscreenButton->show();
}

void MeetingWidget::hideEnterFullscreenButton() {
    m_enterFullscreenButton->hide();
}

void MeetingWidget::setIsOwner(bool isOwner) {
    m_isOwner = isOwner;
}
