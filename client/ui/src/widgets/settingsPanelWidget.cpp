#include "settingsPanelWidget.h"
#include "widgets/components/button.h"
#include <QIcon>
#include "utilities/utilities.h"
#include "constants/constant.h"
#include "constants/color.h"

const QColor SettingsPanel::StyleSettingsPanel::primaryColor = COLOR_SETTINGS_ACCENT;
const QColor SettingsPanel::StyleSettingsPanel::hoverColor = COLOR_SETTINGS_ACCENT_HOVER;
const QColor SettingsPanel::StyleSettingsPanel::backgroundColor = COLOR_BACKGROUND_MAIN;
const QColor SettingsPanel::StyleSettingsPanel::textColor = COLOR_TEXT_MAIN;
const QColor SettingsPanel::StyleSettingsPanel::containerColor = COLOR_OVERLAY_PURE_200;

QString SettingsPanel::StyleSettingsPanel::containerStyle() {
    return QString("QWidget {"
        "   background-color: %1;"
        "   border-radius: %2px;"
        "   padding: 0px;"
        "}")
        .arg(containerColor.name())
        .arg(scale(20));
}

QString SettingsPanel::StyleSettingsPanel::titleStyle() {
    return QString("QLabel {"
        "   color: %1;"
        "   font-size: %2px;"
        "   font-weight: bold;"
        "   margin-bottom: %3px;"
        "   text-align: center;"
        "}")
        .arg(textColor.name())
        .arg(scale(14))
        .arg(scale(10));
}

QString SettingsPanel::StyleSettingsPanel::sliderStyle() {
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
            background-color: %7;
            border-radius: %6px;
        }
        QSlider::sub-page:horizontal {
            background-color: %8;
            border-radius: %6px;
        }
        QSlider::disabled {
            background-color: transparent;
        }
        QSlider::groove:horizontal:disabled {
            background-color: %9;
        }
        QSlider::handle:horizontal:disabled {
            background-color: %10;
        }
        QSlider::add-page:horizontal:disabled {
            background-color: %9;
        }
        QSlider::sub-page:horizontal:disabled {
            background-color: %11;
        }
    )")
        .arg(scale(8))
        .arg(scale(4))
        .arg(scale(17))
        .arg(scale(17))
        .arg(scale(8))
        .arg(scale(4))
        .arg(COLOR_SLIDER_TRACK.name())
        .arg(COLOR_SLIDER_FILL.name())
        .arg(COLOR_NEUTRAL_180.name())
        .arg(COLOR_NEUTRAL_200.name())
        .arg(COLOR_SLIDER_HANDLE_NEUTRAL.name());
}


QString SettingsPanel::StyleSettingsPanel::refreshButtonStyle() {
    return QString(
        "QPushButton {"
        "   background-color: %1;"
        "   color: %2;"
        "   border: 0px solid %10;"
        "   border-radius: %3px;"
        "   padding: %4px %5px;"
        "   margin: %6px %7px %8px %9px;"
        "}"
        "QPushButton:hover {"
        "   background-color: %11;"
        "   border-color: #c0c0c0;"
        "}"
        "QPushButton:pressed {"
        "   background-color: %12;"
        "}"
        "QPushButton:focus {"
        "   outline: none;"
        "   border-color: %13;"
        "}"
    )
        .arg(COLOR_OVERLAY_SETTINGS_190.name())   // %1 - background-color
        .arg(COLOR_TEXT_SECONDARY.name())         // %2 - color
        .arg(scale(8))                            // %3 - border-radius
        .arg(scale(8))                            // %4 - padding vertical
        .arg(scale(16))                           // %5 - padding horizontal
        .arg(scale(10))                           // %6 - margin top
        .arg(scale(0))                            // %7 - margin right
        .arg(scale(8))                            // %8 - margin bottom
        .arg(scale(0))                            // %9 - margin left
        .arg(COLOR_BORDER_NEUTRAL.name())           // %10 - border color
        .arg(COLOR_OVERLAY_SETTINGS_110.name())     // %11 - hover background-color
        .arg(COLOR_OVERLAY_SETTINGS_110.name())     // %12 - pressed background-color
        .arg(COLOR_OVERLAY_SETTINGS_110.name());    // %13 - focus border-color
}

QString SettingsPanel::StyleSettingsPanel::volumeLabelStyle() {
    return QString(
        "QLabel {"
        "   color: %1;"
        "   font-size: %2px;"
        "   font-weight: bold;"
        "   margin: %3px 0px;"
        "}"
    )
        .arg(COLOR_TEXT_SECONDARY.name())
        .arg(scale(13))
        .arg(scale(5));
}

QString SettingsPanel::StyleSettingsPanel::volumeValueStyle() {
    return QString(
        "QLabel {"
        "   color: %1;"
        "   font-size: 12px;"
        "   margin: 5px 0px;"
        "   min-width: 30px;"
        "}"
    );
}

QString SettingsPanel::StyleSettingsPanel::settingsPanelStyle() {
    return QString("QWidget {"
        "   background-color: transparent;"
        "   border-radius: %1px;"
        "   padding: %2px;"
        "}")
        .arg(scale(10))
        .arg(scale(15));
}

SettingsPanel::SettingsPanel(QWidget* parent) : QWidget(parent) {
    setupUI();
}

void SettingsPanel::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(scale(15), scale(15), scale(15), scale(15));
    mainLayout->setSpacing(scale(10));

    // Device picker button
    QHBoxLayout* refreshLayout = new QHBoxLayout();
    refreshLayout->setContentsMargins(0, 0, 0, 0);

    m_devicePickerButton = new QPushButton("Audio devices", this);
    m_devicePickerButton->setFixedHeight(scale(60));
    m_devicePickerButton->setStyleSheet(StyleSettingsPanel::refreshButtonStyle());
    QFont refreshButtonFont("Outfit", scale(13), QFont::Light);
    m_devicePickerButton->setFont(refreshButtonFont);
    m_devicePickerButton->setToolTip("Select input/output devices");
    m_devicePickerButton->setCursor(Qt::PointingHandCursor);

    // Camera button
    m_cameraButton = new ToggleButtonIcon(this,
        QIcon(":/resources/cameraDisabled.png"),
        QIcon(":/resources/cameraDisabledHover.png"),
        QIcon(":/resources/camera.png"),
        QIcon(":/resources/cameraHover.png"),
        scale(32), scale(32));
    m_cameraButton->setSize(scale(32), scale(32));
    m_cameraButton->setCursor(Qt::PointingHandCursor);

    refreshLayout->addWidget(m_devicePickerButton);
    refreshLayout->addStretch();
    refreshLayout->addWidget(m_cameraButton);
    refreshLayout->addSpacing(scale(12));


    // Microphone section - microphone, volume level and mute
    QHBoxLayout* micLayout = new QHBoxLayout();
    micLayout->setContentsMargins(0, 0, 0, 0);
    micLayout->setSpacing(scale(10));
    micLayout->setAlignment(Qt::AlignCenter);

    // Microphone mute button (toggle)
    m_micMuteButton = new ToggleButtonIcon(this,
        QIcon(":/resources/microphone.png"),
        QIcon(":/resources/microphoneHover.png"),
        QIcon(":/resources/mute-enabled-microphone.png"),
        QIcon(":/resources/mute-enabled-microphoneHover.png"),
        scale(24), scale(24));
    m_micMuteButton->setSize(scale(24), scale(24));
    m_micMuteButton->setCursor(Qt::PointingHandCursor);

    // Microphone slider
    m_micSlider = new QSlider(Qt::Horizontal, this);
    m_micSlider->setRange(MIN_VOLUME, MAX_VOLUME);
    m_micSlider->setValue(DEFAULT_VOLUME);
    m_micSlider->setFixedHeight(scale(30));
    m_micSlider->setStyleSheet(StyleSettingsPanel::sliderStyle());
    m_micSlider->setCursor(Qt::PointingHandCursor);
    m_micSlider->setTracking(true);

    micLayout->addWidget(m_micMuteButton);
    micLayout->addWidget(m_micSlider, 1);

    // Speaker section - speaker, volume level
    QHBoxLayout* speakerLayout = new QHBoxLayout();
    speakerLayout->setContentsMargins(0, 0, 0, 0);
    speakerLayout->setSpacing(scale(10));
    speakerLayout->setAlignment(Qt::AlignCenter);

    // Speaker mute button (toggle)
    m_speakerMuteButton = new ToggleButtonIcon(this,
        QIcon(":/resources/speaker.png"),
        QIcon(":/resources/speakerHover.png"),
        QIcon(":/resources/speakerMutedActive.png"),
        QIcon(":/resources/speakerMutedActiveHover.png"),
        scale(22), scale(22));
    m_speakerMuteButton->setSize(scale(22), scale(22));
    m_speakerMuteButton->setCursor(Qt::PointingHandCursor);

    // Speaker slider
    m_speakerSlider = new QSlider(Qt::Horizontal, this);
    m_speakerSlider->setRange(MIN_VOLUME, MAX_VOLUME);
    m_speakerSlider->setValue(DEFAULT_VOLUME);
    m_speakerSlider->setFixedHeight(scale(30));
    m_speakerSlider->setStyleSheet(StyleSettingsPanel::sliderStyle());
    m_speakerSlider->setCursor(Qt::PointingHandCursor);
    m_speakerSlider->setTracking(true);

    speakerLayout->addWidget(m_speakerMuteButton);
    speakerLayout->addWidget(m_speakerSlider, 1);

    // Add widgets to layout
    mainLayout->addLayout(refreshLayout);
    mainLayout->addLayout(micLayout);
    mainLayout->addLayout(speakerLayout);

    // Connect signals
    connect(m_micSlider, &QSlider::valueChanged, this, &SettingsPanel::onMicVolumeChanged);
    connect(m_speakerSlider, &QSlider::valueChanged, this, &SettingsPanel::onSpeakerVolumeChanged);
    connect(m_micMuteButton, &ToggleButtonIcon::toggled, this, &SettingsPanel::onMicMuteClicked);
    connect(m_speakerMuteButton, &ToggleButtonIcon::toggled, this, &SettingsPanel::onSpeakerMuteClicked);
    connect(m_cameraButton, &ToggleButtonIcon::toggled, this, &SettingsPanel::onCameraButtonClicked);
    connect(m_devicePickerButton, &QPushButton::clicked, this, [this]() {
        emit audioDevicePickerRequested();
    });

    setStyleSheet(StyleSettingsPanel::settingsPanelStyle());
}

void SettingsPanel::onMicVolumeChanged(int volume) {
    emit inputVolumeChanged(volume);
}

void SettingsPanel::onSpeakerVolumeChanged(int volume) {
    emit outputVolumeChanged(volume);
}

void SettingsPanel::setInputVolume(int volume) {
    m_micSlider->blockSignals(true);
    m_micSlider->setValue(volume);
    m_micSlider->blockSignals(false);
}

void SettingsPanel::setOutputVolume(int volume) {
    m_speakerSlider->blockSignals(true);
    m_speakerSlider->setValue(volume);
    m_speakerSlider->blockSignals(false);
}

void SettingsPanel::setMicrophoneMuted(bool muted) {
    if (m_micMuteButton->isToggled() != muted) {
        m_micMuteButton->setToggled(muted);
    }
    m_micSlider->setEnabled(!muted);
}

void SettingsPanel::setSpeakerMuted(bool muted) {
    if (m_speakerMuteButton->isToggled() != muted) {
        m_speakerMuteButton->setToggled(muted);
    }
    m_speakerSlider->setEnabled(!muted);
}

void SettingsPanel::onMicMuteClicked()
{
    const bool muted = m_micMuteButton->isToggled();
    if (m_micSlider) {
        m_micSlider->setEnabled(!muted);
    }
    emit muteMicrophoneClicked(muted);
}

void SettingsPanel::onSpeakerMuteClicked()
{
    const bool muted = m_speakerMuteButton->isToggled();
    if (m_speakerSlider) {
        m_speakerSlider->setEnabled(!muted);
    }
    emit muteSpeakerClicked(muted);
}

void SettingsPanel::onCameraButtonClicked(bool enabled)
{
    emit cameraButtonClicked(enabled);
}

void SettingsPanel::setCameraActive(bool active)
{
    if (m_cameraButton)
    {
        // When enabled, button is toggled (shows camera.png - active icons)
        // When disabled, button is not toggled (shows cameraDisabled.png - disabled icons)
        m_cameraButton->setToggled(active);
    }
}
