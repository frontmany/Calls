#include "updaterCallbacksHandler.h"

#include <QMetaObject>
#include <QApplication>

#include "updateManager.h"

UpdaterCallbacksHandler::UpdaterCallbacksHandler(UpdateManager* updateManager) 
	: m_updateManager(updateManager)
{
}

void UpdaterCallbacksHandler::onUpdatesCheckResult(updater::UpdatesCheckResult updatesCheckResult) {
    QMetaObject::invokeMethod(m_updateManager, "onUpdaterCheckResult",
        Qt::QueuedConnection, Q_ARG(updater::UpdatesCheckResult, updatesCheckResult));
}

void UpdaterCallbacksHandler::onLoadingProgress(double progress) {
    QMetaObject::invokeMethod(m_updateManager, "onLoadingProgress",
        Qt::QueuedConnection, Q_ARG(double, progress));
}

void UpdaterCallbacksHandler::onUpdateLoaded(bool emptyUpdate) {
    QMetaObject::invokeMethod(m_updateManager, "onUpdateLoaded",
        Qt::QueuedConnection, Q_ARG(bool, emptyUpdate));
}

void UpdaterCallbacksHandler::onError() {
    QMetaObject::invokeMethod(m_updateManager, "onUpdaterNetworkError", Qt::QueuedConnection);
}