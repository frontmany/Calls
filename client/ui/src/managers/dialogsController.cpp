#include "dialogsController.h"
#include "widgets/overlayWidget.h"
#include "utilities/utilities.h"
#include "dialogs/connectionDownDialog.h"
#include "dialogs/connectionRestoredDialog.h"
#include "dialogs/pendingOperationDialog.h"
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
#include "dialogs/audioSettingsDialog.h"
#include "dialogs/updatingDialog.h"
#include <algorithm>

DialogsController::DialogsController(QWidget* parent)
    : QObject(parent)
    , m_parent(parent)
    , m_updatingOverlay(nullptr)
    , m_updatingDialog(nullptr)
    , m_connectionDownOverlay(nullptr)
    , m_connectionDownDialog(nullptr)
    , m_connectionRestoredOverlay(nullptr)
    , m_connectionRestoredDialog(nullptr)
    , m_pendingOperationOverlay(nullptr)
    , m_pendingOperationDialog(nullptr)
    , m_screenShareOverlay(nullptr)
    , m_screenShareDialog(nullptr)
    , m_alreadyRunningOverlay(nullptr)
    , m_alreadyRunningDialog(nullptr)
    , m_firstLaunchOverlay(nullptr)
    , m_firstLaunchDialog(nullptr)
    , m_audioSettingsOverlay(nullptr)
    , m_audioSettingsDialog(nullptr)
{
}

DialogsController::~DialogsController()
{
    hideUpdatingDialog();
    hideConnectionDownDialog();
    hideConnectionRestoredDialog();
    hidePendingOperationDialog();
    hideScreenShareDialog();
    hideAlreadyRunningDialog();
    hideFirstLaunchDialog();
    hideAudioSettingsDialog();

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

void DialogsController::swapUpdatingToRestarting()
{
    if (m_updatingDialog)
    {
        m_updatingDialog->swapToRestarting();
    }
}

void DialogsController::swapUpdatingToUpToDate()
{
    if (m_updatingDialog)
    {
        m_updatingDialog->swapToUpToDate();
    }
}

void DialogsController::showNotificationDialogInternal(OverlayWidget*& overlay,
    NotificationDialogBase*& dialog,
    bool createOverlay,
    const std::function<NotificationDialogBase*(QWidget*)>& createDialog,
    const std::function<void(NotificationDialogBase*)>& updateDialog)
{
    QWidget* parentWidget = m_parent;
    if (createOverlay)
    {
        if (!overlay)
        {
            overlay = new OverlayWidget(m_parent);
            overlay->setAttribute(Qt::WA_TranslucentBackground);
        }
        overlay->show();
        overlay->raise();
        parentWidget = overlay;
    }
    else
    {
        if (!overlay)
        {
            overlay = new OverlayWidget(m_parent);
            overlay->setAttribute(Qt::WA_TranslucentBackground);
        }
        overlay->setVisible(false);
    }

    if (!parentWidget && overlay)
    {
        parentWidget = overlay;
    }

    if (!dialog && createDialog)
    {
        dialog = createDialog(parentWidget);
    }

    if (dialog)
    {
        if (updateDialog)
        {
            updateDialog(dialog);
        }
        dialog->setParent(parentWidget);
    }

    OverlayWidget** overlayPtr = &overlay;
    NotificationDialogBase** dialogPtr = &dialog;
    auto positionDialog = [this, createOverlay, overlayPtr, dialogPtr]()
    {
        if (!dialogPtr || !(*dialogPtr))
            return;

        QWidget* referenceWidget = createOverlay ? *overlayPtr : m_parent;
        if (!referenceWidget && overlayPtr && *overlayPtr)
        {
            referenceWidget = *overlayPtr;
        }
        if (!referenceWidget)
            return;

        (*dialogPtr)->adjustSize();
        QSize dialogSize = (*dialogPtr)->size();

        int x = (referenceWidget->width() - dialogSize.width()) / 2;
        int y = 40;

        (*dialogPtr)->move(x, y);
        (*dialogPtr)->raise();
    };

    positionDialog();
    dialog->show();
    QTimer::singleShot(0, this, positionDialog);

    if (overlay)
    {
        QObject::connect(overlay, &OverlayWidget::geometryChanged, this, positionDialog, Qt::UniqueConnection);
    }
}

void DialogsController::hideNotificationDialogInternal(OverlayWidget*& overlay, NotificationDialogBase*& dialog)
{
    if (dialog)
    {
        dialog->disconnect();
        dialog->hide();
        dialog->deleteLater();
        dialog = nullptr;
    }

    if (overlay)
    {
        overlay->close();
        overlay->deleteLater();
        overlay = nullptr;
    }
}

void DialogsController::showConnectionDownDialog()
{
    {
        NotificationDialogBase* baseDialog = m_connectionRestoredDialog;
        hideNotificationDialogInternal(m_connectionRestoredOverlay, baseDialog);
        m_connectionRestoredDialog = static_cast<ConnectionRestoredDialog*>(baseDialog);
    }
    {
        NotificationDialogBase* baseDialog = m_pendingOperationDialog;
        hideNotificationDialogInternal(m_pendingOperationOverlay, baseDialog);
        m_pendingOperationDialog = static_cast<PendingOperationDialog*>(baseDialog);
    }

    NotificationDialogBase* baseDialog = m_connectionDownDialog;
    auto createDialog = [](QWidget* parent) -> NotificationDialogBase*
    {
        return new ConnectionDownDialog(parent);
    };
    showNotificationDialogInternal(m_connectionDownOverlay,
        baseDialog,
        true,
        createDialog,
        nullptr);
    m_connectionDownDialog = static_cast<ConnectionDownDialog*>(baseDialog);
}

void DialogsController::hideConnectionDownDialog()
{
    NotificationDialogBase* baseDialog = m_connectionDownDialog;
    hideNotificationDialogInternal(m_connectionDownOverlay, baseDialog);
    m_connectionDownDialog = static_cast<ConnectionDownDialog*>(baseDialog);
}

void DialogsController::showConnectionRestoredDialog()
{
    {
        NotificationDialogBase* baseDialog = m_connectionDownDialog;
        hideNotificationDialogInternal(m_connectionDownOverlay, baseDialog);
        m_connectionDownDialog = static_cast<ConnectionDownDialog*>(baseDialog);
    }
    {
        NotificationDialogBase* baseDialog = m_pendingOperationDialog;
        hideNotificationDialogInternal(m_pendingOperationOverlay, baseDialog);
        m_pendingOperationDialog = static_cast<PendingOperationDialog*>(baseDialog);
    }

    NotificationDialogBase* baseDialog = m_connectionRestoredDialog;
    auto createDialog = [](QWidget* parent) -> NotificationDialogBase*
    {
        return new ConnectionRestoredDialog(parent);
    };
    showNotificationDialogInternal(m_connectionRestoredOverlay,
        baseDialog,
        false,
        createDialog,
        nullptr);
    m_connectionRestoredDialog = static_cast<ConnectionRestoredDialog*>(baseDialog);
}

void DialogsController::hideConnectionRestoredDialog()
{
    NotificationDialogBase* baseDialog = m_connectionRestoredDialog;
    hideNotificationDialogInternal(m_connectionRestoredOverlay, baseDialog);
    m_connectionRestoredDialog = static_cast<ConnectionRestoredDialog*>(baseDialog);
}

void DialogsController::showPendingOperationDialog(const QString& statusText)
{
    if (m_connectionDownDialog || m_connectionRestoredDialog)
    {
        return;
    }

    NotificationDialogBase* baseDialog = m_pendingOperationDialog;
    auto createDialog = [statusText](QWidget* parent) -> NotificationDialogBase*
    {
        return new PendingOperationDialog(parent, statusText);
    };
    auto updateDialog = [statusText](NotificationDialogBase* dialog)
    {
        dialog->setStatusText(statusText);
    };
    showNotificationDialogInternal(m_pendingOperationOverlay,
        baseDialog,
        false,
        createDialog,
        updateDialog);
    m_pendingOperationDialog = static_cast<PendingOperationDialog*>(baseDialog);
}

void DialogsController::hidePendingOperationDialog()
{
    NotificationDialogBase* baseDialog = m_pendingOperationDialog;
    hideNotificationDialogInternal(m_pendingOperationOverlay, baseDialog);
    m_pendingOperationDialog = static_cast<PendingOperationDialog*>(baseDialog);
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
    connect(m_firstLaunchDialog, &FirstLaunchDialog::closeRequested, this, &DialogsController::closeRequested);
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
        m_firstLaunchOverlay->close();
        m_firstLaunchOverlay->deleteLater();
        m_firstLaunchOverlay = nullptr;
    }

}

void DialogsController::showAudioSettingsDialog(bool showSliders, bool micMuted, bool speakerMuted, int inputVolume, int outputVolume, int currentInputDevice, int currentOutputDevice)
{
    if (!m_audioSettingsDialog) {
        m_audioSettingsDialog = new AudioSettingsDialog(m_parent);
        connect(m_audioSettingsDialog, &AudioSettingsDialog::inputDeviceSelected, this, &DialogsController::inputDeviceSelected);
        connect(m_audioSettingsDialog, &AudioSettingsDialog::outputDeviceSelected, this, &DialogsController::outputDeviceSelected);
        connect(m_audioSettingsDialog, &AudioSettingsDialog::inputVolumeChanged, this, &DialogsController::inputVolumeChanged);
        connect(m_audioSettingsDialog, &AudioSettingsDialog::outputVolumeChanged, this, &DialogsController::outputVolumeChanged);
        connect(m_audioSettingsDialog, &AudioSettingsDialog::muteMicrophoneClicked, this, &DialogsController::muteMicrophoneClicked);
        connect(m_audioSettingsDialog, &AudioSettingsDialog::muteSpeakerClicked, this, &DialogsController::muteSpeakerClicked);
        connect(m_audioSettingsDialog, &AudioSettingsDialog::refreshAudioDevicesRequested, this, &DialogsController::refreshAudioDevicesRequested);
        connect(m_audioSettingsDialog, &AudioSettingsDialog::closeRequested, this, &DialogsController::hideAudioSettingsDialog);
    }

    if (m_audioSettingsDialog) {
        if (!m_audioSettingsOverlay) {
            m_audioSettingsOverlay = new OverlayWidget(m_parent);
            m_audioSettingsOverlay->setAttribute(Qt::WA_TranslucentBackground);
            m_audioSettingsOverlay->show();
            m_audioSettingsOverlay->raise();
        }

        // Refresh device list before displaying the dialog
        emit refreshAudioDevicesRequested();

        auto centerAudioDialog = [this]() {
            if (!m_audioSettingsDialog || !m_audioSettingsOverlay) return;
            m_audioSettingsDialog->adjustSize();
            QRect overlayRect = m_audioSettingsOverlay->rect();
            QSize dlgSize = m_audioSettingsDialog->size();
            int x = overlayRect.center().x() - dlgSize.width() / 2;
            int y = overlayRect.center().y() - dlgSize.height() / 2;
            m_audioSettingsDialog->move(x, y);
            m_audioSettingsDialog->raise();
        };

        m_audioSettingsDialog->setParent(m_audioSettingsOverlay);
        m_audioSettingsDialog->setSlidersVisible(showSliders);
        m_audioSettingsDialog->setMicrophoneMuted(micMuted);
        m_audioSettingsDialog->setSpeakerMuted(speakerMuted);
        m_audioSettingsDialog->setInputVolume(inputVolume);
        m_audioSettingsDialog->setOutputVolume(outputVolume);
        m_audioSettingsDialog->refreshDevices(currentInputDevice, currentOutputDevice);
        centerAudioDialog();

        connect(m_audioSettingsOverlay, &OverlayWidget::geometryChanged, this, centerAudioDialog);

        m_audioSettingsDialog->show();
        m_audioSettingsDialog->raise();
        m_audioSettingsOverlay->raise();
    }
}

void DialogsController::hideAudioSettingsDialog()
{
    if (m_audioSettingsDialog) {
        m_audioSettingsDialog->hide();
    }
    if (m_audioSettingsOverlay) {
        m_audioSettingsOverlay->close();
        m_audioSettingsOverlay->deleteLater();
        m_audioSettingsOverlay = nullptr;
    }

    if (m_audioSettingsDialog) {
        m_audioSettingsDialog->setParent(nullptr);
    }
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