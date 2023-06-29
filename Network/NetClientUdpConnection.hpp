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
		ClientUdpConnection(asio::io_context& asioContext, asio::ip::udp::endpoint endpoint, uint16_t destPort, tsQueue<ownedUdpPacket<T>>& qPacketsIn);
		virtual ~ClientUdpConnection();

		void SetEndPoint(const asio::ip::udp::resolver::results_type& endpoints);
		void ListenToServer();

		const bool IsConnected() const;
		const uint32_t GetID() const { return ID;};

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
		asio::ip::udp::endpoint m_ReceiverEndpoint;

		uint32_t ID = 0;
	};
}

