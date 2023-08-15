#pragma once
#include "includes.hpp"
#include "NetPacket.hpp"
#include "NetThreadSafeQueue.hpp"
#include "MessageTypes.hpp"

namespace net
{
	template<typename T>
	class ClientUdpConnection : public std::enable_shared_from_this<ClientUdpConnection<T>>
	{
	public:
		ClientUdpConnection(asio::io_context& asioContext, asio::ip::udp::endpoint endpoint, const uint16_t udp_receive_port, const uint16_t udp_send_port, tsQueue<std::pair<ownedUdpPacket<T>, double>>& qPacketsIn);
		virtual ~ClientUdpConnection();

		void SetEndPoint(const asio::ip::udp::resolver::results_type& endpoints);
		void ListenToServer();

		const bool IsConnected() const;
		const uint32_t GetID() const { return ID;}

		void Send(const udpPacket<T>& pkt);

	private:
		void ReadHeader();	//async
		void ReadBody();	//async
		void WriteHeader();	//async
		void WriteBody();	//async
		void AddToIncomingPacketQueue();

	protected:
		asio::io_context& m_AsioContext;
		asio::ip::udp::socket m_Socket;

		tsQueue<udpPacket<T>> m_QueuePacketsOut;
		tsQueue<std::pair<ownedUdpPacket<T>, double>>& m_QueuePacketsIn;
		static constexpr double BufferTime = 1.f;
		double m_PrevPacketTime = 0.f;
		udpPacket<T> m_TempPktIn;
		std::unique_ptr<std::thread> th;
		std::atomic_bool quit = false;

		asio::ip::udp::endpoint m_RemoteEndpoint;
		asio::ip::udp::endpoint m_ReceiverEndpoint;

		uint32_t ID = 0;
	};
}

