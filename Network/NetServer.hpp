#pragma once
/* Thx javidx9 */

#include "includes.hpp"
#define SERVER_CONNECTION
#include "NetPacket.hpp"
#include "NetThreadSafeQueue.hpp"
#include "NetServerConnection.hpp"
#include "NetServerUdpConnection.hpp"
#include "MessageTypes.hpp"

namespace net 
{
	enum class ConnectionGroup : uint32_t
	{
		LOBBY = 1u,
		GAME
	};

	template<typename T>
	class ServerInterface 
	{
	public:
		ServerInterface(uint16_t tcp_port);
		virtual ~ServerInterface();

		bool Start();
		bool Stop();

		void Update(size_t maxPackets = -1, bool onReceive = false);	//unsigned int -1 = INT_MAX

		void ListenForClientConnection();	//async
		void SendTcpPacket(std::shared_ptr<ServerConnection<T>> client, const packet<T>& pkt);
		void SendTcpPacketToAll(const uint32_t gid, const packet<T>& pkt, std::shared_ptr<ServerConnection<T>> ignore = nullptr);	//flip it
		void SendUdpPacket(std::shared_ptr<ServerUdpConnection<T>> client, const udpPacket<T>& pkt);
		void SendUdpPacketToAll(const uint32_t gid, const udpPacket<T>& pkt, std::shared_ptr<ServerUdpConnection<T>> ignore = nullptr);

		void RemoveConnection(std::shared_ptr<ServerConnection<T>> client);
		void CreateNewConnectionGroup(uint32_t groupId);
		void AddConnectionToGroup(std::shared_ptr<ServerConnection<T>> tcp, std::shared_ptr<ServerUdpConnection<T>> udp, uint32_t groupId);
		void MoveConnectionToGroup(std::shared_ptr<ServerConnection<T>> tcp, uint32_t groupId);
		void AddFreedUdpPort(const uint16_t port);

	protected:
		virtual bool OnClientConnect(std::shared_ptr<ServerConnection<T>> tcpClient, std::shared_ptr<ServerUdpConnection<T>> udpClient);	//false = refuse the connection
		virtual void OnClientDisconnect(const std::shared_ptr<ServerConnection<T>>& client);
		virtual void OnTcpPacketReceived(std::shared_ptr<ServerConnection<T>> client, packet<T>& packet);
		virtual void OnUdpPacketReceived(std::shared_ptr<ServerUdpConnection<T>> client, udpPacket<T>& packet);

	public:
		virtual bool OnChecksumMismatch(std::shared_ptr<ServerConnection<T>> client);
		virtual void OnChecksumMatch(std::shared_ptr<ServerConnection<T>> client);

	private:
		void SendUdpPort();

	protected:
		asio::io_context m_AsioContext;	//server owns the context
		std::thread m_ContextThread;

		tsQueue<ownedPacket<T>> m_QueuePacketsIn;
		tsQueue<ownedUdpPacket<T>> m_QueueUdpPacketsIn;
		struct Connection
		{
			//Connection(Connection& c) : tcp(c.tcp), udp(c.udp){}

			std::shared_ptr<ServerConnection<T>> tcp = nullptr;
			std::shared_ptr<ServerUdpConnection<T>> udp = nullptr;
		};
		std::unordered_map<uint32_t, std::deque<Connection>> m_MapConnectionGroups;

		//std::deque<Connection> m_DeqConnections;	//big problem the player states are sent to login clients as well TODO: separate
		std::deque<Connection> m_DeadConnections;

		asio::ip::tcp::acceptor m_AsioAcceptor;
		uint32_t m_CurrentID = 1000;
		uint16_t m_client_receive_udp_port = 15001;
		uint16_t m_client_send_udp_port = 15000;
		asio::ip::udp::socket m_UdpSocket;

		net::tsQueue<uint16_t> m_FreeUdpPorts;
	private:
		char* m_ClientChecksum = nullptr;
	};
}