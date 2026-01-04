#pragma once

#include "updateCheckResult.h"

namespace updater
{
	class EventListener {
	public:
		virtual void onUpdateCheckResult(UpdateCheckResult updateCheckResult) = 0;
		virtual void onLoadingProgress(double progress) = 0;
		virtual void onUpdateLoaded(bool emptyUpdate) = 0;
		virtual void onNetworkError() = 0;
	};
}
