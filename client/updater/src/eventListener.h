#pragma once

#include "checkResult.h"
#include <cstddef>
#include <string>

namespace updater
{
	class EventListener {
	public:
		virtual void onUpdateCheckResult(CheckResult updateCheckResult, const std::string& newVersion = {}) = 0;
		virtual void onLoadingProgress(double progress) = 0;
		virtual void onUpdateLoaded(bool emptyUpdate) = 0;
		virtual void onNetworkError() = 0;
		virtual void onConnected() = 0;
		/// Called during manifest preparation (scan + hash). May be called from background thread; UI must marshal to main thread.
		virtual void onManifestProgress(std::size_t filesProcessed, std::size_t totalFiles, const std::string& currentFilePath) {}
	};
}