#include "logic/callManager.h"

namespace server::logic
{
	std::shared_ptr<Call> CallManager::createCall(UserPtr user1, UserPtr user2) {
		if (!user1 || !user2) {
			return nullptr;
		}
		std::lock_guard<std::mutex> lock(m_mutex);
		auto call = std::make_shared<Call>(user1, user2);
		m_calls.insert(call);
		return call;
	}

	void CallManager::endCall(std::shared_ptr<Call> call) {
		if (!call) return;
		std::lock_guard<std::mutex> lock(m_mutex);
		m_calls.erase(call);
	}

	void CallManager::addPendingCall(std::shared_ptr<PendingCall> pendingCall) {
		if (!pendingCall) return;
		std::lock_guard<std::mutex> lock(m_mutex);
		m_pendingCalls.insert(pendingCall);
	}

	void CallManager::removePendingCall(std::shared_ptr<PendingCall> pendingCall) {
		if (!pendingCall) return;
		std::lock_guard<std::mutex> lock(m_mutex);
		m_pendingCalls.erase(pendingCall);
	}

	bool CallManager::hasPendingCall(std::shared_ptr<PendingCall> pendingCall) const {
		if (!pendingCall) return false;
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_pendingCalls.find(pendingCall) != m_pendingCalls.end();
	}
}
