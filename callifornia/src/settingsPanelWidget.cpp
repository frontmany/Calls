#include "settingsPanelWidget.h"
#include "buttons.h"
#include <QIcon>
#include "scaleFactor.h"

const QColor SettingsPanel::StyleSettingsPanel::primaryColor = QColor(224, 168, 0);
const QColor SettingsPanel::StyleSettingsPanel::hoverColor = QColor(219, 164, 0);
const QColor SettingsPanel::StyleSettingsPanel::backgroundColor = QColor(230, 230, 230);
const QColor SettingsPanel::StyleSettingsPanel::textColor = QColor(1, 11, 19);
const QColor SettingsPanel::StyleSettingsPanel::containerColor = QColor(255, 255, 255, 200);

QString SettingsPanel::StyleSettingsPanel::containerStyle() {
    return QString("QWidget {"
        "   background-color: %1;"
        "   border-radius: 20px;"
        "   padding: 0px;"
        "}").arg(containerColor.name());
}

QString SettingsPanel::StyleSettingsPanel::titleStyle() {
    return QString("QLabel {"
        "   color: %1;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   margin-bottom: 10px;"
        "   text-align: center;"
        "}").arg(textColor.name());
}

QString SettingsPanel::StyleSettingsPanel::sliderStyle() {
    return QString(R"(
        QSlider::groove:horizontal {
            background-color: rgb(77, 77, 77); 
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
    )")
        .arg(QString::fromStdString(std::to_string(scale(8))))
        .arg(QString::fromStdString(std::to_string(scale(4))))
        .arg(QString::fromStdString(std::to_string(scale(16))))
        .arg(QString::fromStdString(std::to_string(scale(16))))
        .arg(QString::fromStdString(std::to_string(scale(8))))  
        .arg(QString::fromStdString(std::to_string(scale(4))));
}

QString SettingsPanel::StyleSettingsPanel::refreshButtonStyle() {
    return QString(
        "QPushButton {"
        "   background-color: rgba(235, 235, 235, %1);"
        "   color: %2;"
        "   border: 0px solid #d0d0d0;"
        "   border-radius: %3px;"
        "   padding: %4px %5px;"
        "   margin: %6px %7px %8px %9px;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgba(235, 235, 235, %10);"
        "   border-color: #c0c0c0;"
        "}"
        "QPushButton:pressed {"
        "   background-color: rgba(235, 235, 235, %11);"
        "}"
        "QPushButton:focus {"
        "   outline: none;"
        "   border-color: rgba(235, 235, 235, %12);"
        "}"
    )
        .arg(QString::fromStdString(std::to_string(190)))    // background-color alpha
        .arg("#333333") // color
        .arg(QString::fromStdString(std::to_string(scale(8))))      // border-radius
        .arg(QString::fromStdString(std::to_string(scale(8))))      // padding vertical
        .arg(QString::fromStdString(std::to_string(scale(16))))     // padding horizontal
        .arg(QString::fromStdString(std::to_string(scale(10))))     // margin top
        .arg(QString::fromStdString(std::to_string(scale(0))))      // margin right
        .arg(QString::fromStdString(std::to_string(scale(8))))      // margin bottom
        .arg(QString::fromStdString(std::to_string(scale(0))))      // margin left
        .arg(QString::fromStdString(std::to_string(110)))    // hover background-color alpha
        .arg(QString::fromStdString(std::to_string(110)))    // pressed background-color alpha
        .arg(QString::fromStdString(std::to_string(110)));   // focus border-color alpha
}

QString SettingsPanel::StyleSettingsPanel::volumeLabelStyle() {
    return QString(
        "QLabel {"
        "   color: #333333;"
        "   font-size: 13px;"
        "   font-weight: bold;"
        "   margin: 5px 0px;"
        "}"
    );
}

QString SettingsPanel::StyleSettingsPanel::volumeValueStyle() {
    return QString(
        "QLabel {"
        "   color: #666666;"
        "   font-size: 12px;"
        "   margin: 5px 0px;"
        "   min-width: 30px;"
        "}"
    );
}

QString SettingsPanel::StyleSettingsPanel::settingsPanelStyle() {
    return QString("QWidget {"
        "   background-color: rgba(240, 240, 240, 200);"
        "   border-radius: 10px;"
        "   padding: 15px;"
        "}");
}

SettingsPanel::SettingsPanel(QWidget* parent) : QWidget(parent) {
    setupUI();
}

void SettingsPanel::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(scale(15), scale(15), scale(15), scale(15));
    mainLayout->setSpacing(scale(10));

    // Refresh button with icon
    QHBoxLayout* refreshLayout = new QHBoxLayout();
    refreshLayout->setContentsMargins(0, 0, 0, 0);

    m_refreshButton = new QPushButton("  Refresh", this);
    m_refreshButton->setFixedHeight(scale(60));
    m_refreshButton->setIcon(QIcon(":/resources/reload.png"));
    m_refreshButton->setIconSize(QSize(scale(32), scale(28)));
    m_refreshButton->setStyleSheet(StyleSettingsPanel::refreshButtonStyle());
    QFont refreshButtonFont("Outfit", scale(13), QFont::Medium);
    m_refreshButton->setFont(refreshButtonFont);
    m_refreshButton->setToolTip("Refresh audio devices if changed");

    m_refreshCooldownTimer = new QTimer(this);
    m_refreshCooldownTimer->setSingleShot(true);
    m_refreshEnabled = true;


    // Toggle кнопка mute
    m_muteButton = new ToggleButtonIcon(this, QIcon(":/resources/mute-microphone.png"), QIcon(":/resources/mute-microphoneHover.png"), QIcon(":/resources/mute-enabled-microphone.png"), QIcon(":/resources/mute-enabled-microphoneHover.png"), scale(40), scale(40));
    m_muteButton->setSize(scale(26), scale(26));

    refreshLayout->addWidget(m_refreshButton);
    refreshLayout->addStretch();
    refreshLayout->addWidget(m_muteButton);

    // Microphone section - микрофон, уровень громкости и mute
    QHBoxLayout* micLayout = new QHBoxLayout();
    micLayout->setContentsMargins(0, 0, 0, 0);

    // Microphone slider
    m_micSlider = new QSlider(Qt::Horizontal, this);
    m_micSlider->setRange(0, 100);
    m_micSlider->setValue(80);
    m_micSlider->setFixedHeight(scale(30));
    m_micSlider->setStyleSheet(StyleSettingsPanel::sliderStyle());

    m_micMuteButton = new ButtonIcon(this,
        QIcon(":/resources/microphone.png"),
        QIcon(":/resources/microphone.png"),
        scale(24), scale(24));

    micLayout->addWidget(m_micMuteButton);
    micLayout->addWidget(m_micSlider, 1);

    // Speaker section - спикер, уровень громкости
    QHBoxLayout* speakerLayout = new QHBoxLayout();
    speakerLayout->setContentsMargins(0, 0, 0, 0);

    // Speaker slider
    m_speakerSlider = new QSlider(Qt::Horizontal, this);
    m_speakerSlider->setRange(0, 100);
    m_speakerSlider->setValue(50);
    m_speakerSlider->setFixedHeight(scale(30));
    m_speakerSlider->setStyleSheet(StyleSettingsPanel::sliderStyle());

    ButtonIcon* speakerIconButton = new ButtonIcon(this,
        QIcon(":/resources/speaker.png"),
        QIcon(":/resources/speaker.png"),
        scale(22), scale(22));

    speakerLayout->addSpacing(scale(2));
    speakerLayout->addWidget(speakerIconButton);
    speakerLayout->addSpacing(scale(3));
    speakerLayout->addWidget(m_speakerSlider, 1);

    // Add widgets to layout
    mainLayout->addLayout(refreshLayout);
    mainLayout->addLayout(micLayout);
    mainLayout->addLayout(speakerLayout);

    // Connect signals
    connect(m_micSlider, &QSlider::valueChanged, this, &SettingsPanel::onMicVolumeChanged);
    connect(m_speakerSlider, &QSlider::valueChanged, this, &SettingsPanel::onSpeakerVolumeChanged);
    connect(m_muteButton, &ToggleButtonIcon::toggled, this, &SettingsPanel::onMicMuteClicked);
    connect(m_refreshButton, &QPushButton::clicked, this, [this]() {
        if (m_refreshEnabled) {
            m_refreshEnabled = false;
            m_refreshButton->setEnabled(false);
            m_refreshButton->setText("  Wait...");

            m_refreshButton->setStyleSheet(
                StyleSettingsPanel::refreshButtonStyle() +
                "QPushButton { color: #999; }"
            );

            m_refreshCooldownTimer->start(2000);
            emit refreshAudioDevicesButtonClicked();
        }
    });

    connect(m_refreshCooldownTimer, &QTimer::timeout, this, [this]() {
        m_refreshEnabled = true;
        m_refreshButton->setEnabled(true);
        m_refreshButton->setText("  Refresh");
        m_refreshButton->setStyleSheet(StyleSettingsPanel::refreshButtonStyle());
    });

    setStyleSheet(StyleSettingsPanel::settingsPanelStyle());
}

void SettingsPanel::onMicVolumeChanged(int volume) {
    if (!m_isMicMuted) {
        emit inputVolumeChanged(volume);
    }
}

void SettingsPanel::onSpeakerVolumeChanged(int volume) {
    emit outputVolumeChanged(volume);
}

void SettingsPanel::setInputVolume(int volume) {
    m_micSlider->setValue(volume);
}

void SettingsPanel::setOutputVolume(int volume) {
    m_speakerSlider->setValue(volume);
}

void SettingsPanel::setMuted(bool muted) {
    if (m_muteButton->isToggled() != muted) {
        m_muteButton->setToggled(true);
        m_muteButton->setToggled(true);
    }

    if (muted) {
        m_micSlider->setEnabled(false);
        emit muteButtonClicked(true);
    }
    else {
        m_micSlider->setEnabled(true);
        emit muteButtonClicked(false);
    }
}

void SettingsPanel::onMicMuteClicked()
{
    m_isMicMuted = !m_isMicMuted;

    if (m_isMicMuted)
    {
        if (m_micSlider)
        {
            m_micSlider->setEnabled(false);
            emit muteButtonClicked(true);
        }
    }
    else
    {
        if (m_micSlider)
        {
            m_micSlider->setEnabled(true);
            emit muteButtonClicked(false);
        }
    }
}