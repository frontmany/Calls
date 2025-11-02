#include "updaterCallbacksHandler.h"

#include <QMetaObject>
#include <QApplication>

#include "mainwindow.h"

UpdaterCallbacksHandler::UpdaterCallbacksHandler(MainWindow* mainWindow) 
	: m_mainWindow(mainWindow)
{
}

void UpdaterCallbacksHandler::onUpdatesCheckResult(updater::UpdatesCheckResult updatesCheckResult) {
    QMetaObject::invokeMethod(m_mainWindow, "onUpdaterCheckResult",
        Qt::QueuedConnection, Q_ARG(updater::UpdatesCheckResult, updatesCheckResult));
}

void UpdaterCallbacksHandler::onLoadingProgress(double progress) {
    QMetaObject::invokeMethod(m_mainWindow, "onLoadingProgress",
        Qt::QueuedConnection, Q_ARG(double, progress));
}

void UpdaterCallbacksHandler::onUpdateLoaded(bool emptyUpdate) {
    QMetaObject::invokeMethod(m_mainWindow, "onUpdateLoaded",
        Qt::QueuedConnection, Q_ARG(bool, emptyUpdate));
}

void UpdaterCallbacksHandler::onError() {
    QMetaObject::invokeMethod(m_mainWindow, "onUpdaterNetworkError", Qt::QueuedConnection);
}