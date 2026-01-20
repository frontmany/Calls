#pragma once

#include "checkResult.h"

namespace updater
{
	class EventListener {
	public:
		virtual void onUpdateCheckResult(CheckResult updateCheckResult) = 0;
		virtual void onLoadingProgress(double progress) = 0;
		virtual void onUpdateLoaded(bool emptyUpdate) = 0;
		virtual void onNetworkError() = 0;
		virtual void onConnected() = 0;
	};
}