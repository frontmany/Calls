#include "updaterEventListener.h"

#include <QMetaObject>
#include <QApplication>

#include "managers/updateManager.h"
#include "managers/updaterNetworkErrorHandler.h"

UpdaterEventListener::UpdaterEventListener(UpdateManager* updateManager, UpdaterNetworkErrorHandler* networkErrorHandler) 
	: m_updateManager(updateManager)
	, m_networkErrorHandler(networkErrorHandler)
{
}

void UpdaterEventListener::onUpdateCheckResult(updater::CheckResult updateCheckResult) {
    QMetaObject::invokeMethod(m_updateManager, "onUpdateCheckResult",
        Qt::QueuedConnection, Q_ARG(updater::CheckResult, updateCheckResult));
}

void UpdaterEventListener::onLoadingProgress(double progress) {
    QMetaObject::invokeMethod(m_updateManager, "onLoadingProgress",
        Qt::QueuedConnection, Q_ARG(double, progress));
}

void UpdaterEventListener::onUpdateLoaded(bool emptyUpdate) {
    QMetaObject::invokeMethod(m_updateManager, "onUpdateLoaded",
        Qt::QueuedConnection, Q_ARG(bool, emptyUpdate));
}

void UpdaterEventListener::onNetworkError() {
    if (m_networkErrorHandler) {
        QMetaObject::invokeMethod(static_cast<QObject*>(m_networkErrorHandler), "onNetworkError", Qt::QueuedConnection);
    }
}

void UpdaterEventListener::onConnected() {
    if (m_networkErrorHandler) {
        QMetaObject::invokeMethod(static_cast<QObject*>(m_networkErrorHandler), "onConnected", Qt::QueuedConnection);
    }
}
