#include "settingsPanelWidget.h"
#include "buttons.h"
#include <QIcon>

#include "calls.h"

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
    return QString(
        "QSlider::groove:horizontal {"
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "       stop:0 #E0E0E0, stop:1 #F0F0F0);"
        "   height: 4px;"
        "   border-radius: 2px;"
        "}"
        "QSlider::handle:horizontal {"
        "   background: %1;"
        "   border: 2px solid %2;"
        "   width: 16px;"
        "   height: 16px;"
        "   border-radius: 8px;"
        "   margin: -6px 0;"
        "}"
        "QSlider::handle:horizontal:hover {"
        "   background: %2;"
        "   border: 2px solid %1;"
        "}"
        "QSlider::sub-page:horizontal {"
        "   background: %1;"
        "   height: 4px;"
        "   border-radius: 2px;"
        "}"
    ).arg(primaryColor.name()).arg(hoverColor.name());
}

QString SettingsPanel::StyleSettingsPanel::refreshButtonStyle() {
    return QString(
        "QPushButton {"
        "   background-color: #e0e0e0;"
        "   color: #333333;"
        "   border: 0px solid #d0d0d0;"
        "   border-radius: 8px;"
        "   padding: 8px 16px;"
        "   margin: 10px 0px 8px 0px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #d9d9d9;"
        "   border-color: #c0c0c0;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #d9d9d9;"
        "}"
        "QPushButton:focus {"
        "   outline: none;"
        "   border-color: #a0a0a0;"
        "}"
    );
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
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(10);

    // Refresh button with icon
    QHBoxLayout* refreshLayout = new QHBoxLayout();
    refreshLayout->setContentsMargins(0, 0, 0, 0);

    m_refreshButton = new QPushButton("  Refresh", this);
    m_refreshButton->setIcon(QIcon(":/resources/reload.png"));
    m_refreshButton->setIconSize(QSize(32, 28));
    m_refreshButton->setStyleSheet(StyleSettingsPanel::refreshButtonStyle());
    QFont refreshButtonFont("Outfit", 13, QFont::Medium);
    m_refreshButton->setFont(refreshButtonFont);

    m_refreshCooldownTimer = new QTimer(this);
    m_refreshCooldownTimer->setSingleShot(true);
    m_refreshEnabled = true;


    // Toggle кнопка mute
    m_muteButton = new QPushButton(this);
    m_muteButton->setCheckable(true); // Делаем кнопку toggle
    m_muteButton->setChecked(false); // По умолчанию выключено
    m_muteButton->setIcon(QIcon(":/resources/mute-microphone.png")); // Иконка когда не muted
    m_muteButton->setIconSize(QSize(26, 26));
    m_muteButton->setToolTip("Mute Audio");
    m_muteButton->setStyleSheet(
        "QPushButton {"
        "   background-color: transparent;"
        "   border: none;"
        "   padding: 5px;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgba(0, 0, 0, 0.1);"
        "}"
        "QPushButton:checked {"
        "   background-color: rgba(255, 0, 0, 0.2);"
        "}"
        "QPushButton:checked:hover {"
        "   background-color: rgba(255, 0, 0, 0.3);"
        "}"
    );

    refreshLayout->addWidget(m_refreshButton);
    refreshLayout->addStretch();
    refreshLayout->addWidget(m_muteButton);

    // Microphone section - микрофон, уровень громкости и mute
    QHBoxLayout* micLayout = new QHBoxLayout();
    micLayout->setContentsMargins(0, 0, 0, 0);

    // Microphone slider
    m_micSlider = new QSlider(Qt::Horizontal, this);
    m_micSlider->setRange(0, 100);
    m_micSlider->setValue(50);
    m_micSlider->setFixedHeight(30);
    m_micSlider->setStyleSheet(R"(
        QSlider::groove:horizontal {
            background-color: rgb(77, 77, 77); 
            height: 8px; 
            border-radius: 4px;
            margin: 0px 0px; 
        }
        QSlider::handle:horizontal {
            background-color: white;
            width: 16px;
            height: 16px; 
            border-radius: 8px;
            margin: -4px 0;
        }
        QSlider::add-page:horizontal {
            background-color: rgb(77, 77, 77);
            border-radius: 4px;
        }
        QSlider::sub-page:horizontal {
            background-color: rgb(21, 119, 232);
            border-radius: 4px;
        }
    )");

    m_micMuteButton = new ButtonIcon(this,
        QIcon(":/resources/microphone.png"),
        QIcon(":/resources/microphone.png"),
        24, 24);

    micLayout->addWidget(m_micMuteButton);
    micLayout->addWidget(m_micSlider, 1);

    // Speaker section - спикер, уровень громкости
    QHBoxLayout* speakerLayout = new QHBoxLayout();
    speakerLayout->setContentsMargins(0, 0, 0, 0);

    // Speaker slider
    m_speakerSlider = new QSlider(Qt::Horizontal, this);
    m_speakerSlider->setRange(0, 100);
    m_speakerSlider->setValue(50);
    m_speakerSlider->setFixedHeight(30);
    m_speakerSlider->setStyleSheet(R"(
        QSlider::groove:horizontal {
            background-color: rgb(77, 77, 77); 
            height: 8px; 
            border-radius: 4px;
            margin: 0px 0px; 
        }
        QSlider::handle:horizontal {
            background-color: white;
            width: 16px;
            height: 16px; 
            border-radius: 8px;
            margin: -4px 0;
        }
        QSlider::add-page:horizontal {
            background-color: rgb(77, 77, 77);
            border-radius: 4px;
        }
        QSlider::sub-page:horizontal {
            background-color: rgb(21, 119, 232);
            border-radius: 4px;
        }
    )");

    ButtonIcon* speakerIconButton = new ButtonIcon(this,
        QIcon(":/resources/speaker.png"),
        QIcon(":/resources/speaker.png"),
        22, 22);

    speakerLayout->addSpacing(2);
    speakerLayout->addWidget(speakerIconButton);
    speakerLayout->addSpacing(3);
    speakerLayout->addWidget(m_speakerSlider, 1);

    // Add widgets to layout
    mainLayout->addLayout(refreshLayout);
    mainLayout->addLayout(micLayout);
    mainLayout->addLayout(speakerLayout);

    // Connect signals
    connect(m_micSlider, &QSlider::valueChanged, this, &SettingsPanel::onMicVolumeChanged);
    connect(m_speakerSlider, &QSlider::valueChanged, this, &SettingsPanel::onSpeakerVolumeChanged);
    connect(m_muteButton, &QPushButton::toggled, this, &SettingsPanel::onMicMuteClicked);
    connect(m_refreshButton, &QPushButton::clicked, this, [this]() {
        if (m_refreshEnabled) {
            calls::refreshAudioDevices();

            m_refreshEnabled = false;
            m_refreshButton->setEnabled(false);
            m_refreshButton->setText("  Wait...");

            m_refreshButton->setStyleSheet(
                StyleSettingsPanel::refreshButtonStyle() +
                "QPushButton { color: #999; }"
            );

            m_refreshCooldownTimer->start(2000);
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
    calls::setInputVolume(volume);
}

void SettingsPanel::onSpeakerVolumeChanged(int volume) {
    calls::setOutputVolume(volume);
}

void SettingsPanel::onMicMuteClicked()
{
    m_isMicMuted = !m_isMicMuted;

    if (m_isMicMuted)
    {
        if (m_micSlider)
        {
            m_micVolume = m_micSlider->value();
            m_micSlider->setEnabled(false);
            m_micSlider->setValue(0);
            calls::mute(true);
        }
    }
    else
    {
        if (m_micSlider)
        {
            m_micSlider->setEnabled(true);
            m_micSlider->setValue(m_micVolume);
            calls::mute(false);
        }
    }
}