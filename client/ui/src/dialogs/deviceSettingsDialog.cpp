#include "deviceSettingsDialog.h"

#include <tuple>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPainter>
#include <QGraphicsDropShadowEffect>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QEvent>
#include <QCursor>
#include <QScrollBar>
#include <unordered_set>
#include "constants/color.h"

namespace {
    QString containerStyle(int radius, int padTop, int padRight, int padBottom, int borderPx)
    {
        return QString(
            "#deviceSettingsContainer {"
            "   background-color: rgb(248, 250, 252);"
            "   border-radius: %1px;"
            "   border: %2px solid rgb(210, 210, 210);"
            "   padding-top: %3px;"
            "   padding-right: %4px;"
            "   padding-bottom: %5px;"
            "   padding-left: 0px;"
            "}"
        ).arg(radius)
         .arg(borderPx)
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
            " font-family: 'Outfit';"
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
         .arg(scale(10))
         .arg(scale(12))
         .arg(scale(12));
    }

    QString navDeviceButtonStyleSheet()
    {
        return QString(
            "QPushButton {"
            " background-color: transparent;"
            " border: none;"
            " border-radius: %6px;"
            " padding: %3px %4px;"
            " color: %1;"
            " text-align: left;"
            " font-size: %5px;"
            " font-family: 'Outfit';"
            " font-weight: bold;"
            " letter-spacing: 0.2px;"
            " text-transform: uppercase;"
            "}"
            "QPushButton:focus {"
            " outline: none;"
            " border: none;"
            "}"
            "QPushButton:checked {"
            " background-color: %2;"
            " color: %7;"
            " border-radius: %6px;"
            "}"
        ).arg(QString("rgb(130, 130, 130)"))
         .arg(COLOR_OVERLAY_ACCENT_36.name(QColor::HexArgb))
         .arg(scale(10))
         .arg(scale(12))
         .arg(scale(11))
         .arg(scale(20))
         .arg(COLOR_TEXT_MAIN.name());
    }

    QString panelsChromeStyle(int dialogRadius, int contentCornerRadius)
    {
        // Keep nav/content panels consistent with the overall dialog background (Meetings style).
        const QString mainBg = "rgb(248, 250, 252)";
        const QString navDreamBg = "rgb(238, 242, 250)";
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
        ).arg(navDreamBg)
         .arg(dialogRadius)
         .arg(mainBg)
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
            " font-family: 'Outfit';"
        ).arg(COLOR_TEXT_MAIN.name())
         .arg(scale(18))
         .arg(scale(4));
    }
}

DeviceSettingsDialog::DeviceSettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setModal(false);
    setAttribute(Qt::WA_TranslucentBackground);

    m_container = new QWidget(this);
    m_container->setObjectName("deviceSettingsContainer");

    auto* shadowEffect = new QGraphicsDropShadowEffect();
    shadowEffect->setBlurRadius(scale(30));
    shadowEffect->setXOffset(0);
    shadowEffect->setYOffset(0);
    shadowEffect->setColor(COLOR_SHADOW_STRONG_150);
    m_container->setGraphicsEffect(shadowEffect);

    m_deviceCenterLabel = new QLabel("Device Center", m_container);
    m_deviceCenterLabel->setStyleSheet(QString(
        "color: rgb(35, 35, 35);"
        "font-size: %1px;"
        "font-family: 'Outfit';"
        "font-weight: bold;"
        "padding: 0px;"
        "margin: 0px;")
        .arg(scale(14)));

    m_navButtonGroup = new QButtonGroup(this);
    m_navButtonGroup->setExclusive(true);

    const int navIconPx = scale(18);
    const int navIconTextGap = scale(14);
    auto makeNavButton = [this](const QString& text) {
        auto* btn = new QPushButton(text);
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(navDeviceButtonStyleSheet());
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        return btn;
    };

    m_navInputButton = makeNavButton("INPUT");
    m_navOutputButton = makeNavButton("OUTPUT");
    m_navCameraButton = makeNavButton("CAMERA");
    m_navInputButton->setProperty("stackIndex", 0);
    m_navOutputButton->setProperty("stackIndex", 1);
    m_navCameraButton->setProperty("stackIndex", 2);

    auto wrapNavRow = [navIconPx, navIconTextGap](QPushButton* btn, const QString& iconPath) -> QWidget* {
        auto* row = new QWidget();
        row->setObjectName("deviceSettingsNavItemRow");
        row->setAttribute(Qt::WA_StyledBackground, true);
        row->setAutoFillBackground(true);
        // Stable height ensures border-radius is rendered as expected.
        btn->setFixedHeight(scale(40));
        row->setFixedHeight(scale(40));
        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(scale(4), 0, scale(4), 0);
        rowLayout->setSpacing(0);
        if (!iconPath.isEmpty()) {
            QPixmap pm(iconPath);
            if (!pm.isNull()) {
                const QPixmap iconPm = pm.scaled(navIconPx, navIconPx, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                QPixmap padded(navIconPx + navIconTextGap, navIconPx);
                padded.fill(Qt::transparent);
                QPainter p(&padded);
                p.setRenderHint(QPainter::SmoothPixmapTransform, true);
                p.drawPixmap(0, (navIconPx - iconPm.height()) / 2, iconPm);
                p.end();
                btn->setIcon(QIcon(padded));
                btn->setIconSize(padded.size());
            }
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
    navLayout->setContentsMargins(scale(22), scale(20), scale(18), scale(18));
    navLayout->setSpacing(scale(10));
    navLayout->addWidget(m_deviceCenterLabel);
    navLayout->addSpacing(scale(10));
    navLayout->addWidget(m_navInputRow);
    navLayout->addWidget(m_navOutputRow);
    navLayout->addWidget(m_navCameraRow);
    navLayout->addStretch();

    auto* navColumn = new QWidget(m_container);
    navColumn->setObjectName("deviceSettingsNavPanel");
    navColumn->setLayout(navLayout);
    navColumn->setMinimumWidth(scale(180));
    navColumn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    m_stack = new QStackedWidget(m_container);

    auto makeScrollList = [this]() {
        auto* area = new QScrollArea(m_container);
        area->setWidgetResizable(true);
        area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        area->setFrameShape(QFrame::NoFrame);
        area->setStyleSheet(scrollAreaStyle());
        auto* w = new QWidget(area);
        w->setAttribute(Qt::WA_TranslucentBackground);
        w->setStyleSheet("background: transparent; border: none;");
        w->setMinimumWidth(0);
        // Must expand to full viewport width so row highlights are consistent.
        w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        auto* lay = new QVBoxLayout(w);
        // Right padding is used to keep room for the inline "logo"/padding inside each row.
        // With a smaller dialog width we reduce it so device names still have enough space.
        lay->setContentsMargins(0, 0, scale(96), 0);
        lay->setSpacing(scale(10));
        area->setWidget(w);
        area->installEventFilter(this);
        area->viewport()->installEventFilter(this);
        if (area->verticalScrollBar()) {
            area->verticalScrollBar()->installEventFilter(this);
        }
        setScrollBarVisibleOnHover(area, false);
        area->setMinimumHeight(scale(200));
        area->setMaximumWidth(QWIDGETSIZE_MAX);
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
    m_micSlider->setFixedWidth(scale(300));
    m_micSlider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_micSlider->setStyleSheet(sliderStyle());

    auto makeVolumeRow = [this](QWidget* toggle, QSlider* slider) {
        auto* row = new QWidget(m_container);
        row->setAttribute(Qt::WA_StyledBackground, true);
        row->setStyleSheet("background: transparent; border: none;");

        auto* layout = new QHBoxLayout(row);
        // Align with the device list's left padding (button padding-left ~= 12px).
        layout->setContentsMargins(scale(12), 0, 0, 0);
        layout->setSpacing(scale(12));
        layout->addWidget(toggle, 0, Qt::AlignVCenter);
        layout->addWidget(slider, 0, Qt::AlignVCenter);
        layout->addStretch(1);
        return row;
    };

    QWidget* micVolumeRow = makeVolumeRow(m_micToggle, m_micSlider);

    m_inputPage = new QWidget();
    auto* inputPageLayout = new QVBoxLayout(m_inputPage);
    inputPageLayout->setContentsMargins(0, 0, 0, 0);
    inputPageLayout->setSpacing(scale(16));
    QLabel* inputHdr = new QLabel("Microphone", m_inputPage);
    inputHdr->setStyleSheet(sectionTitleStyle());
    inputPageLayout->addWidget(inputHdr);
    inputPageLayout->addWidget(m_inputDevicesArea, 1);
    inputPageLayout->addSpacing(scale(20));
    inputPageLayout->addWidget(micVolumeRow, 0);

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
    m_speakerSlider->setFixedWidth(scale(300));
    m_speakerSlider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_speakerSlider->setStyleSheet(sliderStyle());

    QWidget* speakerVolumeRow = makeVolumeRow(m_speakerToggle, m_speakerSlider);

    m_outputPage = new QWidget();
    auto* outputPageLayout = new QVBoxLayout(m_outputPage);
    outputPageLayout->setContentsMargins(0, 0, 0, 0);
    outputPageLayout->setSpacing(scale(16));
    QLabel* outputHdr = new QLabel("Headphones", m_outputPage);
    outputHdr->setStyleSheet(sectionTitleStyle());
    outputPageLayout->addWidget(outputHdr);
    outputPageLayout->addWidget(m_outputDevicesArea, 1);
    outputPageLayout->addSpacing(scale(16));
    outputPageLayout->addWidget(speakerVolumeRow, 0);

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
    contentInner->setContentsMargins(scale(8), scale(18), scale(18), scale(18));
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
    rightColumnLayout->setContentsMargins(0, scale(18), scale(18), scale(16));
    rightColumnLayout->setSpacing(scale(14));
    rightColumnLayout->addLayout(headerLayout);
    rightColumnLayout->addWidget(contentPanel, 1);

    auto* mainRow = new QHBoxLayout();
    mainRow->setContentsMargins(0, 0, 0, 0);
    mainRow->setSpacing(scale(10));
    mainRow->addWidget(navColumn, 0);
    mainRow->addWidget(rightColumn, 1);

    auto* containerLayout = new QVBoxLayout(m_container);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);
    containerLayout->addLayout(mainRow);

    m_container->setMinimumWidth(scale(640));

    auto* outerLayout = new QVBoxLayout(this);
    const int shadowMargin = scale(24);
    outerLayout->setContentsMargins(shadowMargin, shadowMargin, shadowMargin, shadowMargin);
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

    // Allow auto-sizing (DialogsController calls adjustSize()) to prevent clipping.
    // Height aligned with meetings-style baseline (prevents vertical clipping).
    setMinimumSize(scale(640) + shadowMargin * 2, scale(500) + shadowMargin * 2);
}

void DeviceSettingsDialog::applyStyle()
{
    setStyleSheet(
        QString(
            "QDialog { background: transparent; }"
            "QLabel { font-size: %1px; font-family: 'Outfit'; }")
            .arg(scale(12)) +
        containerStyle(scale(16), 0, scale(10), 0, scale(1)) +
        panelsChromeStyle(scale(16), scale(16)) +
        sliderStyle()
    );
}

QString DeviceSettingsDialog::sliderStyle() const
{
    const int grooveHeight = qMax(6, scale(8));
    const int grooveRadius = qMax(3, grooveHeight / 2);
    const int handleSize = qMax(14, scale(17));
    const int handleRadius = qMax(7, handleSize / 2);
    const int handleMargin = -(handleSize - grooveHeight) / 2;

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
        .arg(QString::number(grooveHeight))
        .arg(QString::number(grooveRadius))
        .arg(QString::number(handleSize))
        .arg(QString::number(handleSize))
        .arg(QString::number(handleRadius))
        .arg(QString::number(grooveRadius))
        .arg(COLOR_NEUTRAL_200.name())
        .arg(COLOR_SLIDER_TRACK.name())
        .arg(COLOR_SLIDER_FILL.name())
        .arg(COLOR_NEUTRAL_180.name())
        .arg(COLOR_SLIDER_HANDLE_NEUTRAL.name())
        .arg(QString::number(handleMargin));
}

QString DeviceSettingsDialog::scrollAreaStyle() const
{
    return QString(
        "QScrollArea {"
        "   border: none;"
        "   background-color: rgb(248, 250, 252);"
        "}"
        "QScrollArea QWidget {"
        "   background-color: transparent;"
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

    // `buildAudioDeviceList()` de-duplicates items by `device.name` and keeps only
    // the first one. If `current*Index` points to a later duplicate, nothing will
    // be marked as selected. Re-select against the same de-duplication rule.
    auto selectIndexForUiList = [](const auto& devices, int desiredIndex, bool isInput) -> int {
        std::unordered_set<std::string> seenNames;
        std::vector<int> uniqueIndices;
        int defaultUniqueIndex = -1;

        for (const auto& d : devices) {
            if (!seenNames.insert(d.name).second) {
                continue;
            }
            uniqueIndices.push_back(d.deviceIndex);
            if (isInput ? d.isDefaultInput : d.isDefaultOutput) {
                // Mark the "default" among the items that will actually be drawn.
                defaultUniqueIndex = d.deviceIndex;
            }
        }

        for (int idx : uniqueIndices) {
            if (idx == desiredIndex) {
                return desiredIndex;
            }
        }

        if (defaultUniqueIndex >= 0) {
            return defaultUniqueIndex;
        }
        return uniqueIndices.empty() ? desiredIndex : uniqueIndices.front();
    };

    currentInputIndex = selectIndexForUiList(inputs, currentInputIndex, true);
    currentOutputIndex = selectIndexForUiList(outputs, currentOutputIndex, false);

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
            // Highlight is applied on the QPushButton itself (:checked),
            // so the row background stays transparent.
            row->setStyleSheet("background-color: transparent; border: none;");
        }
    }
}

void DeviceSettingsDialog::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    updateDeviceListElision();
}

bool DeviceSettingsDialog::eventFilter(QObject* watched, QEvent* event)
{
    QScrollArea* area = hoveredScrollAreaForObject(watched);
    if (area) {
        if (event->type() == QEvent::Enter) {
            setScrollBarVisibleOnHover(area, true);
        } else if (event->type() == QEvent::Leave) {
            const QPoint localPos = area->mapFromGlobal(QCursor::pos());
            const bool cursorInside = area->rect().contains(localPos);
            if (!cursorInside) {
                setScrollBarVisibleOnHover(area, false);
            }
        }
    }
    return QDialog::eventFilter(watched, event);
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
        for (auto* btn : group->buttons()) {
            const QString full = btn->property("fullDeviceLabel").toString();
            if (full.isEmpty()) {
                continue;
            }
            // Prefer actual button geometry to avoid over-truncation on wide layouts.
            int textW = btn->width();
            if (textW > 0) {
                textW -= scale(28); // QPushButton horizontal padding: 14px + 14px
                if (!btn->icon().isNull()) {
                    textW -= btn->iconSize().width() + scale(4);
                }
            }
            if (textW <= 0) {
                textW = elideWidthForScrollArea(scroll);
            }
            textW = qMax(scale(80), textW);
            QFontMetrics fm(btn->font());
            btn->setText(fm.elidedText(full, Qt::ElideRight, textW));
            btn->setToolTip(full);
        }
    };
    apply(m_inputButtonsGroup, m_inputDevicesArea);
    apply(m_outputButtonsGroup, m_outputDevicesArea);
    apply(m_cameraButtonsGroup, m_cameraDevicesArea);
}

QScrollArea* DeviceSettingsDialog::hoveredScrollAreaForObject(QObject* watched) const
{
    if (!watched) {
        return nullptr;
    }

    auto mapArea = [watched](QScrollArea* area) -> QScrollArea* {
        if (!area) {
            return nullptr;
        }
        if (watched == area || watched == area->viewport() || watched == area->verticalScrollBar()) {
            return area;
        }
        return nullptr;
    };

    if (auto* area = mapArea(m_inputDevicesArea)) return area;
    if (auto* area = mapArea(m_outputDevicesArea)) return area;
    if (auto* area = mapArea(m_cameraDevicesArea)) return area;
    return nullptr;
}

void DeviceSettingsDialog::setScrollBarVisibleOnHover(QScrollArea* area, bool visible)
{
    if (!area) {
        return;
    }
    auto* sb = area->verticalScrollBar();
    if (!sb) {
        return;
    }
    const bool hasOverflow = sb->maximum() > 0;
    const bool showHandle = visible && hasOverflow;

    const QString handleColor = showHandle
        ? COLOR_SHADOW_MEDIUM_60.name(QColor::HexArgb)
        : QColor(0, 0, 0, 0).name(QColor::HexArgb);
    const QString hoverColor = showHandle
        ? COLOR_SHADOW_STRONG_80.name(QColor::HexArgb)
        : QColor(0, 0, 0, 0).name(QColor::HexArgb);

    sb->setStyleSheet(QString(
        "QScrollBar:vertical {"
        "   background-color: transparent;"
        "   width: %1px;"
        "   margin: 0px;"
        "   border-radius: %2px;"
        "}"
        "QScrollBar::handle:vertical {"
        "   background-color: %3;"
        "   border-radius: %2px;"
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
    ).arg(scale(6))
     .arg(scale(3))
     .arg(handleColor)
     .arg(hoverColor));
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
        logoPixmap = logoPixmap.scaled(scale(22), scale(22), Qt::KeepAspectRatio, Qt::SmoothTransformation);
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
        row->setAttribute(Qt::WA_StyledBackground, true);
        row->setAutoFillBackground(true);
        row->setMinimumWidth(0);
        row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        QHBoxLayout* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(0);
        rowLayout->addWidget(btn, 1);
        rowLayout->addSpacing(scale(8));
        rowLayout->addWidget(logo, 0, Qt::AlignRight | Qt::AlignVCenter);
        rowLayout->addSpacing(scale(12));

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
        logoPixmap = logoPixmap.scaled(scale(22), scale(22), Qt::KeepAspectRatio, Qt::SmoothTransformation);
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
        row->setAttribute(Qt::WA_StyledBackground, true);
        row->setAutoFillBackground(true);
        row->setMinimumWidth(0);
        row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        QHBoxLayout* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(0);
        rowLayout->addWidget(btn, 1);
        rowLayout->addSpacing(scale(8));
        rowLayout->addWidget(logo, 0, Qt::AlignRight | Qt::AlignVCenter);
        rowLayout->addSpacing(scale(12));

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




