#include "network/networkController.h"
#include "../utilities/utilities.h"
#include "../packetType.h"
#include "../checkResult.h"
#include "json.hpp"
#include "../jsonType.h"

#include <chrono>

namespace updater
{
	namespace network
	{
		NetworkController::NetworkController(
			std::function<void(CheckResult)>&& onUpdateCheckResult,
			std::function<void(double)>&& onUpdateLoadingProgress,
			std::function<void(bool)>&& onUpdateLoaded,
			std::function<void()>&& onNetworkError,
			std::function<std::vector<FileMetadata>(Packet&)>&& onMetadata,
			std::function<void()>&& onConnected)
			: m_onCheckResult(onUpdateCheckResult),
			m_onLoadingProgress(onUpdateLoadingProgress),
			m_onAllFilesLoaded(onUpdateLoaded),
			m_onNetworkError(onNetworkError),
			m_onMetadata(onMetadata),
			m_onConnected(std::move(onConnected)),
			m_socket(m_context)
		{
		}

		NetworkController::~NetworkController()
		{
			disconnect();
		}

		void NetworkController::sendPacket(const Packet& packet)
		{
			if (m_packetSender) {
				m_packetSender->sendPacket(packet);
			}
		}

		void NetworkController::connect(const std::string& host, const std::string& port)
		{
			if (m_connecting.exchange(true)) {
				return;
			}

			if (m_context.stopped()) {
				m_context.restart();
				m_socket = asio::ip::tcp::socket(m_context);
			}

			m_workGuard = std::make_unique<asio::executor_work_guard<asio::io_context::executor_type>>(
				asio::make_work_guard(m_context));

			if (!m_asioThread.joinable()) {
				m_asioThread = std::thread([this]() { m_context.run(); });
			}

			m_connectionResolver = std::make_unique<ConnectionResolver>(m_context, m_socket);
			m_connectionResolver->connect(host, std::stoi(port),
				[this]() { 
					m_connecting = false;
					initializeAfterHandshake(); 
				},
				[this]() { 
					m_connecting = false;
					m_onNetworkError(); 
				});
		}

		void NetworkController::disconnect()
		{
			m_shuttingDown = true;

			if (m_socket.is_open()) {
				std::error_code ec;
				m_socket.cancel(ec);
				m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
				m_socket.close(ec);
			}

			if (m_packetQueueThread.joinable()) {
				m_packetQueueThread.join();
			}

			reset(true);
		}

		bool NetworkController::isConnected() const
		{
			return m_socket.is_open() && !m_shuttingDown;
		}

		void NetworkController::processPacketQueue()
		{
			while (!m_shuttingDown) {
				auto packet = m_packetQueue.pop_for(std::chrono::milliseconds(100));
				if (packet.has_value()) {
					Packet p = std::move(packet.value());
					
					auto type = static_cast<PacketType>(p.type());

					if (type == PacketType::UPDATE_RESULT) {
						handleCheckResultPacket(std::move(p));
					}
					else if (type == PacketType::UPDATE_METADATA) {
						handleMetadataPacket(std::move(p));
					}
					else {
						m_onNetworkError();
					}
				}
			}
		}

		void NetworkController::handleCheckResultPacket(Packet&& packet)
		{
			nlohmann::json jsonObject = nlohmann::json::parse(packet.data());
			CheckResult result = static_cast<CheckResult>(jsonObject[UPDATE_CHECK_RESULT].get<int>());
			m_onCheckResult(result);
		}

		void NetworkController::handleMetadataPacket(Packet&& packet)
		{
			std::vector<FileMetadata> files = m_onMetadata(packet);
			m_updateSession = UpdateSession(files);

			if (m_updateSession->isEmpty()) {
				m_onAllFilesLoaded(true);
				if (m_packetReceiver) {
					m_packetReceiver->startReceiving();
				}
				return;
			}

			if (m_fileReceiver) {
				m_fileReceiver->startReceivingFile(m_updateSession->nextFile());
				m_fileReceiver->receiveChunk();
			}
		}

		void NetworkController::handleFileDownloaded()
		{
			if (!m_updateSession.has_value() || m_updateSession->isEmpty() || !m_fileReceiver) {
				return;
			}

			const FileMetadata& completedFile = m_fileReceiver->getCurrentFile();
			if (completedFile.fileHash != updater::utilities::calculateFileHash(completedFile.relativeFilePath)) {
				m_onNetworkError();
				return;
			}

			m_updateSession->onFileCompleted();

			if (!m_updateSession->hasRemainingFiles()) {
				reset(false);
				m_onLoadingProgress(100.0);
				m_onAllFilesLoaded(false);

				if (m_packetReceiver) {
					m_packetReceiver->startReceiving();
				}
			}
			else {
				m_fileReceiver->startReceivingFile(m_updateSession->nextFile());
				m_fileReceiver->receiveChunk();
			}
		}


		void NetworkController::reset(bool stopContext)
		{
			if (stopContext) {
				m_context.stop();
				if (m_asioThread.joinable()) {
					m_asioThread.join();
				}
			}

			m_updateSession.reset();

			m_packetQueue.clear();

			m_shuttingDown = false;
			m_connecting = false;
		}

		void NetworkController::initializeAfterHandshake()
		{
			m_packetSender = std::make_unique<PacketSender>(m_socket,
				[this]() { 
					m_onNetworkError(); 
				});

			m_fileReceiver = std::make_unique<FileReceiver>(m_socket,
				[this](double fileProgress) {
					if (m_updateSession.has_value()) {
						double totalProgress = m_updateSession->getTotalProgress(fileProgress);
						m_onLoadingProgress(totalProgress);
					}
				},
				[this]() { 
					m_onNetworkError(); 
				},
				[this]() {
					handleFileDownloaded(); 
				});

			m_packetReceiver = std::make_unique<PacketReceiver>(m_socket,
				[this](Packet&& packet) {
					m_packetQueue.push(std::move(packet));
				},
				[this]() {
					m_onNetworkError();
				},
				[this](uint32_t packetType) {
					return static_cast<PacketType>(packetType) != PacketType::UPDATE_METADATA;
				});

			if (!m_packetQueueThread.joinable()) {
				m_packetQueueThread = std::thread([this]() { processPacketQueue(); });
			}

			m_packetReceiver->startReceiving();

			m_onConnected();
		}
	}
}
