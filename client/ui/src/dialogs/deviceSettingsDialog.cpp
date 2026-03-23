#include "deviceSettingsDialog.h"

#include <tuple>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QResizeEvent>
#include <QSizePolicy>
#include <unordered_set>
#include "constants/color.h"

namespace {
    QString containerStyle(int radius, int padTop, int padRight, int padBottom)
    {
        return QString(
            "#deviceSettingsContainer {"
            "   background-color: %1;"
            "   border-radius: %2px;"
            "   padding-top: %3px;"
            "   padding-right: %4px;"
            "   padding-bottom: %5px;"
            "   padding-left: 0px;"
            "}"
        ).arg(COLOR_BACKGROUND_PURE.name())
         .arg(radius)
         .arg(padTop)
         .arg(padRight)
         .arg(padBottom);
    }

    QString deviceButtonStyleSheet()
    {
        return QString(
            "QPushButton {"
            " background-color: transparent;"
            " border: none;"
            " padding: %3px %4px;"
            " color: %1;"
            " text-align: left;"
            " font-size: %5px;"
            "}"
            "QPushButton:focus {"
            " outline: none;"
            " border: none;"
            "}"
            "QPushButton:checked {"
            " color: %2;"
            "}"
        ).arg(COLOR_TEXT_SECONDARY.name())
         .arg(COLOR_TEXT_TERTIARY.name())
         .arg(scale(11))
         .arg(scale(14))
         .arg(scale(14));
    }

    QString navDeviceButtonStyleSheet()
    {
        return QString(
            "QPushButton {"
            " background-color: transparent;"
            " border: none;"
            " padding: %3px %4px;"
            " color: %1;"
            " text-align: left;"
            " font-size: %5px;"
            "}"
            "QPushButton:focus {"
            " outline: none;"
            " border: none;"
            "}"
            "QPushButton:checked {"
            " background-color: transparent;"
            " color: %2;"
            "}"
        ).arg(COLOR_TEXT_SECONDARY.name())
         .arg(COLOR_TEXT_TERTIARY.name())
         .arg(scale(11))
         .arg(scale(14))
         .arg(scale(14));
    }

    QString panelsChromeStyle(int dialogRadius, int contentCornerRadius)
    {
        return QString(
            "QWidget#deviceSettingsNavPanel {"
            "   background-color: %1;"
            "   border: none;"
            "   border-top-left-radius: %2px;"
            "   border-bottom-left-radius: %2px;"
            "   border-top-right-radius: 0px;"
            "   border-bottom-right-radius: 0px;"
            "}"
            "QWidget#deviceSettingsContentPanel {"
            "   background-color: %3;"
            "   border: none;"
            "   border-top-left-radius: 0px;"
            "   border-bottom-left-radius: 0px;"
            "   border-top-right-radius: %4px;"
            "   border-bottom-right-radius: %4px;"
            "}"
        ).arg(COLOR_BACKGROUND_SUBTLE.name())
         .arg(dialogRadius)
         .arg(COLOR_BACKGROUND_PURE.name())
         .arg(contentCornerRadius);
    }

    QString sectionTitleStyle()
    {
        return QString(
            "color: %1;"
            " font-weight: 700;"
            " font-size: %2px;"
            " letter-spacing: 0.2px;"
            " padding: 0 0 %3px 0;"
        ).arg(COLOR_TEXT_MAIN.name())
         .arg(scale(19))
         .arg(scale(4));
    }
}

DeviceSettingsDialog::DeviceSettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setModal(false);

    m_container = new QWidget(this);
    m_container->setObjectName("deviceSettingsContainer");

    m_deviceCenterLabel = new QLabel("Device Center", m_container);
    m_deviceCenterLabel->setStyleSheet(QString("color: %1; font-weight: 700; font-size: %2px; letter-spacing: 0.3px; padding: 0 0 %3px %4px;")
        .arg(COLOR_TEXT_MAIN.name())
        .arg(scale(14))
        .arg(scale(2))
        .arg(scale(8)));

    m_navButtonGroup = new QButtonGroup(this);
    m_navButtonGroup->setExclusive(true);

    const int navIconPx = scale(18);
    auto makeNavButton = [this](const QString& text) {
        auto* btn = new QPushButton(text);
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(navDeviceButtonStyleSheet());
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        return btn;
    };

    m_navInputButton = makeNavButton("Input");
    m_navOutputButton = makeNavButton("Output");
    m_navCameraButton = makeNavButton("Camera");
    m_navInputButton->setProperty("stackIndex", 0);
    m_navOutputButton->setProperty("stackIndex", 1);
    m_navCameraButton->setProperty("stackIndex", 2);

    auto wrapNavRow = [navIconPx](QPushButton* btn, const QString& iconPath) -> QWidget* {
        auto* row = new QWidget();
        row->setObjectName("deviceSettingsNavItemRow");
        row->setAttribute(Qt::WA_StyledBackground, true);
        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(scale(20), 0, scale(6), 0);
        rowLayout->setSpacing(0);
        if (!iconPath.isEmpty()) {
            auto* iconLabel = new QLabel();
            iconLabel->setFixedSize(navIconPx, navIconPx);
            iconLabel->setAttribute(Qt::WA_TranslucentBackground);
            iconLabel->setStyleSheet("background: transparent;");
            QPixmap pm(iconPath);
            if (!pm.isNull()) {
                iconLabel->setPixmap(pm.scaled(navIconPx, navIconPx, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
            rowLayout->addWidget(iconLabel, 0, Qt::AlignVCenter);
        }
        rowLayout->addWidget(btn, 1);
        btn->setProperty("logoLabel", QVariant::fromValue<quintptr>(0));
        btn->setProperty("rowWidget", QVariant::fromValue<quintptr>(reinterpret_cast<quintptr>(row)));
        return row;
    };

    m_navInputRow = wrapNavRow(m_navInputButton, ":/resources/microphone.png");
    m_navOutputRow = wrapNavRow(m_navOutputButton, ":/resources/speaker.png");
    m_navCameraRow = wrapNavRow(m_navCameraButton, ":/resources/camera.png");

    m_navButtonGroup->addButton(m_navInputButton);
    m_navButtonGroup->addButton(m_navOutputButton);
    m_navButtonGroup->addButton(m_navCameraButton);
    m_navInputButton->setChecked(true);

    auto* navLayout = new QVBoxLayout();
    navLayout->setContentsMargins(scale(20), scale(16), scale(20), scale(22));
    navLayout->setSpacing(scale(12));
    navLayout->addWidget(m_deviceCenterLabel);
    navLayout->addSpacing(scale(4));
    navLayout->addWidget(m_navInputRow);
    navLayout->addWidget(m_navOutputRow);
    navLayout->addWidget(m_navCameraRow);
    navLayout->addStretch();

    auto* navColumn = new QWidget(m_container);
    navColumn->setObjectName("deviceSettingsNavPanel");
    navColumn->setLayout(navLayout);
    navColumn->setMinimumWidth(scale(200));
    navColumn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    m_stack = new QStackedWidget(m_container);

    auto makeScrollList = [this]() {
        auto* area = new QScrollArea(m_container);
        area->setWidgetResizable(true);
        area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        area->setFrameShape(QFrame::NoFrame);
        area->setStyleSheet(scrollAreaStyle());
        auto* w = new QWidget(area);
        w->setAttribute(Qt::WA_TranslucentBackground);
        w->setStyleSheet("background: transparent; border: none;");
        w->setMinimumWidth(0);
        w->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
        auto* lay = new QVBoxLayout(w);
        lay->setContentsMargins(0, 0, scale(4), 0);
        lay->setSpacing(scale(10));
        area->setWidget(w);
        area->setMinimumHeight(scale(240));
        area->setMaximumWidth(scale(440));
        area->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        return std::tuple{area, w, lay};
    };

    auto inList = makeScrollList();
    m_inputDevicesArea = std::get<0>(inList);
    m_inputDevicesWidget = std::get<1>(inList);
    m_inputDevicesLayout = std::get<2>(inList);

    auto outList = makeScrollList();
    m_outputDevicesArea = std::get<0>(outList);
    m_outputDevicesWidget = std::get<1>(outList);
    m_outputDevicesLayout = std::get<2>(outList);

    auto camList = makeScrollList();
    m_cameraDevicesArea = std::get<0>(camList);
    m_cameraDevicesWidget = std::get<1>(camList);
    m_cameraDevicesLayout = std::get<2>(camList);

    m_inputButtonsGroup = new QButtonGroup(this);
    m_inputButtonsGroup->setExclusive(true);
    m_outputButtonsGroup = new QButtonGroup(this);
    m_outputButtonsGroup->setExclusive(true);
    m_cameraButtonsGroup = new QButtonGroup(this);
    m_cameraButtonsGroup->setExclusive(true);

    m_micToggle = new ToggleButtonIcon(m_container,
        QIcon(":/resources/microphone.png"),
        QIcon(":/resources/microphoneHover.png"),
        QIcon(":/resources/mute-enabled-microphone.png"),
        QIcon(":/resources/mute-enabled-microphoneHover.png"),
        scale(22), scale(22));
    m_micToggle->setSize(scale(22), scale(22));
    m_micToggle->setCursor(Qt::PointingHandCursor);

    m_micSlider = new QSlider(Qt::Horizontal, m_container);
    m_micSlider->setRange(0, 200);
    m_micSlider->setValue(100);
    m_micSlider->setCursor(Qt::PointingHandCursor);
    m_micSlider->setTracking(false);
    m_micSlider->setFixedWidth(scale(360));
    m_micSlider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_micSlider->setStyleSheet(sliderStyle());

    auto* micRow = new QHBoxLayout();
    micRow->setContentsMargins(scale(14), scale(24), 0, scale(12));
    micRow->setSpacing(scale(16));
    micRow->addWidget(m_micToggle, 0, Qt::AlignVCenter);
    micRow->addWidget(m_micSlider, 0, Qt::AlignVCenter);
    micRow->addStretch();

    m_inputPage = new QWidget();
    auto* inputPageLayout = new QVBoxLayout(m_inputPage);
    inputPageLayout->setContentsMargins(0, 0, 0, 0);
    inputPageLayout->setSpacing(scale(16));
    QLabel* inputHdr = new QLabel("Microphone", m_inputPage);
    inputHdr->setStyleSheet(sectionTitleStyle());
    inputPageLayout->addWidget(inputHdr);
    inputPageLayout->addWidget(m_inputDevicesArea, 1);
    inputPageLayout->addLayout(micRow);

    m_speakerToggle = new ToggleButtonIcon(m_container,
        QIcon(":/resources/speaker.png"),
        QIcon(":/resources/speakerHover.png"),
        QIcon(":/resources/speakerMutedActive.png"),
        QIcon(":/resources/speakerMutedActiveHover.png"),
        scale(20), scale(20));
    m_speakerToggle->setSize(scale(20), scale(20));
    m_speakerToggle->setCursor(Qt::PointingHandCursor);

    m_speakerSlider = new QSlider(Qt::Horizontal, m_container);
    m_speakerSlider->setRange(0, 200);
    m_speakerSlider->setValue(100);
    m_speakerSlider->setCursor(Qt::PointingHandCursor);
    m_speakerSlider->setTracking(false);
    m_speakerSlider->setFixedWidth(scale(360));
    m_speakerSlider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_speakerSlider->setStyleSheet(sliderStyle());

    auto* speakerRow = new QHBoxLayout();
    speakerRow->setContentsMargins(scale(14), scale(24), 0, scale(12));
    speakerRow->setSpacing(scale(16));
    speakerRow->addWidget(m_speakerToggle, 0, Qt::AlignVCenter);
    speakerRow->addWidget(m_speakerSlider, 0, Qt::AlignVCenter);
    speakerRow->addStretch();

    m_outputPage = new QWidget();
    auto* outputPageLayout = new QVBoxLayout(m_outputPage);
    outputPageLayout->setContentsMargins(0, 0, 0, 0);
    outputPageLayout->setSpacing(scale(16));
    QLabel* outputHdr = new QLabel("Speaker", m_outputPage);
    outputHdr->setStyleSheet(sectionTitleStyle());
    outputPageLayout->addWidget(outputHdr);
    outputPageLayout->addWidget(m_outputDevicesArea, 1);
    outputPageLayout->addLayout(speakerRow);

    m_cameraPage = new QWidget();
    auto* cameraPageLayout = new QVBoxLayout(m_cameraPage);
    cameraPageLayout->setContentsMargins(0, 0, 0, 0);
    cameraPageLayout->setSpacing(scale(16));
    QLabel* cameraHdr = new QLabel("Camera", m_cameraPage);
    cameraHdr->setStyleSheet(sectionTitleStyle());
    cameraPageLayout->addWidget(cameraHdr);
    cameraPageLayout->addWidget(m_cameraDevicesArea, 1);

    m_stack->addWidget(m_inputPage);
    m_stack->addWidget(m_outputPage);
    m_stack->addWidget(m_cameraPage);

    m_closeButton = new ButtonIcon(m_container, scale(28), scale(28));
    m_closeButton->setIcons(QIcon(":/resources/close.png"), QIcon(":/resources/closeHover.png"));
    m_closeButton->setSize(scale(28), scale(28));
    m_closeButton->setCursor(Qt::PointingHandCursor);

    auto* contentPanel = new QWidget(m_container);
    contentPanel->setObjectName("deviceSettingsContentPanel");
    auto* contentInner = new QVBoxLayout(contentPanel);
    contentInner->setContentsMargins(scale(8), scale(20), scale(20), scale(28));
    contentInner->setSpacing(0);
    contentInner->addWidget(m_stack, 1);

    auto* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(0);
    headerLayout->addStretch();
    headerLayout->addWidget(m_closeButton, 0, Qt::AlignTop);

    auto* rightColumn = new QWidget(m_container);
    rightColumn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* rightColumnLayout = new QVBoxLayout(rightColumn);
    rightColumnLayout->setContentsMargins(0, scale(16), scale(18), scale(14));
    rightColumnLayout->setSpacing(scale(18));
    rightColumnLayout->addLayout(headerLayout);
    rightColumnLayout->addWidget(contentPanel, 1);

    auto* mainRow = new QHBoxLayout();
    mainRow->setContentsMargins(0, 0, 0, 0);
    mainRow->setSpacing(scale(12));
    mainRow->addWidget(navColumn, 0);
    mainRow->addWidget(rightColumn, 1);

    auto* containerLayout = new QVBoxLayout(m_container);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);
    containerLayout->addLayout(mainRow);

    m_container->setMinimumWidth(scale(720));

    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(m_container);

    applyStyle();

    connect(m_navButtonGroup, &QButtonGroup::buttonClicked, this, [this](QAbstractButton* btn) {
        if (!btn) return;
        const int idx = btn->property("stackIndex").toInt();
        if (idx >= 0 && idx < m_stack->count()) {
            m_stack->setCurrentIndex(idx);
        }
        updateRowSelectionStyle(m_navButtonGroup);
    });

    connect(m_inputButtonsGroup, &QButtonGroup::buttonClicked, this, [this](QAbstractButton* btn) {
        if (m_blockDeviceSignals || !btn) return;
        emit inputDeviceSelected(btn->property("deviceIndex").toInt());
        updateRowSelectionStyle(m_inputButtonsGroup);
    });
    connect(m_outputButtonsGroup, &QButtonGroup::buttonClicked, this, [this](QAbstractButton* btn) {
        if (m_blockDeviceSignals || !btn) return;
        emit outputDeviceSelected(btn->property("deviceIndex").toInt());
        updateRowSelectionStyle(m_outputButtonsGroup);
    });
    connect(m_cameraButtonsGroup, &QButtonGroup::buttonClicked, this, [this](QAbstractButton* btn) {
        if (m_blockDeviceSignals || !btn) return;
        emit cameraDeviceSelected(btn->property("cameraDeviceId").toString());
        updateRowSelectionStyle(m_cameraButtonsGroup);
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

    updateRowSelectionStyle(m_navButtonGroup);
}

void DeviceSettingsDialog::applyStyle()
{
    setStyleSheet(
        QString("QDialog { background: transparent; }"
        "QLabel { font-size: %1px; }").arg(scale(14)) +
        containerStyle(scale(26), 0, scale(10), 0) +
        panelsChromeStyle(scale(26), scale(16)) +
        sliderStyle()
    );
}

QString DeviceSettingsDialog::sliderStyle() const
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

QString DeviceSettingsDialog::scrollAreaStyle() const
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

void DeviceSettingsDialog::refreshDevices(int currentInputIndex, int currentOutputIndex)
{
    auto inputs = core::media::AudioEngine::getInputDevices();
    auto outputs = core::media::AudioEngine::getOutputDevices();

    if (currentInputIndex < 0) {
        currentInputIndex = core::media::AudioEngine::getDefaultInputDeviceIndex();
    }
    if (currentOutputIndex < 0) {
        currentOutputIndex = core::media::AudioEngine::getDefaultOutputDeviceIndex();
    }

    buildAudioDeviceList(m_inputDevicesLayout, m_inputButtonsGroup, m_inputDevicesArea, inputs, currentInputIndex, true);
    buildAudioDeviceList(m_outputDevicesLayout, m_outputButtonsGroup, m_outputDevicesArea, outputs, currentOutputIndex, false);

    if (m_inputButtonsGroup) {
        for (auto* btn : m_inputButtonsGroup->buttons()) {
            if (btn->property("deviceIndex").toInt() == currentInputIndex) {
                btn->setChecked(true);
            }
        }
    }
    if (m_outputButtonsGroup) {
        for (auto* btn : m_outputButtonsGroup->buttons()) {
            if (btn->property("deviceIndex").toInt() == currentOutputIndex) {
                btn->setChecked(true);
            }
        }
    }
    updateRowSelectionStyle(m_inputButtonsGroup);
    updateRowSelectionStyle(m_outputButtonsGroup);
    updateDeviceListElision();
}

void DeviceSettingsDialog::refreshCameraDevices(const std::vector<core::media::Camera>& cameras, const QString& selectedDeviceId)
{
    buildCameraDeviceList(cameras, selectedDeviceId);
}

void DeviceSettingsDialog::setInputVolume(int volume)
{
    m_micSlider->setValue(volume);
}

void DeviceSettingsDialog::setOutputVolume(int volume)
{
    m_speakerSlider->setValue(volume);
}

void DeviceSettingsDialog::setMicrophoneMuted(bool muted)
{
    if (m_micToggle->isToggled() != muted) {
        m_micToggle->setToggled(muted);
    }
    m_micSlider->setEnabled(!muted);
}

void DeviceSettingsDialog::setSpeakerMuted(bool muted)
{
    if (m_speakerToggle->isToggled() != muted) {
        m_speakerToggle->setToggled(muted);
    }
    m_speakerSlider->setEnabled(!muted);
}

void DeviceSettingsDialog::setSlidersVisible(bool visible)
{
    m_micToggle->setVisible(visible);
    m_micSlider->setVisible(visible);
    m_speakerToggle->setVisible(visible);
    m_speakerSlider->setVisible(visible);
}

void DeviceSettingsDialog::clearLayout(QLayout* layout)
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

void DeviceSettingsDialog::updateRowSelectionStyle(QButtonGroup* group)
{
    if (!group) return;
    const bool isNav = (group == m_navButtonGroup);
    const int listRowRadius = scale(6);
    for (auto* btn : group->buttons()) {
        auto logoPtr = btn->property("logoLabel").value<quintptr>();
        QLabel* logo = logoPtr ? reinterpret_cast<QLabel*>(logoPtr) : nullptr;
        QWidget* row = reinterpret_cast<QWidget*>(btn->property("rowWidget").value<quintptr>());
        const bool checked = btn->isChecked();
        if (logo) {
            logo->setVisible(checked);
        }
        if (row && !isNav) {
            row->setStyleSheet(checked
                ? QString("background-color: %1; border-radius: %2px;").arg(COLOR_OVERLAY_ACCENT_36.name(QColor::HexArgb)).arg(listRowRadius)
                : QString("background-color: transparent; border-radius: %1px;").arg(listRowRadius));
        } else if (row && isNav) {
            const int navR = scale(20);
            row->setAttribute(Qt::WA_StyledBackground, true);
            row->setStyleSheet(checked
                ? QString(
                      "QWidget#deviceSettingsNavItemRow {"
                      " background-color: %1;"
                      " border: none;"
                      " border-radius: %2px;"
                      "}"
                  ).arg(COLOR_OVERLAY_ACCENT_36.name(QColor::HexArgb)).arg(navR)
                : QString(
                      "QWidget#deviceSettingsNavItemRow {"
                      " background-color: transparent;"
                      " border: none;"
                      " border-radius: %1px;"
                      "}"
                  ).arg(navR));
        }
    }
}

void DeviceSettingsDialog::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    updateDeviceListElision();
}

int DeviceSettingsDialog::elideWidthForScrollArea(const QScrollArea* area) const
{
    if (!area) {
        return scale(280);
    }
    const int vw = area->viewport()->width();
    if (vw <= 0) {
        return scale(280);
    }
    const int reserve = scale(4 + 20 + 8 + 8 + 8 + 6);
    return qMax(scale(120), vw - reserve);
}

void DeviceSettingsDialog::updateDeviceListElision()
{
    auto apply = [this](QButtonGroup* group, QScrollArea* scroll) {
        if (!group || !scroll) {
            return;
        }
        const int maxW = elideWidthForScrollArea(scroll);
        for (auto* btn : group->buttons()) {
            const QString full = btn->property("fullDeviceLabel").toString();
            if (full.isEmpty()) {
                continue;
            }
            QFontMetrics fm(btn->font());
            btn->setText(fm.elidedText(full, Qt::ElideRight, maxW));
            btn->setToolTip(full);
        }
    };
    apply(m_inputButtonsGroup, m_inputDevicesArea);
    apply(m_outputButtonsGroup, m_outputDevicesArea);
    apply(m_cameraButtonsGroup, m_cameraDevicesArea);
}

void DeviceSettingsDialog::buildAudioDeviceList(QVBoxLayout* layout, QButtonGroup* group, QScrollArea* listArea,
    const std::vector<core::media::DeviceInfo>& devices, int currentIndex, bool isInput)
{
    if (!layout || !group || !listArea) return;

    m_blockDeviceSignals = true;
    group->blockSignals(true);
    const auto oldAudioButtons = group->buttons();
    for (auto* btn : oldAudioButtons) {
        group->removeButton(btn);
    }
    clearLayout(layout);
    group->setExclusive(true);

    QPixmap logoPixmap(":/resources/logo.png");
    if (!logoPixmap.isNull()) {
        logoPixmap = logoPixmap.scaled(scale(18), scale(18), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    std::unordered_set<std::string> seenNames;
    for (const auto& device : devices) {
        if (!seenNames.insert(device.name).second) {
            continue;
        }

        QString label = QString::fromStdString(device.name);
        if ((isInput && device.isDefaultInput) || (!isInput && device.isDefaultOutput)) {
            label += " (default)";
        }

        QPushButton* btn = new QPushButton();
        btn->setStyleSheet(deviceButtonStyleSheet());
        const int elideW = elideWidthForScrollArea(listArea);
        QFontMetrics fm(btn->font());
        btn->setText(fm.elidedText(label, Qt::ElideRight, elideW));
        btn->setProperty("fullDeviceLabel", label);
        btn->setToolTip(label);
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setProperty("deviceIndex", device.deviceIndex);
        btn->setMinimumWidth(0);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        QLabel* logo = new QLabel();
        logo->setFixedSize(scale(20), scale(20));
        logo->setPixmap(logoPixmap);
        logo->setVisible(false);
        logo->setAttribute(Qt::WA_TranslucentBackground);
        logo->setStyleSheet("background-color: transparent;");

        QWidget* row = new QWidget();
        row->setMinimumWidth(0);
        row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
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
        } else {
            row->setStyleSheet(QString("background-color: transparent; border-radius: %1px;").arg(scale(6)));
        }
    }

    layout->addStretch();
    group->blockSignals(false);
    m_blockDeviceSignals = false;
    updateRowSelectionStyle(group);
}

void DeviceSettingsDialog::buildCameraDeviceList(const std::vector<core::media::Camera>& cameras, const QString& selectedDeviceId)
{
    if (!m_cameraDevicesLayout || !m_cameraButtonsGroup) return;

    m_blockDeviceSignals = true;
    m_cameraButtonsGroup->blockSignals(true);
    const auto oldCamButtons = m_cameraButtonsGroup->buttons();
    for (auto* btn : oldCamButtons) {
        m_cameraButtonsGroup->removeButton(btn);
    }
    clearLayout(m_cameraDevicesLayout);
    m_cameraButtonsGroup->setExclusive(true);

    QPixmap logoPixmap(":/resources/logo.png");
    if (!logoPixmap.isNull()) {
        logoPixmap = logoPixmap.scaled(scale(18), scale(18), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    QString effectiveSelection = selectedDeviceId;
    if (effectiveSelection.isEmpty() && !cameras.empty()) {
        effectiveSelection = QString::fromStdString(cameras.front().deviceId);
    }

    std::unordered_set<std::string> seenIds;
    for (const auto& cam : cameras) {
        if (!seenIds.insert(cam.deviceId).second) {
            continue;
        }

        const QString id = QString::fromStdString(cam.deviceId);
        QString label = QString::fromStdString(cam.displayName);
        if (label.isEmpty()) {
            label = id;
        }

        QPushButton* btn = new QPushButton();
        btn->setStyleSheet(deviceButtonStyleSheet());
        const int elideW = elideWidthForScrollArea(m_cameraDevicesArea);
        QFontMetrics fm(btn->font());
        btn->setText(fm.elidedText(label, Qt::ElideRight, elideW));
        btn->setProperty("fullDeviceLabel", label);
        btn->setToolTip(label);
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setProperty("cameraDeviceId", id);
        btn->setMinimumWidth(0);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        QLabel* logo = new QLabel();
        logo->setFixedSize(scale(20), scale(20));
        logo->setPixmap(logoPixmap);
        logo->setVisible(false);
        logo->setAttribute(Qt::WA_TranslucentBackground);
        logo->setStyleSheet("background-color: transparent;");

        QWidget* row = new QWidget();
        row->setMinimumWidth(0);
        row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        QHBoxLayout* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(scale(8));
        rowLayout->addWidget(btn, 1);
        rowLayout->addWidget(logo, 0, Qt::AlignRight | Qt::AlignVCenter);
        rowLayout->addSpacing(scale(8));

        btn->setProperty("logoLabel", QVariant::fromValue<quintptr>(reinterpret_cast<quintptr>(logo)));
        btn->setProperty("rowWidget", QVariant::fromValue<quintptr>(reinterpret_cast<quintptr>(row)));

        m_cameraButtonsGroup->addButton(btn);
        m_cameraDevicesLayout->addWidget(row);

        if (id == effectiveSelection) {
            btn->setChecked(true);
            logo->setVisible(true);
            row->setStyleSheet(QString("background-color: %1; border-radius: %2px;").arg(COLOR_OVERLAY_ACCENT_36.name(QColor::HexArgb)).arg(scale(6)));
        } else {
            row->setStyleSheet(QString("background-color: transparent; border-radius: %1px;").arg(scale(6)));
        }
    }

    if (cameras.empty()) {
        auto* empty = new QLabel("No cameras available");
        empty->setStyleSheet(QString("color: %1; padding: %2px;").arg(COLOR_TEXT_SECONDARY.name()).arg(scale(8)));
        m_cameraDevicesLayout->addWidget(empty);
    }

    m_cameraDevicesLayout->addStretch();
    m_cameraButtonsGroup->blockSignals(false);
    m_blockDeviceSignals = false;
    updateRowSelectionStyle(m_cameraButtonsGroup);
    updateDeviceListElision();
}




