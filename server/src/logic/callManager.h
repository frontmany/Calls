#pragma once

#include <unordered_set>
#include <memory>
#include <mutex>
#include <string>
#include "models/call.h"
#include "models/pendingCall.h"
#include "models/user.h"

namespace server::logic 
{
	class CallManager {
	public:
		CallManager() = default;
		~CallManager() = default;

		std::shared_ptr<Call> createCall(UserPtr user1, UserPtr user2);
		void endCall(std::shared_ptr<Call> call);
		void addPendingCall(std::shared_ptr<PendingCall> pendingCall);
		void removePendingCall(std::shared_ptr<PendingCall> pendingCall);
		bool hasPendingCall(std::shared_ptr<PendingCall> pendingCall) const;

	private:
		mutable std::mutex m_mutex;
		std::unordered_set<std::shared_ptr<Call>> m_calls;
		std::unordered_set<std::shared_ptr<PendingCall>> m_pendingCalls;
	};
}
