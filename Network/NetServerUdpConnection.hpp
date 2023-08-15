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
		ServerUdpConnection(uint32_t id);
		virtual ~ServerUdpConnection();

		static const bool IsConnected() { return m_Socket->is_open(); } ;
		const uint32_t GetID() const { return ID; };
		void SetPort(const uint16_t _port) { port = _port; }
		const uint16_t GetPort() const { return port; }
		void SetGroupID(uint32_t id) { m_GroupId = id; };
		const uint32_t GetGroupID() const { return m_GroupId; };

		static void StartListen();
		static asio::ip::udp::socket& GetSocket() { return *m_Socket;}
		static bool ReadyToDestroy() { return m_bAllAsyncReturned.load(); }

		static void AddRemoteEndpoint(asio::ip::tcp::endpoint endpoint, uint16_t port, uint32_t id, std::shared_ptr<ServerUdpConnection<T>> sptr);
		static void RemoveRemoteEndpoint(uint32_t id);
		static void Send(const udpPacket<T>& pkt, uint32_t id);

	private:
		static void ReadHeader();	//async
		static void ReadBody();		//async
		static void WriteHeader(uint32_t id);	//async
		static void WriteBody(uint32_t id);		//async
		static void AddToIncomingPacketQueue(uint32_t uid);

	public:
		inline static asio::io_context* m_AsioContext;
		inline static asio::ip::udp::socket* m_Socket;
		inline static tsQueue<ownedUdpPacket<T>>* m_QueuePacketsIn;

	protected:

		inline static tsQueue<udpPacket<T>> m_QueuePacketsOut;
		inline static udpPacket<T> m_TempPktIn;

		inline static asio::ip::udp::endpoint m_RemoteEndpoint;
		inline static std::unordered_map<uint32_t, asio::ip::udp::endpoint> m_RemoteEndpointsById;
		inline static std::unordered_map<uint32_t, std::shared_ptr<ServerUdpConnection<T>>> m_UdpClientsByIp;

		inline static std::atomic_bool m_bAllAsyncReturned = false;
		uint32_t ID = 0;
		uint32_t m_GroupId = 0;
		uint16_t port = 0;
	};
}

