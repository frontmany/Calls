#pragma once

#include <fstream>
#include <filesystem>
#include <queue>
#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <atomic>

#include "../updateCheckResult.h"
#include "packet.h"
#include "packetReceiver.h"
#include "packetSender.h"
#include "fileReceiver.h"
#include "connectionResolver.h"
#include "updateSession.h"
#include "../utilities/safeQueue.h"

#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <optional>

namespace updater
{
	namespace network
	{
		class NetworkController
		{
		public:
			NetworkController(std::function<void(UpdateCheckResult)>&& onCheckResult,
				std::function<void(double)>&& onLoadingProgress,
				std::function<void(bool)>&& onAllFilesLoaded,
				std::function<void()>&& onConnectError,
				std::function<void()>&& onNetworkError,
				std::function<std::vector<FileMetadata>(Packet&)>&& onMetadata
			);

			~NetworkController();

			void sendPacket(const Packet& packet);
			void connect(const std::string& host, const std::string& port);
			void disconnect();
			bool isConnected() const;

		private:
			void processPacketQueue();
			void handleCheckResultPacket(Packet&& packet);
			void handleMetadataPacket(Packet&& packet);
			void handleFileDownloaded();
			void reset(bool stopContext);
			void initializeAfterHandshake();

		private:
			asio::io_context m_context;
			asio::ip::tcp::socket m_socket;
			std::unique_ptr<asio::executor_work_guard<asio::io_context::executor_type>> m_workGuard;
			std::thread m_asioThread;

			utilities::SafeQueue<Packet> m_packetQueue;
			std::thread m_packetQueueThread;

			std::optional<UpdateSession> m_updateSession;
			std::unique_ptr<ConnectionResolver> m_connectionResolver;
			std::unique_ptr<PacketReceiver> m_packetReceiver;
			std::unique_ptr<PacketSender> m_packetSender;
			std::unique_ptr<FileReceiver> m_fileReceiver;

			std::function<std::vector<FileMetadata>(Packet&)> m_onMetadata;
			std::function<void(UpdateCheckResult)> m_onCheckResult;
			std::function<void(double)> m_onLoadingProgress;
			std::function<void(bool)> m_onAllFilesLoaded;
			std::function<void()> m_onConnectError;
			std::function<void()> m_onNetworkError;

			std::atomic_bool m_shuttingDown = false;
		};
	}
}
