#pragma once

#include <queue>
#include <vector>
#include <cstdint>

#include "fileReceiver.h"

namespace updater
{
	namespace network
	{
		class UpdateSession
		{
		public:
			UpdateSession(const std::vector<FileMetadata>& files);

			void onFileCompleted();

			bool hasRemainingFiles() const;
			bool isEmpty() const;
			const FileMetadata& nextFile() const;

			double getTotalProgress(double currentFileProgress) const;

		private:
			std::queue<FileMetadata> m_expectedFiles;
			uint64_t m_totalBytes = 0;
			uint64_t m_completedBytes = 0;
		};
	}
}
