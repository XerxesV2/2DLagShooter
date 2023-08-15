#pragma once
/* Thx javidx9 */

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
		bool SetUdp(const uint16_t udp_receive_port, const uint16_t udp_send_port);
		void SendTcpPacket(const packet<T>& pkt);
		void SendUdpPacket(const udpPacket<T>& pkt);
		bool IsConnected();
		void Disconnect();
		tsQueue<ownedPacket<T>>& IncomingPackets();
		tsQueue<std::pair<ownedUdpPacket<T>, double>>& IncomingUdpPackets();

	protected:
		asio::io_context m_AsioContext;	//client owns the context
		std::thread m_ContextThread;

		asio::ip::tcp::socket m_Socket;
		std::unique_ptr<ClientConnection<T>> m_Connection;
		std::unique_ptr<ClientUdpConnection<T>> m_UdpConnection;

	private:
		tsQueue<ownedPacket<T>> m_QueuePacketsIn;	//from the server
		tsQueue<std::pair<ownedUdpPacket<T>, double>> m_QueueUdpPacketsIn;

		asio::ip::tcp::resolver::results_type m_Endponts;
		asio::ip::udp::resolver::results_type m_UdpEndpoints;
	};
//#include "NetClient.cpp"
}
