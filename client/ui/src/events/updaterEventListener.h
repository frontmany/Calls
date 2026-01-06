#pragma once 

#include "updater.h"

class UpdateManager;

class UpdaterEventListener : public updater::EventListener {
public:
    UpdaterEventListener(UpdateManager* updateManager);

	virtual void onUpdateCheckResult(updater::UpdateStatus status) override;
	virtual void onLoadingProgress(double progress) override;
	virtual void onUpdateLoaded(bool emptyUpdate) override;
	virtual void onUpdateLoadingFailed() override;

private:
    UpdateManager* m_updateManager;
};