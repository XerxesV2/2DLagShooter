#pragma once
#include "includes.hpp"
#include "NetPacket.hpp"
#include "NetThreadSafeQueue.hpp"
#include "NetClientConnection.hpp"
#include "NetClientUdpConnection.hpp"
#include "MessageTypes.hpp"

namespace net 
{
	template<typename T>
	class ClientInterface
	{
	public:
		ClientInterface();
		virtual ~ClientInterface();

		bool Connect(const std::string& host, const uint16_t tcp_port);
		bool SetUdp(const uint16_t udp_port);
		void SendTcpPacket(const packet<T>& pkt);
		void SendUdpPacket(const packet<T>& pkt);
		bool IsConnected();
		void Disconnect();
		tsQueue<ownedPacket<T>>& IncomingPackets();
		tsQueue<ownedUdpPacket<T>>& IncomingUdpPackets();

	protected:
		asio::io_context m_AsioContext;	//client owns the context
		std::thread m_ContextThread;

		asio::ip::tcp::socket m_Socket;
		asio::ip::tcp::socket m_UdpSocket;
		std::unique_ptr<ClientConnection<T>> m_Connection;
		std::unique_ptr<ClientUdpConnection<T>> m_UdpConnection;

	private:
		tsQueue<ownedPacket<T>> m_QueuePacketsIn;	//from the server
		tsQueue<ownedUdpPacket<T>> m_QueueUdpPacketsIn;
		asio::ip::tcp::resolver::results_type m_Endponts;
		asio::ip::udp::resolver::results_type m_UdpEndpoints;
	};
//#include "NetClient.cpp"
}
