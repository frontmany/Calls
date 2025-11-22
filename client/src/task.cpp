#include "task.h"

#include <utility>

using namespace std::chrono_literals;

namespace calls {
Task::Task(std::shared_ptr<NetworkController> networkController, const std::string& uuid, std::vector<unsigned char> packet, PacketType type, std::function<void()>&& onAttemptFailed, std::function<void()>&& onAllAttemptsFailed, int maxAttempts)
		: m_networkController(networkController), m_uuid(uuid), m_packet(std::move(packet)), m_type(type), m_onAllAttemptsFailed(onAllAttemptsFailed), m_onAttemptFailed(onAttemptFailed), m_maxAttempts(maxAttempts)
	{
		send();
	}

	Task::~Task() {
		m_timer.stop();
	}

	void Task::retry() {
		send();
	}

	void Task::setFinished() {
		m_timer.stop();
	}

	void Task::send() {
		m_networkController->send(m_packet, m_type);

		m_timer.start(2s, [this]() {
			m_attempts++;

			if (m_attempts == m_maxAttempts) {
				m_onAllAttemptsFailed();
			}
			else
				m_onAttemptFailed();
		});
	}
}