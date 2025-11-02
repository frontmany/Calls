#pragma once 

#include "updater.h"

class MainWindow;

class UpdaterCallbacksHandler : public updater::CallbacksInterface {
public:
    UpdaterCallbacksHandler(MainWindow* mainWindow);

	virtual void onUpdatesCheckResult(updater::UpdatesCheckResult updatesCheckResult) override;
	virtual void onLoadingProgress(double progress) override;
	virtual void onUpdateLoaded(bool emptyUpdate) override;
	virtual void onError() override;

private:
    MainWindow* m_mainWindow;
};