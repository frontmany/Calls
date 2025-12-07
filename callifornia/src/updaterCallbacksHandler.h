#pragma once 

#include "updater.h"

class UpdateManager;

class UpdaterCallbacksHandler : public updater::CallbacksInterface {
public:
    UpdaterCallbacksHandler(UpdateManager* updateManager);

	virtual void onUpdatesCheckResult(updater::UpdatesCheckResult updatesCheckResult) override;
	virtual void onLoadingProgress(double progress) override;
	virtual void onUpdateLoaded(bool emptyUpdate) override;
	virtual void onError() override;

private:
    UpdateManager* m_updateManager;
};