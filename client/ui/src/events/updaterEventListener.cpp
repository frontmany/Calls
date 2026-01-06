#include "updaterEventListener.h"

#include <QMetaObject>
#include <QApplication>

#include "managers/updateManager.h"

UpdaterEventListener::UpdaterEventListener(UpdateManager* updateManager) 
	: m_updateManager(updateManager)
{
}

void UpdaterEventListener::onUpdateCheckResult(updater::UpdateStatus status) {
    QMetaObject::invokeMethod(m_updateManager, "onUpdaterCheckResult",
        Qt::QueuedConnection, Q_ARG(updater::UpdateStatus, status));
}

void UpdaterEventListener::onLoadingProgress(double progress) {
    QMetaObject::invokeMethod(m_updateManager, "onLoadingProgress",
        Qt::QueuedConnection, Q_ARG(double, progress));
}

void UpdaterEventListener::onUpdateLoaded(bool emptyUpdate) {
    QMetaObject::invokeMethod(m_updateManager, "onUpdateLoaded",
        Qt::QueuedConnection, Q_ARG(bool, emptyUpdate));
}

void UpdaterEventListener::onUpdateLoadingFailed() {
    QMetaObject::invokeMethod(m_updateManager, "onUpdateLoadingFailed", Qt::QueuedConnection);
}
