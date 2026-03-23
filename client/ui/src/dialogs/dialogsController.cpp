#include "dialogsController.h"
#include "widgets/components/overlayWidget.h"
#include "utilities/utilities.h"
#include "dialogs/screenShareDialog.h"
#include "dialogs/alreadyRunningDialog.h"
#include "dialogs/firstLaunchDialog.h"
#include "dialogs/incomingCallDialog.h"

#include <QGuiApplication>
#include <QVBoxLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QMovie>
#include <QWindow>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QPainter>
#include <QPainterPath>
#include <QCursor>
#include <QFontMetrics>
#include <QPointer>
#include "dialogs/deviceSettingsDialog.h"
#include "dialogs/updatingDialog.h"
#include "dialogs/meetingManagementDialog.h"
#include "dialogs/endMeetingConfirmationDialog.h"
#include "updater.h"
#include <algorithm>

DialogsController::DialogsController(QWidget* parent)
    : QObject(parent)
    , m_parent(parent)
    , m_updatingOverlay(nullptr)
    , m_updatingDialog(nullptr)
    , m_screenShareOverlay(nullptr)
    , m_screenShareDialog(nullptr)
    , m_alreadyRunningOverlay(nullptr)
    , m_alreadyRunningDialog(nullptr)
    , m_firstLaunchOverlay(nullptr)
    , m_firstLaunchDialog(nullptr)
    , m_deviceSettingsOverlay(nullptr)
    , m_deviceSettingsDialog(nullptr)
    , m_meetingsManagementOverlay(nullptr)
    , m_meetingsManagementDialog(nullptr)
    , m_endMeetingConfirmationOverlay(nullptr)
    , m_endMeetingConfirmationDialog(nullptr)
{
    m_meetingsJoinWaitingTimer.setSingleShot(true);
    connect(&m_meetingsJoinWaitingTimer, &QTimer::timeout, this, [this]() {
        if (!m_meetingsManagementDialog) return;
        m_meetingsManagementDialog->showConnectingState(m_meetingsJoinWaitingRoomId);
    });
}

DialogsController::~DialogsController()
{
    hideUpdatingDialog();
    hideScreenShareDialog();
    hideCameraShareDialog();
    hideAlreadyRunningDialog();
    hideFirstLaunchDialog();
    hideDeviceSettingsDialog();
    hideMeetingsManagementDialog();
    hideEndMeetingConfirmationDialog();

    for (IncomingCallDialog* dialog : m_incomingCallDialogs)
    {
        if (!dialog)
        {
            continue;
        }

        dialog->hide();
        dialog->deleteLater();
    }

    m_incomingCallDialogs.clear();
}


void DialogsController::showUpdatingDialog()
{
	if (m_updatingDialog)
	{
		return;
	}

	m_updatingOverlay = new OverlayWidget(m_parent);
	m_updatingOverlay->setAttribute(Qt::WA_TranslucentBackground);
    m_updatingOverlay->show();
    m_updatingOverlay->raise();

    m_updatingDialog = new UpdatingDialog(m_updatingOverlay);

    auto centerUpdating = [this]()
    {
        if (!m_updatingDialog || !m_updatingOverlay)
            return;
        m_updatingDialog->adjustSize();
        QSize dialogSize = m_updatingDialog->size();
        QRect overlayRect = m_updatingOverlay->rect();
        int x = overlayRect.center().x() - dialogSize.width() / 2;
        int y = overlayRect.center().y() - dialogSize.height() / 2;
        m_updatingDialog->move(x, y);
        m_updatingDialog->raise();
    };
    centerUpdating();
    m_updatingDialog->show();
    QTimer::singleShot(0, this, centerUpdating);
    QObject::connect(m_updatingOverlay, &OverlayWidget::geometryChanged, this, centerUpdating);
    connect(m_updatingDialog, &UpdatingDialog::closeRequested, this, &DialogsController::closeRequested);
}

void DialogsController::hideUpdatingDialog()
{
	if (m_updatingDialog)
	{
        m_updatingDialog->disconnect();
        m_updatingDialog->hide();
        m_updatingDialog->deleteLater();
		m_updatingDialog = nullptr;
	}

	if (m_updatingOverlay)
	{
		m_updatingOverlay->close();
		m_updatingOverlay->deleteLater();
		m_updatingOverlay = nullptr;
	}
}

void DialogsController::showScreenShareDialog(const QList<QScreen*>& screens)
{
    if (m_screenShareDialog)
    {
        m_screenShareDialog->raise();
        m_screenShareDialog->setScreens(screens);
        return;
    }

    m_screenShareOverlay = new OverlayWidget(m_parent);
    m_screenShareOverlay->setStyleSheet("background-color: transparent;");
    m_screenShareOverlay->setAttribute(Qt::WA_TranslucentBackground);
    m_screenShareOverlay->show();
    m_screenShareOverlay->raise();

    m_screenShareDialog = new ScreenShareDialog(m_screenShareOverlay);
    m_screenShareDialog->setScreens(screens);

    auto centerDialog = [this]()
    {
        if (!m_screenShareDialog || !m_screenShareOverlay)
        {
            return;
        }

        m_screenShareDialog->adjustSize();
        const QSize dialogSize = m_screenShareDialog->size();

        QWidget* referenceWidget = m_screenShareOverlay->window();
        if (!referenceWidget && m_parent)
        {
            referenceWidget = m_parent->window();
        }

        QRect targetRect;
        if (referenceWidget)
        {
            const QRect windowRect = referenceWidget->frameGeometry();
            const QPoint windowCenter = windowRect.center();

            QScreen* screen = QGuiApplication::screenAt(windowCenter);
            if (!screen && referenceWidget->windowHandle())
            {
                screen = referenceWidget->windowHandle()->screen();
            }
            if (!screen)
            {
                screen = QGuiApplication::primaryScreen();
            }
            if (screen)
            {
                targetRect = screen->availableGeometry();
            }
        }

        if (!targetRect.isValid())
        {
            if (QScreen* primaryScreen = QGuiApplication::primaryScreen())
            {
                targetRect = primaryScreen->availableGeometry();
            }
            else if (referenceWidget)
            {
                targetRect = referenceWidget->frameGeometry();
            }
            else
            {
                const QPoint overlayTopLeft = m_screenShareOverlay->mapToGlobal(QPoint(0, 0));
                targetRect = QRect(overlayTopLeft, m_screenShareOverlay->size());
            }
        }

        const QPoint desiredGlobal(
            targetRect.x() + (targetRect.width() - dialogSize.width()) / 2,
            targetRect.y() + (targetRect.height() - dialogSize.height()) / 2);

        QPoint localPos = m_screenShareOverlay->mapFromGlobal(desiredGlobal);
        const QRect overlayRect = m_screenShareOverlay->rect();
        const QRect desiredLocalRect(localPos, dialogSize);

        if (!overlayRect.contains(desiredLocalRect))
        {
            const int maxX = std::max(0, overlayRect.width() - dialogSize.width());
            const int maxY = std::max(0, overlayRect.height() - dialogSize.height());

            const int boundedX = std::clamp(localPos.x(), 0, maxX);
            const int boundedY = std::clamp(localPos.y(), 0, maxY);

            localPos = QPoint(boundedX, boundedY);

            if (!overlayRect.contains(QRect(localPos, dialogSize)))
            {
                const QPoint overlayCenter = overlayRect.center();
                localPos.setX(std::clamp(overlayCenter.x() - dialogSize.width() / 2, 0, maxX));
                localPos.setY(std::clamp(overlayCenter.y() - dialogSize.height() / 2, 0, maxY));
            }
        }

        m_screenShareDialog->move(localPos);
        m_screenShareDialog->raise();
    };

    centerDialog();
    m_screenShareDialog->show();
    QTimer::singleShot(0, this, centerDialog);
    QObject::connect(m_screenShareOverlay, &OverlayWidget::geometryChanged, this, centerDialog);

    connect(m_screenShareDialog, &ScreenShareDialog::screenSelected, this, [this](int index)
    {
        emit screenSelected(index);
        hideScreenShareDialog();
    });
    connect(m_screenShareDialog, &ScreenShareDialog::cancelled, this, [this]()
    {
        emit screenShareDialogCancelled();
        hideScreenShareDialog();
    });
}

void DialogsController::hideScreenShareDialog()
{
    if (m_screenShareDialog)
    {
        m_screenShareDialog->disconnect();
        m_screenShareDialog->hide();
        m_screenShareDialog->deleteLater();
        m_screenShareDialog = nullptr;
    }

    if (m_screenShareOverlay)
    {
        m_screenShareOverlay->close();
        m_screenShareOverlay->deleteLater();
        m_screenShareOverlay = nullptr;
    }
}

void DialogsController::showCameraShareDialog()
{
    // Camera share dialog is just a notification, not a separate dialog
    // This method can be used to show a camera sharing indicator if needed
}

void DialogsController::hideCameraShareDialog()
{
    // Camera share dialog is just a notification, not a separate dialog
    // This method can be used to hide a camera sharing indicator if needed
}

void DialogsController::setUpdateLoadingProgress(double progress)
{
    if (m_updatingDialog)
    {
        m_updatingDialog->setProgress(progress);
    }
}

void DialogsController::setUpdateDialogStatus(const QString& statusText, bool hideProgress) {
    if (m_updatingDialog)
    {
        m_updatingDialog->setStatus(statusText, hideProgress);
    }
}


void DialogsController::showAlreadyRunningDialog()
{
    if (m_alreadyRunningDialog)
    {
        m_alreadyRunningDialog->raise();
        return;
    }

    m_alreadyRunningOverlay = new OverlayWidget(m_parent);
    m_alreadyRunningOverlay->setAttribute(Qt::WA_TranslucentBackground);
    m_alreadyRunningOverlay->show();
    m_alreadyRunningOverlay->raise();

    m_alreadyRunningDialog = new AlreadyRunningDialog(m_alreadyRunningOverlay);

    auto centerDialog = [this]()
        {
            if (!m_alreadyRunningDialog || !m_alreadyRunningOverlay)
                return;

            m_alreadyRunningDialog->adjustSize();
            QSize dialogSize = m_alreadyRunningDialog->size();
            QRect overlayRect = m_alreadyRunningOverlay->rect();
            int x = overlayRect.center().x() - dialogSize.width() / 2;
            int y = overlayRect.center().y() - dialogSize.height() / 2;
            m_alreadyRunningDialog->move(x, y);
            m_alreadyRunningDialog->raise();
        };

    centerDialog();
    m_alreadyRunningDialog->show();
    QTimer::singleShot(0, this, centerDialog);
    QObject::connect(m_alreadyRunningOverlay, &OverlayWidget::geometryChanged, this, centerDialog);
    connect(m_alreadyRunningDialog, &AlreadyRunningDialog::closeRequested, this, &DialogsController::closeRequested);
}

void DialogsController::hideAlreadyRunningDialog()
{
    if (m_alreadyRunningDialog)
    {
        m_alreadyRunningDialog->disconnect();
        m_alreadyRunningDialog->hide();
        m_alreadyRunningDialog->deleteLater();
        m_alreadyRunningDialog = nullptr;
    }

    if (m_alreadyRunningOverlay)
    {
        m_alreadyRunningOverlay->close();
        m_alreadyRunningOverlay->deleteLater();
        m_alreadyRunningOverlay = nullptr;
    }

}

void DialogsController::showFirstLaunchDialog(const QString& imagePath, const QString& descriptionText)
{
    if (m_firstLaunchDialog)
    {
        return;
    }

    m_firstLaunchOverlay = new OverlayWidget(m_parent);
    m_firstLaunchOverlay->setAttribute(Qt::WA_TranslucentBackground);
    m_firstLaunchOverlay->show();
    m_firstLaunchOverlay->raise();

    m_firstLaunchDialog = new FirstLaunchDialog(m_firstLaunchOverlay, imagePath, descriptionText);

    auto centerDialog = [this]()
        {
            if (!m_firstLaunchDialog || !m_firstLaunchOverlay)
                return;

            m_firstLaunchDialog->adjustSize();
            QSize dialogSize = m_firstLaunchDialog->size();
            QRect overlayRect = m_firstLaunchOverlay->rect();
            int x = overlayRect.center().x() - dialogSize.width() / 2;
            int y = overlayRect.center().y() - dialogSize.height() / 2;
            m_firstLaunchDialog->move(x, y);
            m_firstLaunchDialog->raise();
        };

    centerDialog();
    m_firstLaunchDialog->show();
    QTimer::singleShot(0, this, centerDialog);
    QObject::connect(m_firstLaunchOverlay, &OverlayWidget::geometryChanged, this, centerDialog);
    connect(m_firstLaunchDialog, &FirstLaunchDialog::closeRequested, this, &DialogsController::hideFirstLaunchDialog);
}

void DialogsController::hideFirstLaunchDialog()
{
    if (m_firstLaunchDialog)
    {
        m_firstLaunchDialog->disconnect();
        m_firstLaunchDialog->hide();
        m_firstLaunchDialog->deleteLater();
        m_firstLaunchDialog = nullptr;
    }

    if (m_firstLaunchOverlay)
    {
        m_firstLaunchOverlay->disconnect();
        m_firstLaunchOverlay->close();
        m_firstLaunchOverlay->deleteLater();
        m_firstLaunchOverlay = nullptr;
    }

}

void DialogsController::showDeviceSettingsDialog(bool showSliders, bool micMuted, bool speakerMuted, int inputVolume, int outputVolume,
    int currentInputDevice, int currentOutputDevice,
    const std::vector<core::media::Camera>& cameras, const QString& currentCameraDeviceId)
{
    if (!m_deviceSettingsDialog) {
        m_deviceSettingsDialog = new DeviceSettingsDialog(m_parent);
        connect(m_deviceSettingsDialog, &DeviceSettingsDialog::inputDeviceSelected, this, &DialogsController::inputDeviceSelected);
        connect(m_deviceSettingsDialog, &DeviceSettingsDialog::outputDeviceSelected, this, &DialogsController::outputDeviceSelected);
        connect(m_deviceSettingsDialog, &DeviceSettingsDialog::cameraDeviceSelected, this, &DialogsController::cameraDeviceSelected);
        connect(m_deviceSettingsDialog, &DeviceSettingsDialog::inputVolumeChanged, this, &DialogsController::inputVolumeChanged);
        connect(m_deviceSettingsDialog, &DeviceSettingsDialog::outputVolumeChanged, this, &DialogsController::outputVolumeChanged);
        connect(m_deviceSettingsDialog, &DeviceSettingsDialog::muteMicrophoneClicked, this, &DialogsController::muteMicrophoneClicked);
        connect(m_deviceSettingsDialog, &DeviceSettingsDialog::muteSpeakerClicked, this, &DialogsController::muteSpeakerClicked);
        connect(m_deviceSettingsDialog, &DeviceSettingsDialog::refreshAudioDevicesRequested, this, &DialogsController::refreshAudioDevicesRequested);
        connect(m_deviceSettingsDialog, &DeviceSettingsDialog::closeRequested, this, &DialogsController::hideDeviceSettingsDialog);
    }

    if (m_deviceSettingsDialog) {
        if (!m_deviceSettingsOverlay) {
            m_deviceSettingsOverlay = new OverlayWidget(m_parent);
            m_deviceSettingsOverlay->setAttribute(Qt::WA_TranslucentBackground);
            m_deviceSettingsOverlay->show();
            m_deviceSettingsOverlay->raise();
        }

        emit refreshAudioDevicesRequested();

        auto centerDeviceDialog = [this]() {
            if (!m_deviceSettingsDialog || !m_deviceSettingsOverlay) return;
            m_deviceSettingsDialog->adjustSize();
            QRect overlayRect = m_deviceSettingsOverlay->rect();
            QSize dlgSize = m_deviceSettingsDialog->size();
            int x = overlayRect.center().x() - dlgSize.width() / 2;
            int y = overlayRect.center().y() - dlgSize.height() / 2;
            m_deviceSettingsDialog->move(x, y);
            m_deviceSettingsDialog->raise();
        };

        m_deviceSettingsDialog->setParent(m_deviceSettingsOverlay);
        m_deviceSettingsDialog->setSlidersVisible(showSliders);
        m_deviceSettingsDialog->setMicrophoneMuted(micMuted);
        m_deviceSettingsDialog->setSpeakerMuted(speakerMuted);
        m_deviceSettingsDialog->setInputVolume(inputVolume);
        m_deviceSettingsDialog->setOutputVolume(outputVolume);
        m_deviceSettingsDialog->refreshDevices(currentInputDevice, currentOutputDevice);
        m_deviceSettingsDialog->refreshCameraDevices(cameras, currentCameraDeviceId);
        centerDeviceDialog();

        connect(m_deviceSettingsOverlay, &OverlayWidget::geometryChanged, this, centerDeviceDialog);

        m_deviceSettingsDialog->show();
        m_deviceSettingsDialog->raise();
        m_deviceSettingsOverlay->raise();
    }
}

void DialogsController::refreshDeviceSettingsDialog(int currentInputIndex, int currentOutputIndex,
    const std::vector<core::media::Camera>& cameras, const QString& currentCameraDeviceId)
{
    if (m_deviceSettingsDialog && m_deviceSettingsDialog->isVisible()) {
        m_deviceSettingsDialog->refreshDevices(currentInputIndex, currentOutputIndex);
        m_deviceSettingsDialog->refreshCameraDevices(cameras, currentCameraDeviceId);
    }
}

void DialogsController::hideDeviceSettingsDialog()
{
    if (m_deviceSettingsDialog) {
        m_deviceSettingsDialog->hide();
    }
    if (m_deviceSettingsOverlay) {
        m_deviceSettingsOverlay->close();
        m_deviceSettingsOverlay->deleteLater();
        m_deviceSettingsOverlay = nullptr;
    }

    if (m_deviceSettingsDialog) {
        m_deviceSettingsDialog->setParent(nullptr);
    }
    emit deviceSettingsDialogClosed();
}

void DialogsController::showIncomingCallsDialog(const QString& friendNickname, int remainingTime)
{
    IncomingCallDialog* dialog = m_incomingCallDialogs.value(friendNickname, nullptr);
    if (!dialog)
    {
        dialog = new IncomingCallDialog(nullptr, friendNickname, remainingTime);
        m_incomingCallDialogs.insert(friendNickname, dialog);

        connect(dialog, &IncomingCallDialog::callAccepted, this, &DialogsController::incomingCallAccepted);
        connect(dialog, &IncomingCallDialog::callDeclined, this, &DialogsController::incomingCallDeclined);
        connect(dialog, &IncomingCallDialog::dialogClosed, this, &DialogsController::incomingCallsDialogClosed);
        connect(dialog, &QObject::destroyed, this, [this, friendNickname]() { m_incomingCallDialogs.remove(friendNickname); });
    }
    else
    {
        dialog->setRemainingTime(remainingTime);
    }

    QWidget* ref = m_parent ? m_parent->window() : nullptr;
    QScreen* screen = ref && ref->windowHandle() ? ref->windowHandle()->screen() : QGuiApplication::primaryScreen();
    if (!screen)
    {
        return;
    }

    QRect avail = screen->availableGeometry();

    dialog->adjustSize();
    dialog->show();
    dialog->raise();

    int x = avail.x() + (avail.width() - dialog->width()) / 2;
    int y = avail.y() + (avail.height() - dialog->height()) / 2 + scale(15);
    dialog->move(x, y);
}

void DialogsController::hideIncomingCallsDialog(const QString& friendNickname)
{
    IncomingCallDialog* dialog = m_incomingCallDialogs.take(friendNickname);
    if (!dialog)
    {
        return;
    }

    dialog->hide();
    dialog->deleteLater();
}

void DialogsController::setIncomingCallButtonsActive(const QString& friendNickname, bool active)
{
    if (IncomingCallDialog* dialog = m_incomingCallDialogs.value(friendNickname, nullptr))
    {
        dialog->setButtonsEnabled(active);
    }
}

void DialogsController::showMeetingsManagementDialog()
{
    if (m_meetingsManagementDialog)
    {
        m_meetingsManagementDialog->showInitialState();
        m_meetingsManagementDialog->raise();
        return;
    }

    m_meetingsManagementOverlay = new OverlayWidget(m_parent);
    m_meetingsManagementOverlay->setAttribute(Qt::WA_TranslucentBackground);
    m_meetingsManagementOverlay->show();
    m_meetingsManagementOverlay->raise();

    m_meetingsManagementDialog = new MeetingManagementDialog(m_meetingsManagementOverlay);

    auto centerDialog = [this]()
    {
        if (!m_meetingsManagementDialog || !m_meetingsManagementOverlay)
            return;

        m_meetingsManagementDialog->adjustSize();
        QSize dialogSize = m_meetingsManagementDialog->size();
        QRect overlayRect = m_meetingsManagementOverlay->rect();
        int x = overlayRect.center().x() - dialogSize.width() / 2;
        int y = overlayRect.center().y() - dialogSize.height() / 2;
        m_meetingsManagementDialog->move(x, y);
        m_meetingsManagementDialog->raise();
    };

    centerDialog();
    m_meetingsManagementDialog->show();
    QTimer::singleShot(0, this, centerDialog);
    QTimer::singleShot(0, this, [this]() {
        if (m_meetingsManagementDialog) {
            m_meetingsManagementDialog->focusMeetingIdInput();
        }
    });
    QObject::connect(m_meetingsManagementOverlay, &OverlayWidget::geometryChanged, this, centerDialog);

    connect(m_meetingsManagementDialog, &MeetingManagementDialog::closeRequested, this, &DialogsController::hideMeetingsManagementDialog);
    connect(m_meetingsManagementDialog, &MeetingManagementDialog::createMeetingRequested, this, [this]() {
        emit meetingCreateRequested();
    });
    connect(m_meetingsManagementDialog, &MeetingManagementDialog::joinMeetingRequested, this, [this](const QString& uid) {
        emit meetingJoinRequested(uid);
    });
    connect(m_meetingsManagementDialog, &MeetingManagementDialog::joinCancelled, this, [this]() {
        emit meetingJoinCancelled();
        if (m_meetingsManagementDialog) {
            m_meetingsManagementDialog->showInitialState();
        }
    });
}

void DialogsController::hideMeetingsManagementDialog()
{
    if (m_meetingsManagementDialog)
    {
        // Cancel delayed "waiting for approval" UI update.
        m_meetingsJoinWaitingTimer.stop();
        m_meetingsJoinWaitingRoomId.clear();
        m_meetingsManagementDialog->disconnect();
        m_meetingsManagementDialog->hide();
        m_meetingsManagementDialog->deleteLater();
        m_meetingsManagementDialog = nullptr;
    }

    if (m_meetingsManagementOverlay)
    {
        m_meetingsManagementOverlay->close();
        m_meetingsManagementOverlay->deleteLater();
        m_meetingsManagementOverlay = nullptr;
    }
}

void DialogsController::showMeetingsConnectingState(const QString& roomId)
{
    if (m_meetingsManagementDialog)
    {
        // Avoid micro-flicker: if the join request fails immediately,
        // we don't want to briefly show "waiting for approval".
        constexpr int JOIN_WAITING_SHOW_DELAY_MS = 120;
        m_meetingsJoinWaitingTimer.stop();
        m_meetingsJoinWaitingRoomId = roomId;
        m_meetingsJoinWaitingTimer.start(JOIN_WAITING_SHOW_DELAY_MS);
    }
}

void DialogsController::setMeetingsJoinStatus(const QString& status)
{
    if (m_meetingsManagementDialog)
    {
        m_meetingsManagementDialog->setJoinStatus(status);
    }
}

void DialogsController::resetMeetingsJoinRequestUI()
{
    if (m_meetingsManagementDialog)
    {
        // Cancel delayed "waiting for approval" UI updates.
        m_meetingsJoinWaitingTimer.stop();
        m_meetingsJoinWaitingRoomId.clear();
        m_meetingsManagementDialog->showInitialState();
    }
}

void DialogsController::showEndMeetingConfirmationDialog()
{
    if (m_endMeetingConfirmationDialog)
    {
        m_endMeetingConfirmationDialog->raise();
        return;
    }

    m_endMeetingConfirmationOverlay = new OverlayWidget(m_parent);
    m_endMeetingConfirmationOverlay->setAttribute(Qt::WA_TranslucentBackground);
    m_endMeetingConfirmationOverlay->show();
    m_endMeetingConfirmationOverlay->raise();

    m_endMeetingConfirmationDialog = new EndMeetingConfirmationDialog(m_endMeetingConfirmationOverlay);

    auto centerDialog = [this]()
    {
        if (!m_endMeetingConfirmationDialog || !m_endMeetingConfirmationOverlay)
            return;
        m_endMeetingConfirmationDialog->adjustSize();
        QSize dialogSize = m_endMeetingConfirmationDialog->size();
        QRect overlayRect = m_endMeetingConfirmationOverlay->rect();
        int x = overlayRect.center().x() - dialogSize.width() / 2;
        int y = overlayRect.center().y() - dialogSize.height() / 2;
        m_endMeetingConfirmationDialog->move(x, y);
        m_endMeetingConfirmationDialog->raise();
    };

    centerDialog();
    m_endMeetingConfirmationDialog->show();
    QTimer::singleShot(0, this, centerDialog);
    QObject::connect(m_endMeetingConfirmationOverlay, &OverlayWidget::geometryChanged, this, centerDialog);

    connect(m_endMeetingConfirmationDialog, &EndMeetingConfirmationDialog::endMeetingConfirmed, this, [this]() {
        emit endMeetingConfirmed();
        hideEndMeetingConfirmationDialog();
    });
    connect(m_endMeetingConfirmationDialog, &EndMeetingConfirmationDialog::endMeetingCancelled, this, [this]() {
        emit endMeetingCancelled();
        hideEndMeetingConfirmationDialog();
    });
}

void DialogsController::hideEndMeetingConfirmationDialog()
{
    if (m_endMeetingConfirmationDialog)
    {
        m_endMeetingConfirmationDialog->disconnect();
        m_endMeetingConfirmationDialog->hide();
        m_endMeetingConfirmationDialog->deleteLater();
        m_endMeetingConfirmationDialog = nullptr;
    }

    if (m_endMeetingConfirmationOverlay)
    {
        m_endMeetingConfirmationOverlay->close();
        m_endMeetingConfirmationOverlay->deleteLater();
        m_endMeetingConfirmationOverlay = nullptr;
    }
}

