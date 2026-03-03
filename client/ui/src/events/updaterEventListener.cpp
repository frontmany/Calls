#include "updaterEventListener.h"

#include <QMetaObject>
#include <QApplication>

#include "logic/updateManager.h"
#include "logic/updaterNetworkErrorHandler.h"

UpdaterEventListener::UpdaterEventListener(UpdateManager* updateManager, UpdaterNetworkErrorHandler* networkErrorHandler) 
	: m_updateManager(updateManager)
	, m_networkErrorHandler(networkErrorHandler)
{
}

void UpdaterEventListener::onUpdateCheckResult(updater::CheckResult updateCheckResult, const std::string& newVersion) {
    QMetaObject::invokeMethod(m_updateManager, "onUpdateCheckResult",
        Qt::QueuedConnection, Q_ARG(updater::CheckResult, updateCheckResult), Q_ARG(QString, QString::fromStdString(newVersion)));
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

void UpdaterEventListener::onManifestProgress(std::size_t filesProcessed, std::size_t totalFiles, const std::string& currentFilePath) {
    QMetaObject::invokeMethod(m_updateManager, "onManifestProgress",
        Qt::QueuedConnection,
        Q_ARG(qulonglong, static_cast<qulonglong>(filesProcessed)),
        Q_ARG(qulonglong, static_cast<qulonglong>(totalFiles)),
        Q_ARG(QString, QString::fromStdString(currentFilePath)));
}
