#pragma once
#include "includes.hpp"
#define SERVER_CONNECTION
#include "NetPacket.hpp"
#include "NetThreadSafeQueue.hpp"
#include "MessageTypes.hpp"

namespace net
{
	template<typename T>
	class ServerInterface;

	template<typename T>
	class ServerConnection : public std::enable_shared_from_this<ServerConnection<T>>	//Szeparalni kene
	{
	public:
		ServerConnection(asio::io_context& asioContext, asio::ip::tcp::socket socket, tsQueue<ownedPacket<T>>& qPacketsIn);
		virtual ~ServerConnection();

		const bool ConnectToClient(ServerInterface<T>* server = nullptr, const char* const hash = nullptr, uint32_t uid = 0);
		const bool Disconnect();
		const bool IsConnected() const;
		const uint32_t GetID() const { return ID; };
		void SetGroupID(uint32_t id) { m_GroupId = id; };
		const uint32_t GetGroupID() const { return m_GroupId; };
		const asio::ip::tcp::socket& GetSocket() const { return m_Socket; };
		const asio::ip::address GetIP() const { return m_Socket.remote_endpoint().address(); };
		asio::ip::tcp::socket& GetSocket() { return m_Socket; }
		const bool ReadyToDestroy() { return m_bAllAsyncReturned.load(); }

		void Send(const packet<T>& pkt);

	private:
		void ReadHeader();	//async
		void ReadBody();	//async
		void WriteHeader();	//async
		void WriteBody();	//async
		void AddToIncomingPacketQueue();

		void ReadChecksum(ServerInterface<T>* server = nullptr, const char* const hash = nullptr);

	protected:
		asio::io_context& m_AsioContext;
		asio::ip::tcp::socket m_Socket;

		tsQueue<packet<T>> m_QueuePacketsOut;
		tsQueue<ownedPacket<T>>& m_QueuePacketsIn;
		packet<T> m_TempPktIn;

		uint32_t ID = 0;
		uint32_t m_GroupId = 0;

		std::atomic_bool m_bAllAsyncReturned = false;
		packet<T> checksumPkt;	//one static keyword = 2hours
		char m_CheckSumIn[65];

	};
}