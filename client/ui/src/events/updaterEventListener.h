#pragma once 

#include "updater.h"

class UpdateManager;
class UpdaterNetworkErrorHandler;

class UpdaterEventListener : public updater::EventListener {
public:
    UpdaterEventListener(UpdateManager* updateManager, UpdaterNetworkErrorHandler* networkErrorHandler);

	virtual void onUpdateCheckResult(updater::CheckResult updateCheckResult, const std::string& newVersion = {}) override;
	virtual void onLoadingProgress(double progress) override;
	virtual void onUpdateLoaded(bool emptyUpdate) override;
	virtual void onNetworkError() override;
	virtual void onConnected() override;

private:
    UpdateManager* m_updateManager;
    UpdaterNetworkErrorHandler* m_networkErrorHandler;
};
