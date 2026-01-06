#include "network/updateSession.h"

namespace updater
{
	namespace network
	{
		UpdateSession::UpdateSession(const std::vector<FileMetadata>& files)
		{
			for (const auto& file : files) {
				m_totalBytes += file.fileSize;
				m_expectedFiles.push(file);
			}
		}

		void UpdateSession::onFileCompleted()
		{
			if (!m_expectedFiles.empty()) {
				m_completedBytes += m_expectedFiles.front().fileSize;
				m_expectedFiles.pop();
			}
		}

		bool UpdateSession::hasRemainingFiles() const
		{
			return !m_expectedFiles.empty();
		}

		bool UpdateSession::isEmpty() const
		{
			return m_expectedFiles.empty();
		}

		const FileMetadata& UpdateSession::nextFile() const
		{
			return m_expectedFiles.front();
		}

		double UpdateSession::getTotalProgress(double currentFileProgress) const
		{
			if (m_totalBytes == 0) {
				return 0.0;
			}

			uint64_t currentFileCompletedBytes = 0;
			if (!m_expectedFiles.empty()) {
				currentFileCompletedBytes = static_cast<uint64_t>((m_expectedFiles.front().fileSize * currentFileProgress) / 100.0);
			}

			uint64_t totalCompletedBytes = m_completedBytes + currentFileCompletedBytes;

			double totalProgress = (static_cast<double>(totalCompletedBytes) * 100.0) / static_cast<double>(m_totalBytes);
			totalProgress = std::max(0.0, std::min(100.0, totalProgress));
			totalProgress = std::round(totalProgress * 100.0) / 100.0;

			return totalProgress;
		}
	}
}
