#pragma once 

#include "updater.h"

class UpdateManager;

class UpdaterEventListener : public updater::EventListener {
public:
    UpdaterEventListener(UpdateManager* updateManager);

	virtual void onUpdateCheckResult(updater::UpdateCheckResult updateCheckResult) override;
	virtual void onLoadingProgress(double progress) override;
	virtual void onUpdateLoaded(bool emptyUpdate) override;
	virtual void onNetworkError() override;

private:
    UpdateManager* m_updateManager;
};