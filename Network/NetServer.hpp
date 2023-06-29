#pragma once

#include "includes.hpp"
#define SERVER_CONNECTION
#include "NetPacket.hpp"
#include "NetThreadSafeQueue.hpp"
#include "NetServerConnection.hpp"
#include "NetServerUdpConnection.hpp"
#include "MessageTypes.hpp"

namespace net 
{

	template<typename T>
	class ServerInterface 
	{
	public:
		ServerInterface(uint16_t tcp_port, uint16_t udp_port);
		virtual ~ServerInterface();

		bool Start();
		bool Stop();

		void Update(size_t maxPackets = -1, bool onReceive = false);	//unsigned int -1 = INT_MAX

		void ListenForClientConnection();	//async
		void SendTcpPacket(std::shared_ptr<ServerConnection<T>> client, const packet<T>& pkt);
		void SendTcpPacketToAll(const packet<T>& pkt, std::shared_ptr<ServerConnection<T>> ignore = nullptr);	//flip it
		void SendUdpPacket(std::shared_ptr<ServerUdpConnection<T>> client, const packet<T>& pkt);
		void SendUdpPacketToAll(const packet<T>& pkt, std::shared_ptr<ServerUdpConnection<T>> ignore = nullptr);

	protected:
		virtual bool OnClientConnect(std::shared_ptr<ServerConnection<T>> tcpClient, std::shared_ptr<ServerUdpConnection<T>> udpClient);	//false = refuse the connection
		virtual void OnClientDisconnect(std::shared_ptr<ServerConnection<T>> client);
		virtual void OnTcpPacketReceived(std::shared_ptr<ServerConnection<T>> client, packet<T>& packet);
		virtual void OnUdpPacketReceived(std::shared_ptr<ServerUdpConnection<T>> client, packet<T>& packet);

	public:
		virtual bool OnChecksumMismatch(std::shared_ptr<ServerConnection<T>> client);
		virtual void OnChecksumMatch(std::shared_ptr<ServerConnection<T>> client);

	private:
		void CheckClients();
		void SendUdpPort();

	protected:
		asio::io_context m_AsioContext;	//server owns the context
		std::thread m_ContextThread;

		tsQueue<ownedPacket<T>> m_QueuePacketsIn;
		tsQueue<ownedUdpPacket<T>> m_QueueUdpPacketsIn;
		struct Connection
		{
			std::shared_ptr<ServerConnection<T>> tcp;
			std::shared_ptr<ServerUdpConnection<T>> udp;
		};
		std::deque<Connection> m_DeqConnections;
		std::deque<std::shared_ptr<ServerUdpConnection<T>>> m_DeadUdpConnections;

		asio::ip::tcp::acceptor m_AsioAcceptor;
		uint32_t m_CurrentID = 1000;
		uint16_t m_udp_port;
	private:
		char* m_ClientCheckSum = nullptr;
	};
}