#include "NetServerUdpConnection.hpp"
#include "NetServer.hpp"

namespace net
{
	template<typename T>
	ServerUdpConnection<T>::ServerUdpConnection(asio::io_context& asioContext, asio::ip::tcp::endpoint endpoint, uint16_t port, tsQueue<ownedUdpPacket<T>>& qPacketsIn)
		: m_AsioContext(asioContext), m_Socket(asioContext, asio::ip::udp::endpoint{ asio::ip::make_address(endpoint.address().to_v4().to_string()), port }), m_QueuePacketsIn(qPacketsIn)
	{
		this->port = port;
	}

	template<typename T>
	ServerUdpConnection<T>::~ServerUdpConnection()
	{
	}

	template<typename T>
	void ServerUdpConnection<T>::StartListen(uint32_t uid)
	{
		this->ID = uid;
		ReadHeader();
	}

	template<typename T>
	const bool ServerUdpConnection<T>::IsConnected() const
	{
		return m_Socket.is_open();
	}

	template<typename T>
	void ServerUdpConnection<T>::Send(const packet<T>& pkt)
	{
		/*injecting work*/
		asio::post(m_AsioContext,
			[this, pkt]() {
				bool WritingPacket = !m_QueuePacketsOut.Empty();
				m_QueuePacketsOut.PushBack(pkt);

				if (!WritingPacket) WriteHeader();
			});
	}

	template<typename T>
	void ServerUdpConnection<T>::ReadHeader()
	{
		m_Socket.async_receive_from(asio::buffer(&m_TempPktIn.header, sizeof(packetHeader<T>)), m_RemoteEndpoint,
			[this](std::error_code ec, std::size_t length)
			{

				if (!ec) {
					if (m_TempPktIn.header.size > 0) {
						if (m_TempPktIn.header.size <= 4096) {
							m_TempPktIn.body.resize(m_TempPktIn.header.size - sizeof(packetHeader<T>));
							ReadBody();
						}
						else {
							ReadHeader();
						}
					}
					else {
						AddToIncomingPacketQueue();
					}
				}
				else {
					std::cerr << "[ServerUdpConnection] ID: " << ID << " Read header fail." << std::endl;
					m_Socket.close();
					m_bAllAsyncReturned = true;
				}
			});
	}

	template<typename T>
	void ServerUdpConnection<T>::ReadBody()
	{
		m_Socket.async_receive_from(asio::buffer(m_TempPktIn.body.data(), m_TempPktIn.body.size()), m_RemoteEndpoint,
			[this](std::error_code ec, std::size_t length)
			{

				if (!ec) {
					AddToIncomingPacketQueue();
				}
				else {
					std::cerr << "[ServerUdpConnection] ID: " << ID << " Read body fail." << "  Error: " << ec.message() << std::endl;
					m_Socket.close();
					m_bAllAsyncReturned = true;
				}

			});
	}

	template<typename T>
	void ServerUdpConnection<T>::WriteHeader()
	{
		m_Socket.async_send_to(asio::buffer(&m_QueuePacketsOut.Front().header, sizeof(packetHeader<T>)), m_RemoteEndpoint,
			[this](std::error_code ec, std::size_t length)
			{
				if (!ec) {
					if (m_QueuePacketsOut.Front().body.size() > 0) {
						WriteBody();
					}
					else {
						m_QueuePacketsOut.PopFront();

						if (!m_QueuePacketsOut.Empty())
							WriteHeader();
					}
				}
				else {
					std::cerr << "[ServerUdpConnection] ID: " << ID << " Write header fail." << std::endl;
					m_Socket.close();
				}
			});

	}

	template<typename T>
	void ServerUdpConnection<T>::WriteBody()
	{
		m_Socket.async_send_to(asio::buffer(m_QueuePacketsOut.Front().body.data(), m_QueuePacketsOut.Front().body.size()), m_RemoteEndpoint,
			[this](std::error_code ec, std::size_t length)
			{
				if (!ec) {
					m_QueuePacketsOut.PopFront();

					if (!m_QueuePacketsOut.Empty())
						WriteHeader();
				}
				else {
					std::cerr << "[ServerUdpConnection] ID: " << ID << " Write body fail." << std::endl;
					m_Socket.close();
				}
			});
	}

	template<typename T>
	void ServerUdpConnection<T>::AddToIncomingPacketQueue()
	{
		m_QueuePacketsIn.PushBack({ this->shared_from_this(), m_TempPktIn });
		ReadHeader();
	}

	template class ServerUdpConnection<MessageTypes>;
	template class ServerUdpConnection<GameMessages>;
}