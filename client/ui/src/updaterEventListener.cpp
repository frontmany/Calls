#include "updaterEventListener.h"

#include <QMetaObject>
#include <QApplication>

#include "updateManager.h"

UpdaterEventListener::UpdaterEventListener(UpdateManager* updateManager) 
	: m_updateManager(updateManager)
{
}

void UpdaterEventListener::onUpdateCheckResult(callifornia::updater::UpdateCheckResult updateCheckResult) {
    QMetaObject::invokeMethod(m_updateManager, "onUpdaterCheckResult",
        Qt::QueuedConnection, Q_ARG(callifornia::updater::UpdateCheckResult, updateCheckResult));
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
    QMetaObject::invokeMethod(m_updateManager, "onUpdaterNetworkError", Qt::QueuedConnection);
}
