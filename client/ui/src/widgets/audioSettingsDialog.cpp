#include "audioSettingsDialog.h"

#include <QHBoxLayout>
#include <QGridLayout>
#include <unordered_set>

namespace {
    QString containerStyle(int radius, int padding)
    {
    return QString(
        "#audioSettingsContainer {"
        "   background-color: rgb(255, 255, 255);"
        "   border-radius: %1px;"
        "   padding: %2px;"
        "}"
        ).arg(radius).arg(padding);
    }
}

AudioSettingsDialog::AudioSettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setModal(false);

    m_container = new QWidget(this);
    m_container->setObjectName("audioSettingsContainer");

    m_inputDevicesArea = new QScrollArea(m_container);
    m_inputDevicesArea->setWidgetResizable(true);
    m_inputDevicesArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_inputDevicesArea->setFrameShape(QFrame::NoFrame);
    m_inputDevicesArea->setStyleSheet(scrollAreaStyle());

    m_outputDevicesArea = new QScrollArea(m_container);
    m_outputDevicesArea->setWidgetResizable(true);
    m_outputDevicesArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_outputDevicesArea->setFrameShape(QFrame::NoFrame);
    m_outputDevicesArea->setStyleSheet(scrollAreaStyle());

    m_inputDevicesWidget = new QWidget(m_inputDevicesArea);
    m_outputDevicesWidget = new QWidget(m_outputDevicesArea);
    const QString listBg = "background-color: rgba(255,255,255,230); border: none;";
    m_inputDevicesWidget->setStyleSheet(listBg);
    m_outputDevicesWidget->setStyleSheet(listBg);

    m_inputDevicesLayout = new QVBoxLayout(m_inputDevicesWidget);
    m_inputDevicesLayout->setContentsMargins(0, 0, scale(24), 0);
    m_inputDevicesLayout->setSpacing(scale(8));

    m_outputDevicesLayout = new QVBoxLayout(m_outputDevicesWidget);
    m_outputDevicesLayout->setContentsMargins(0, 0, scale(24), 0);
    m_outputDevicesLayout->setSpacing(scale(8));

    m_inputDevicesArea->setWidget(m_inputDevicesWidget);
    m_outputDevicesArea->setWidget(m_outputDevicesWidget);

    m_inputButtonsGroup = new QButtonGroup(this);
    m_inputButtonsGroup->setExclusive(true);
    m_outputButtonsGroup = new QButtonGroup(this);
    m_outputButtonsGroup->setExclusive(true);

    QLabel* inputLabel = new QLabel("Input device", m_container);
    QLabel* outputLabel = new QLabel("Output device", m_container);
    inputLabel->setStyleSheet("color: #010B13; font-weight: 600;");
    outputLabel->setStyleSheet("color: #010B13; font-weight: 600;");

    m_closeButton = new ButtonIcon(m_container, scale(28), scale(28));
    m_closeButton->setIcons(QIcon(":/resources/close.png"), QIcon(":/resources/closeHover.png"));
    m_closeButton->setSize(scale(28), scale(28));
    m_closeButton->setCursor(Qt::PointingHandCursor);

    auto* devicesLayout = new QGridLayout();
    devicesLayout->setContentsMargins(scale(8), 0, scale(8), 0);
    devicesLayout->setHorizontalSpacing(scale(10));
    devicesLayout->setVerticalSpacing(scale(10));
    devicesLayout->addWidget(inputLabel, 1, 0, 1, 1);
    devicesLayout->addWidget(m_inputDevicesArea, 2, 0, 1, 1);
    devicesLayout->addWidget(outputLabel, 1, 1, 1, 1);
    devicesLayout->addWidget(m_outputDevicesArea, 2, 1, 1, 1);
    devicesLayout->setColumnStretch(0, 1);
    devicesLayout->setColumnStretch(1, 1);
    devicesLayout->setColumnMinimumWidth(0, scale(200));
    devicesLayout->setColumnMinimumWidth(1, scale(200));

    m_micToggle = new ToggleButtonIcon(m_container,
        QIcon(":/resources/microphone.png"),
        QIcon(":/resources/microphoneHover.png"),
        QIcon(":/resources/mute-enabled-microphone.png"),
        QIcon(":/resources/mute-enabled-microphoneHover.png"),
        scale(28), scale(28));
    m_micToggle->setSize(scale(28), scale(28));
    m_micToggle->setCursor(Qt::PointingHandCursor);

    m_speakerToggle = new ToggleButtonIcon(m_container,
        QIcon(":/resources/speaker.png"),
        QIcon(":/resources/speakerHover.png"),
        QIcon(":/resources/speakerMutedActive.png"),
        QIcon(":/resources/speakerMutedActiveHover.png"),
        scale(26), scale(26));
    m_speakerToggle->setSize(scale(26), scale(26));
    m_speakerToggle->setCursor(Qt::PointingHandCursor);

    m_micSlider = new QSlider(Qt::Horizontal, m_container);
    m_micSlider->setRange(0, 200);
    m_micSlider->setValue(100);
    m_micSlider->setCursor(Qt::PointingHandCursor);
    m_micSlider->setTracking(false);
    m_micSlider->setStyleSheet(sliderStyle());

    m_speakerSlider = new QSlider(Qt::Horizontal, m_container);
    m_speakerSlider->setRange(0, 200);
    m_speakerSlider->setValue(100);
    m_speakerSlider->setCursor(Qt::PointingHandCursor);
    m_speakerSlider->setTracking(false);
    m_speakerSlider->setStyleSheet(sliderStyle());

    auto* micLayout = new QHBoxLayout();
    micLayout->setContentsMargins(0, 0, 0, 0);
    micLayout->setSpacing(scale(12));
    micLayout->addWidget(m_micToggle);
    micLayout->addWidget(m_micSlider);

    auto* speakerLayout = new QHBoxLayout();
    speakerLayout->setContentsMargins(0, 0, 0, 0);
    speakerLayout->setSpacing(scale(12));
    speakerLayout->addWidget(m_speakerToggle);
    speakerLayout->addWidget(m_speakerSlider);

    m_slidersContainer = new QWidget(m_container);
    auto* slidersLayout = new QVBoxLayout(m_slidersContainer);
    slidersLayout->setContentsMargins(0, 0, 0, 0);
    slidersLayout->setSpacing(scale(12));
    slidersLayout->addLayout(micLayout);
    slidersLayout->addLayout(speakerLayout);

    auto* containerLayout = new QVBoxLayout(m_container);
    containerLayout->setContentsMargins(scale(20), scale(20), scale(20), scale(20));
    containerLayout->setSpacing(scale(16));
    m_container->setMinimumWidth(scale(780));
    // header with close
    auto* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(scale(8), scale(8), scale(0), scale(4));
    headerLayout->addStretch();
    headerLayout->addWidget(m_closeButton);

    containerLayout->addLayout(headerLayout);
    containerLayout->addLayout(devicesLayout);
    containerLayout->addSpacing(scale(8));
    containerLayout->addWidget(m_slidersContainer);

    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(m_container);

    applyStyle();

    connect(m_inputButtonsGroup, &QButtonGroup::buttonClicked, this, [this](QAbstractButton* btn) {
        if (m_blockDeviceSignals || !btn) return;
        emit inputDeviceSelected(btn->property("deviceIndex").toInt());
    });
    connect(m_outputButtonsGroup, &QButtonGroup::buttonClicked, this, [this](QAbstractButton* btn) {
        if (m_blockDeviceSignals || !btn) return;
        emit outputDeviceSelected(btn->property("deviceIndex").toInt());
    });
    connect(m_micToggle, &ToggleButtonIcon::toggled, this, [this](bool toggled) {
        m_micSlider->setEnabled(!toggled);
        emit muteMicrophoneClicked(toggled);
    });
    connect(m_speakerToggle, &ToggleButtonIcon::toggled, this, [this](bool toggled) {
        m_speakerSlider->setEnabled(!toggled);
        emit muteSpeakerClicked(toggled);
    });
    connect(m_micSlider, &QSlider::valueChanged, this, [this](int volume) { emit inputVolumeChanged(volume); });
    connect(m_speakerSlider, &QSlider::valueChanged, this, [this](int volume) { emit outputVolumeChanged(volume); });
    connect(m_closeButton, &ButtonIcon::clicked, this, [this]() { emit closeRequested(); });
}

void AudioSettingsDialog::applyStyle()
{
    setStyleSheet(
        "QDialog { background: transparent; }"
        "QLabel { font-size: 14px; }" +
        containerStyle(scale(22), scale(8)) +
        sliderStyle()
    );
}

QString AudioSettingsDialog::sliderStyle() const
{
    return QString(R"(
        QSlider::groove:horizontal {
            height: %1px;
            border-radius: %2px;
            margin: 0px 0px;
            background: rgba(0,0,0,40);
        }
        QSlider::handle:horizontal {
            background-color: white;
            width: %3px;
            height: %4px;
            border-radius: %5px;
            margin: -4px 0;
            border: 1px solid rgba(0,0,0,30);
        }
        QSlider::add-page:horizontal {
            background-color: rgba(21, 119, 232, 120);
            border-radius: %6px;
        }
        QSlider::sub-page:horizontal {
            background-color: rgba(21, 119, 232, 220);
            border-radius: %6px;
        }
        QSlider::disabled {
            background-color: transparent;
        }
        QSlider::groove:horizontal:disabled {
            background-color: rgb(220, 220, 220);
        }
        QSlider::handle:horizontal:disabled {
            background-color: rgb(230, 230, 230);
            border: 1px solid rgba(0,0,0,20);
        }
        QSlider::add-page:horizontal:disabled {
            background-color: rgb(210, 210, 210);
        }
        QSlider::sub-page:horizontal:disabled {
            background-color: rgb(200, 200, 200);
        }
    )")
        .arg(QString::number(scale(8)))
        .arg(QString::number(scale(4)))
        .arg(QString::number(scale(10)))
        .arg(QString::number(scale(12)))
        .arg(QString::number(scale(6)))
        .arg(QString::number(scale(6)));
}

QString AudioSettingsDialog::scrollAreaStyle() const
{
    return QString(
        "QScrollArea {"
        "   border: none;"
        "   background: transparent;"
        "}"
        "QScrollBar:vertical {"
        "   background-color: transparent;"
        "   width: 6px;"
        "   margin: 0px;"
        "   border-radius: 3px;"
        "}"
        "QScrollBar::handle:vertical {"
        "   background-color: rgba(0, 0, 0, 60);"
        "   border-radius: 3px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "   height: 0px;"
        "}"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
        "   background: transparent;"
        "}"
    );
}

void AudioSettingsDialog::refreshDevices(int currentInputIndex, int currentOutputIndex)
{
    auto inputs = core::audio::AudioEngine::getInputDevices();
    auto outputs = core::audio::AudioEngine::getOutputDevices();

    if (currentInputIndex < 0) {
        currentInputIndex = core::audio::AudioEngine::getDefaultInputDeviceIndex();
    }
    if (currentOutputIndex < 0) {
        currentOutputIndex = core::audio::AudioEngine::getDefaultOutputDeviceIndex();
    }

    buildDeviceList(m_inputDevicesLayout, m_inputButtonsGroup, inputs, currentInputIndex, true);
    buildDeviceList(m_outputDevicesLayout, m_outputButtonsGroup, outputs, currentOutputIndex, false);

    // Ensure checked buttons reflect current indices
    if (m_inputButtonsGroup) {
        const auto buttons = m_inputButtonsGroup->buttons();
        for (auto* btn : buttons) {
            if (btn->property("deviceIndex").toInt() == currentInputIndex) {
                btn->setChecked(true);
            }
        }
    }
    if (m_outputButtonsGroup) {
        const auto buttons = m_outputButtonsGroup->buttons();
        for (auto* btn : buttons) {
            if (btn->property("deviceIndex").toInt() == currentOutputIndex) {
                btn->setChecked(true);
            }
        }
    }
}

void AudioSettingsDialog::setInputVolume(int volume)
{
    m_micSlider->setValue(volume);
}

void AudioSettingsDialog::setOutputVolume(int volume)
{
    m_speakerSlider->setValue(volume);
}

void AudioSettingsDialog::setMicrophoneMuted(bool muted)
{
    if (m_micToggle->isToggled() != muted) {
        m_micToggle->setToggled(muted);
    }
    m_micSlider->setEnabled(!muted);
}

void AudioSettingsDialog::setSpeakerMuted(bool muted)
{
    if (m_speakerToggle->isToggled() != muted) {
        m_speakerToggle->setToggled(muted);
    }
    m_speakerSlider->setEnabled(!muted);
}

int AudioSettingsDialog::currentInputDeviceIndex() const
{
    if (!m_inputButtonsGroup) return -1;
    auto btn = m_inputButtonsGroup->checkedButton();
    return btn ? btn->property("deviceIndex").toInt() : -1;
}

int AudioSettingsDialog::currentOutputDeviceIndex() const
{
    if (!m_outputButtonsGroup) return -1;
    auto btn = m_outputButtonsGroup->checkedButton();
    return btn ? btn->property("deviceIndex").toInt() : -1;
}

void AudioSettingsDialog::setSlidersVisible(bool visible)
{
    if (m_slidersContainer) {
        m_slidersContainer->setVisible(visible);
    }
}

void AudioSettingsDialog::clearLayout(QLayout* layout)
{
    if (!layout) return;
    QLayoutItem* child;
    while ((child = layout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        if (child->layout()) {
            clearLayout(child->layout());
        }
        delete child;
    }
}

void AudioSettingsDialog::buildDeviceList(QVBoxLayout* layout, QButtonGroup* group, const std::vector<core::audio::DeviceInfo>& devices, int currentIndex, bool isInput)
{
    if (!layout || !group) return;

    m_blockDeviceSignals = true;
    clearLayout(layout);
    group->blockSignals(true);

    // Remove previous buttons from the group to avoid stale entries/duplicates
    const auto oldButtons = group->buttons();
    for (auto* btn : oldButtons) {
        group->removeButton(btn);
    }

    group->setExclusive(true);

    std::unordered_set<std::string> seenNames;
    for (const auto& device : devices) {
        if (!seenNames.insert(device.name).second) {
            continue; // skip exact name duplicates
        }

        QString label = QString::fromStdString(device.name);
        if ((isInput && device.isDefaultInput) || (!isInput && device.isDefaultOutput)) {
            label += " (default)";
        }

        QPushButton* btn = new QPushButton(label);
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton {"
            " background-color: rgba(255,255,255,0);"
            " border: none;"
            " border-radius: 6px;"
            " padding: 8px 10px;"
            " color: #101010;"
            " text-align: left;"
            "}"
            "QPushButton:hover {"
            " background-color: rgba(21,119,232,18);"
            "}"
            "QPushButton:checked {"
            " background-color: rgba(21,119,232,36);"
            " color: #0c2a4f;"
            "}"
        );
        btn->setProperty("deviceIndex", device.deviceIndex);

        group->addButton(btn);
        layout->addWidget(btn);

        if (device.deviceIndex == currentIndex) {
            btn->setChecked(true);
        }
    }

    layout->addStretch();
    group->blockSignals(false);
    m_blockDeviceSignals = false;
}
