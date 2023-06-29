#include "NetClientUdpConnection.hpp"
#include "NetServer.hpp"

namespace net
{
	template<typename T>
	ClientUdpConnection<T>::ClientUdpConnection(asio::io_context& asioContext, asio::ip::udp::endpoint endpoint, uint16_t destPort, tsQueue<ownedUdpPacket<T>>& qPacketsIn)
		: m_AsioContext(asioContext), m_Socket(asioContext, asio::ip::udp::endpoint{ asio::ip::make_address(endpoint.address().to_v4().to_string()), 0 }), m_QueuePacketsIn(qPacketsIn)
	{
		m_ReceiverEndpoint = asio::ip::udp::endpoint{ asio::ip::make_address(endpoint.address().to_v4().to_string()), destPort };
	}

	template<typename T>
	ClientUdpConnection<T>::~ClientUdpConnection()
	{
	}

	template<typename T>
	void ClientUdpConnection<T>::SetEndPoint(const asio::ip::udp::resolver::results_type& endpoints)
	{
		
	}

	template<typename T>
	void ClientUdpConnection<T>::ListenToServer()
	{
		asio::post(m_AsioContext,
			[this]() {
				ReadHeader();
			});
	}

	template<typename T>
	const bool ClientUdpConnection<T>::IsConnected() const
	{
		return m_Socket.is_open();
	}

	template<typename T>
	void ClientUdpConnection<T>::Send(const packet<T>& pkt)
	{
		/*injecting work*/
		asio::post(m_AsioContext,
			[this, pkt]() {
				bool WritingPacket = !m_QueuePacketsOut.Empty();
				m_QueuePacketsOut.PushBack(pkt);	//donno man still chance for not to call WriteHeader

				if (!WritingPacket) WriteHeader();
			});
	}

	template<typename T>
	void ClientUdpConnection<T>::ReadHeader()
	{
		m_Socket.async_receive_from(asio::buffer(&m_TempPktIn.header, sizeof(packetHeader<T>)), m_RemoteEndpoint,
			[this](std::error_code ec, std::size_t length)
			{
				if (!ec) {
					if (m_TempPktIn.header.size > 0) {
						if (m_TempPktIn.header.size <= 4096) {
							m_TempPktIn.body.resize(m_TempPktIn.header.size - sizeof(packetHeader<T>));	//hmmm sercurity problem
							ReadBody();
						}
						else {
							//TODO ban
							ReadHeader();
						}
					}
					else {
						AddToIncomingPacketQueue();
					}
				}
				else {
					std::cerr << "[ClientUdpConnection] ID: " << ID << " Read header fail." << std::endl;
					m_Socket.close();
				}
			});

	}

	template<typename T>
	void ClientUdpConnection<T>::ReadBody()
	{
		m_Socket.async_receive_from(asio::buffer(m_TempPktIn.body.data(), m_TempPktIn.body.size()),	m_RemoteEndpoint,
			[this](std::error_code ec, std::size_t length)
			{

				if (!ec) {
					AddToIncomingPacketQueue();
				}
				else {
					std::cerr << "[ClientUdpConnection] ID: " << ID << " Read body fail." << "  Error: " << ec.message() << std::endl;
					m_Socket.close();
				}

			});
	}

	template<typename T>
	void ClientUdpConnection<T>::WriteHeader()
	{
		m_Socket.async_send_to(asio::buffer(&m_QueuePacketsOut.Front().header, sizeof(packetHeader<T>)), m_ReceiverEndpoint,
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
					std::cerr << "[ClientUdpConnection] ID: " << ID << " Write header fail.\n" << ec.message() << std::endl;
					m_Socket.close();
				}
			});

	}

	template<typename T>
	void ClientUdpConnection<T>::WriteBody()
	{
		m_Socket.async_send_to(asio::buffer(m_QueuePacketsOut.Front().body.data(), m_QueuePacketsOut.Front().body.size()), m_ReceiverEndpoint,
			[this](std::error_code ec, std::size_t length)
			{
				if (!ec) {
					m_QueuePacketsOut.PopFront();
					if (!m_QueuePacketsOut.Empty())
						WriteHeader();
				}
				else {
					std::cerr << "[ClientUdpConnection] ID: " << ID << " Write body fail." << std::endl;
					m_Socket.close();
				}
			});
	}

	template<typename T>
	void ClientUdpConnection<T>::AddToIncomingPacketQueue()
	{
		m_QueuePacketsIn.PushBack({ m_TempPktIn });
		ReadHeader();
	}

	template class ClientUdpConnection<MessageTypes>;
	template class ClientUdpConnection<GameMessages>;
}