#pragma once

#include "updatesCheckResult.h"

namespace updater {
	class CallbacksInterface {
	public:
		virtual void onUpdatesCheckResult(UpdatesCheckResult updatesCheckResult) = 0;
		virtual void onLoadingProgress(double progress) = 0;
		virtual void onUpdateLoaded(bool emptyUpdate) = 0;
		virtual void onError() = 0;
	};
}