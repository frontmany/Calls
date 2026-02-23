#include "audioSettingsDialog.h"

#include <QFontMetrics>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSizePolicy>
#include <unordered_set>
#include "constants/color.h"

namespace {
    QString containerStyle(int radius, int padding)
    {
        return QString(
            "#audioSettingsContainer {"
            "   background-color: %1;"
            "   border-radius: %2px;"
            "   padding: %3px;"
            "}"
        ).arg(COLOR_BACKGROUND_PURE.name()).arg(radius).arg(padding);
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
    const QString listBg = QString("background-color: %1; border: none;").arg(COLOR_OVERLAY_PURE_230.name(QColor::HexArgb));
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

    QLabel* inputLabel = new QLabel("Microphone", m_container);
    QLabel* outputLabel = new QLabel("Speaker", m_container);
    inputLabel->setStyleSheet(QString("color: %1; font-weight: 600;").arg(COLOR_TEXT_MAIN.name()));
    outputLabel->setStyleSheet(QString("color: %1; font-weight: 600;").arg(COLOR_TEXT_MAIN.name()));

    m_closeButton = new ButtonIcon(m_container, scale(28), scale(28));
    m_closeButton->setIcons(QIcon(":/resources/close.png"), QIcon(":/resources/closeHover.png"));
    m_closeButton->setSize(scale(28), scale(28));
    m_closeButton->setCursor(Qt::PointingHandCursor);

    auto* devicesLayout = new QGridLayout();
    devicesLayout->setContentsMargins(scale(8), 0, scale(8), 0);
    devicesLayout->setHorizontalSpacing(scale(28));
    devicesLayout->setVerticalSpacing(scale(10));
    devicesLayout->addWidget(inputLabel, 1, 0, 1, 1);
    devicesLayout->addWidget(m_inputDevicesArea, 2, 0, 1, 1);
    devicesLayout->addWidget(outputLabel, 1, 1, 1, 1);
    devicesLayout->addWidget(m_outputDevicesArea, 2, 1, 1, 1);
    devicesLayout->setColumnStretch(0, 1);
    devicesLayout->setColumnStretch(1, 1);

    const int listMinWidth = scale(220);
    const int minContent = listMinWidth - scale(24) - scale(6) - scale(20) - scale(8) - scale(8) - scale(10) - scale(10);
    m_deviceNameMaxWidth = minContent * scale(2);

    devicesLayout->setColumnMinimumWidth(0, listMinWidth);
    devicesLayout->setColumnMinimumWidth(1, listMinWidth);

    m_inputDevicesArea->setMinimumWidth(listMinWidth);
    m_outputDevicesArea->setMinimumWidth(listMinWidth);
    m_inputDevicesArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_outputDevicesArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_micToggle = new ToggleButtonIcon(m_container,
        QIcon(":/resources/microphone.png"),
        QIcon(":/resources/microphoneHover.png"),
        QIcon(":/resources/mute-enabled-microphone.png"),
        QIcon(":/resources/mute-enabled-microphoneHover.png"),
        scale(22), scale(22));
    m_micToggle->setSize(scale(22), scale(22));
    m_micToggle->setCursor(Qt::PointingHandCursor);

    m_speakerToggle = new ToggleButtonIcon(m_container,
        QIcon(":/resources/speaker.png"),
        QIcon(":/resources/speakerHover.png"),
        QIcon(":/resources/speakerMutedActive.png"), 
        QIcon(":/resources/speakerMutedActiveHover.png"),
        scale(20), scale(20));
    m_speakerToggle->setSize(scale(20), scale(20));
    m_speakerToggle->setCursor(Qt::PointingHandCursor);

    m_micSlider = new QSlider(Qt::Horizontal, m_container);
    m_micSlider->setRange(0, 200);
    m_micSlider->setValue(100);
    m_micSlider->setCursor(Qt::PointingHandCursor);
    m_micSlider->setTracking(false);
    m_micSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_micSlider->setMinimumWidth(scale(260));
    m_micSlider->setStyleSheet(sliderStyle());

    m_speakerSlider = new QSlider(Qt::Horizontal, m_container);
    m_speakerSlider->setRange(0, 200);
    m_speakerSlider->setValue(100);
    m_speakerSlider->setCursor(Qt::PointingHandCursor);
    m_speakerSlider->setTracking(false);
    m_speakerSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_speakerSlider->setMinimumWidth(scale(260));
    m_speakerSlider->setStyleSheet(sliderStyle());

    auto* micLayout = new QHBoxLayout();
    micLayout->setContentsMargins(scale(5), 0, scale(18), 0);
    micLayout->setSpacing(scale(12));
    micLayout->addWidget(m_micToggle, 0, Qt::AlignVCenter);
    micLayout->addWidget(m_micSlider, 1);

    auto* speakerLayout = new QHBoxLayout();
    speakerLayout->setContentsMargins(scale(5), 0, scale(18), 0);
    speakerLayout->setSpacing(scale(12));
    speakerLayout->addWidget(m_speakerToggle, 0, Qt::AlignVCenter);
    speakerLayout->addWidget(m_speakerSlider, 1);

    m_slidersContainer = new QWidget(m_container);
    auto* slidersLayout = new QHBoxLayout(m_slidersContainer);
    slidersLayout->setContentsMargins(scale(5), scale(26), scale(18), 0);
    slidersLayout->setSpacing(scale(24));
    slidersLayout->addLayout(micLayout, 1);
    slidersLayout->addLayout(speakerLayout, 1);
    slidersLayout->setAlignment(Qt::AlignHCenter);

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
    containerLayout->addSpacing(scale(28));
    containerLayout->addWidget(m_slidersContainer);
    containerLayout->addSpacing(scale(8));

    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(m_container);

    applyStyle();

    connect(m_inputButtonsGroup, &QButtonGroup::buttonClicked, this, [this](QAbstractButton* btn) {
        if (m_blockDeviceSignals || !btn) return;
        emit inputDeviceSelected(btn->property("deviceIndex").toInt());
        updateDeviceLogos(m_inputButtonsGroup);
        });
    connect(m_outputButtonsGroup, &QButtonGroup::buttonClicked, this, [this](QAbstractButton* btn) {
        if (m_blockDeviceSignals || !btn) return;
        emit outputDeviceSelected(btn->property("deviceIndex").toInt());
        updateDeviceLogos(m_outputButtonsGroup);
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
        QString("QDialog { background: transparent; }"
        "QLabel { font-size: %1px; }").arg(scale(14)) +
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
        }
        QSlider::handle:horizontal {
            background-color: %7;
            width: %3px;
            height: %4px;
            border-radius: %5px;
            margin: %12px 0;
            border: none;
        }
        QSlider::add-page:horizontal {
            background-color: %8;
            border-radius: %6px;
        }
        QSlider::sub-page:horizontal {
            background-color: %9;
            border-radius: %6px;
        }
        QSlider::disabled {
            background-color: transparent;
        }
        QSlider::groove:horizontal:disabled {
            background-color: %10;
        }
        QSlider::handle:horizontal:disabled {
            background-color: %7;
        }
        QSlider::add-page:horizontal:disabled {
            background-color: %10;
        }
        QSlider::sub-page:horizontal:disabled {
            background-color: %11;
        }
    )")
        .arg(QString::number(scale(8)))
        .arg(QString::number(scale(4)))
        .arg(QString::number(scale(17)))
        .arg(QString::number(scale(17)))
        .arg(QString::number(scale(8)))
        .arg(QString::number(scale(4)))
        .arg(COLOR_NEUTRAL_200.name())
        .arg(COLOR_SLIDER_TRACK.name())
        .arg(COLOR_SLIDER_FILL.name())
        .arg(COLOR_NEUTRAL_180.name())
        .arg(COLOR_SLIDER_HANDLE_NEUTRAL.name())
        .arg(QString::number(scale(-4)));
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
        "   width: %2px;"
        "   margin: 0px;"
        "   border-radius: %3px;"
        "}"
        "QScrollBar::handle:vertical {"
        "   background-color: %1;"
        "   border-radius: %3px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "   background-color: %4;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "   height: 0px;"
        "}"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
        "   background: transparent;"
        "}"
    ).arg(COLOR_SHADOW_MEDIUM_60.name(QColor::HexArgb))
     .arg(scale(6))
     .arg(scale(3))
     .arg(COLOR_SHADOW_STRONG_80.name(QColor::HexArgb));
}

void AudioSettingsDialog::refreshDevices(int currentInputIndex, int currentOutputIndex)
{
    auto inputs = core::media::AudioEngine::getInputDevices();
    auto outputs = core::media::AudioEngine::getOutputDevices();

    if (currentInputIndex < 0) {
        currentInputIndex = core::media::AudioEngine::getDefaultInputDeviceIndex();
    }
    if (currentOutputIndex < 0) {
        currentOutputIndex = core::media::AudioEngine::getDefaultOutputDeviceIndex();
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

void AudioSettingsDialog::updateDeviceLogos(QButtonGroup* group)
{
    if (!group) return;
    for (auto* btn : group->buttons()) {
        auto logoPtr = btn->property("logoLabel").value<quintptr>();
        QLabel* logo = reinterpret_cast<QLabel*>(logoPtr);
        QWidget* row = reinterpret_cast<QWidget*>(btn->property("rowWidget").value<quintptr>());
        bool checked = btn->isChecked();
        if (logo) {
            logo->setVisible(checked);
        }
        if (row) {
            row->setStyleSheet(checked
                ? QString("background-color: %1; border-radius: %2px;").arg(COLOR_OVERLAY_ACCENT_36.name(QColor::HexArgb)).arg(scale(6))
                : QString("background-color: transparent; border-radius: %1px;").arg(scale(6)));
        }
    }
}

void AudioSettingsDialog::buildDeviceList(QVBoxLayout* layout, QButtonGroup* group, const std::vector<core::media::DeviceInfo>& devices, int currentIndex, bool isInput)
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

    QPixmap logoPixmap(":/resources/logo.png");
    if (!logoPixmap.isNull()) {
        logoPixmap = logoPixmap.scaled(scale(18), scale(18), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    std::unordered_set<std::string> seenNames;
    for (const auto& device : devices) {
        if (!seenNames.insert(device.name).second) {
            continue; // skip exact name duplicates
        }

        QString label = QString::fromStdString(device.name);
        if ((isInput && device.isDefaultInput) || (!isInput && device.isDefaultOutput)) {
            label += " (default)";
        }

        QFontMetrics fm(font());
        QString displayText = fm.elidedText(label, Qt::ElideRight, m_deviceNameMaxWidth);
        QPushButton* btn = new QPushButton(displayText);
        btn->setToolTip(label);
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            QString(
            "QPushButton {"
            " background-color: transparent;"
            " border: none;"
            " padding: %3px %4px;"
            " color: %1;"
            " text-align: left;"
            "}"
            "QPushButton:checked {"
            " color: %2;"
            "}"
            ).arg(COLOR_TEXT_SECONDARY.name()).arg(COLOR_TEXT_TERTIARY.name()).arg(scale(8)).arg(scale(10))
        );
        btn->setProperty("deviceIndex", device.deviceIndex);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        QLabel* logo = new QLabel();
        logo->setFixedSize(scale(20), scale(20));
        logo->setPixmap(logoPixmap);
        logo->setVisible(false);
        logo->setAttribute(Qt::WA_TranslucentBackground);
        logo->setStyleSheet("background-color: transparent;");

        QWidget* row = new QWidget();
        QHBoxLayout* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(scale(8));
        rowLayout->addWidget(btn, 1);
        rowLayout->addWidget(logo, 0, Qt::AlignRight | Qt::AlignVCenter);
        rowLayout->addSpacing(scale(8));

        btn->setProperty("logoLabel", QVariant::fromValue<quintptr>(reinterpret_cast<quintptr>(logo)));
        btn->setProperty("rowWidget", QVariant::fromValue<quintptr>(reinterpret_cast<quintptr>(row)));

        group->addButton(btn);
        layout->addWidget(row);

        if (device.deviceIndex == currentIndex) {
            btn->setChecked(true);
            logo->setVisible(true);
            row->setStyleSheet(QString("background-color: %1; border-radius: %2px;").arg(COLOR_OVERLAY_ACCENT_36.name(QColor::HexArgb)).arg(scale(6)));
        }
        else {
            row->setStyleSheet(QString("background-color: transparent; border-radius: %1px;").arg(scale(6)));
        }
    }

    layout->addStretch();
    group->blockSignals(false);
    m_blockDeviceSignals = false;
    updateDeviceLogos(group);
}
