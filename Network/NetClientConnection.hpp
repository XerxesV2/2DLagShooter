#pragma once
#include "includes.hpp"
#include "NetPacket.hpp"
#include "NetThreadSafeQueue.hpp"
#include "MessageTypes.hpp"

namespace net 
{
	template<typename T>
	class ClientConnection : public std::enable_shared_from_this<ClientConnection<T>>	//Szeparalni kene
	{
	public:
		ClientConnection(asio::io_context& asioContext, asio::ip::tcp::socket socket, tsQueue<ownedPacket<T>>& qPacketsIn);
		virtual ~ClientConnection();

		const bool ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints);
		const bool Disconnect();
		const bool IsConnected() const;
		const uint32_t GetID() const { return ID; };

		void Send(const packet<T>& pkt);

	private:
		void ReadHeader();	//async
		void ReadBody();	//async
		void WriteHeader();	//async
		void WriteBody();	//async
		void AddToIncomingPacketQueue();

	protected:
		asio::io_context& m_AsioContext;
		asio::ip::tcp::socket m_Socket;

		tsQueue<packet<T>> m_QueuePacketsOut;
		tsQueue<ownedPacket<T>>& m_QueuePacketsIn;
		packet<T> m_TempPktIn;

		uint32_t ID = 0;
		
	};
}