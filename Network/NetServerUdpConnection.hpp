#pragma once
#include "includes.hpp"
#define SERVER_CONNECTION
#include "NetPacket.hpp"
#include "NetThreadSafeQueue.hpp"
#include "MessageTypes.hpp"
#include <atomic>

namespace net
{
	template<typename T>
	class ServerInterface;

	template<typename T>
	class ServerUdpConnection : public std::enable_shared_from_this<ServerUdpConnection<T>>
	{
	public:
		ServerUdpConnection(asio::io_context& asioContext, asio::ip::tcp::endpoint endpoint, uint16_t port, tsQueue<ownedUdpPacket<T>>& qPacketsIn);
		virtual ~ServerUdpConnection();

		void StartListen(uint32_t uid);
		const bool IsConnected() const;
		const uint16_t GetPort() const { return port; };
		const uint32_t GetID() const { return ID; };
		asio::ip::udp::socket& GetSocket() { return m_Socket;}
		bool ReadyToDestroy() { return m_bAllAsyncReturned; }

		void Send(const packet<T>& pkt);

	private:
		void ReadHeader();	//async
		void ReadBody();	//async
		void WriteHeader();	//async
		void WriteBody();	//async
		void AddToIncomingPacketQueue();

	protected:
		asio::io_context& m_AsioContext;
		asio::ip::udp::socket m_Socket;

		tsQueue<packet<T>> m_QueuePacketsOut;
		tsQueue<ownedUdpPacket<T>>& m_QueuePacketsIn;
		packet<T> m_TempPktIn;

		asio::ip::udp::endpoint m_RemoteEndpoint;

		std::atomic_bool m_bAllAsyncReturned = false;
		uint32_t ID = 0;
		uint16_t port = 0;
	};
}

