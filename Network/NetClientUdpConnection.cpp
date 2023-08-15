#include "NetClientUdpConnection.hpp"
#include "NetServer.hpp"

double serverTickRate = 60.0;

namespace net
{
	template<typename T>
	ClientUdpConnection<T>::ClientUdpConnection(asio::io_context& asioContext, asio::ip::udp::endpoint endpoint, const uint16_t udp_receive_port, const uint16_t udp_send_port, tsQueue<std::pair<ownedUdpPacket<T>, double>>& qPacketsIn)
		: m_AsioContext(asioContext), m_Socket(asioContext, asio::ip::udp::endpoint(asio::ip::udp::v4(), udp_receive_port)), m_QueuePacketsIn(qPacketsIn)
	{
		m_ReceiverEndpoint = asio::ip::udp::endpoint{ asio::ip::make_address(endpoint.address().to_v4().to_string()), udp_send_port };

		m_TempPktIn.body.reserve(256);
		m_TempPktIn.body.resize(256);
	}

	template<typename T>
	ClientUdpConnection<T>::~ClientUdpConnection()
	{
		//m_Socket.cancel();
		th->detach();
		th.reset();
	}

	template<typename T>
	void ClientUdpConnection<T>::SetEndPoint(const asio::ip::udp::resolver::results_type& endpoints)
	{
		
	}

	template<typename T>
	void ClientUdpConnection<T>::ListenToServer()
	{
		/*asio::post(m_AsioContext,
			[this]() {
				ReadHeader();
			});*/

		th = std::make_unique<std::thread>([this]()
			{
				while (1)
				{
					ReadBody();
				}
			});
	}

	template<typename T>
	const bool ClientUdpConnection<T>::IsConnected() const
	{
		return m_Socket.is_open();
	}

	template<typename T>
	void ClientUdpConnection<T>::Send(const udpPacket<T>& pkt)
	{
		/*injecting work*/
		asio::post(m_AsioContext,
			[this, pkt]() {
				bool WritingPacket = !m_QueuePacketsOut.Empty();
				m_QueuePacketsOut.PushBack(pkt);	//donno man still chance for not to call WriteHeader

				if (!WritingPacket) WriteBody();
			});
	}

	template<typename T>
	void ClientUdpConnection<T>::ReadHeader()
	{
		ReadBody();
		return;

		m_Socket.async_receive_from(asio::buffer(&m_TempPktIn.header, sizeof(udpPacketHeader<T>)), m_RemoteEndpoint,
			[this](std::error_code ec, std::size_t length)
			{
				if (!ec) {
					if (m_TempPktIn.header.size > 0) {
						if (m_TempPktIn.header.size <= 4096) {
							m_TempPktIn.body.resize(m_TempPktIn.header.size - sizeof(udpPacketHeader<T>));	//hmmm sercurity problem
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
		std::error_code ec;
		std::size_t length = m_Socket.receive_from(asio::buffer(m_TempPktIn.body.data(), m_TempPktIn.body.size()), m_RemoteEndpoint, 0, ec);

		if (!ec) {
			m_TempPktIn.header = *(udpPacketHeader<T>*)m_TempPktIn.body.data();
			m_TempPktIn.body.erase(m_TempPktIn.body.begin(), m_TempPktIn.body.begin() + sizeof(udpPacketHeader<T>));
			m_TempPktIn.body.resize(m_TempPktIn.header.size - sizeof(udpPacketHeader<T>));

			AddToIncomingPacketQueue();
			m_TempPktIn.body.resize(256);
		}
		else {
			std::cerr << "[ClientUdpConnection] ID: " << ID << " Read body fail." << "  Error: " << ec.message() << std::endl;
			//m_Socket.close();
		}
	}

	template<typename T>
	void ClientUdpConnection<T>::WriteHeader()
	{
		WriteBody();
		return;

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
		std::error_code ec;
		std::size_t length = m_Socket.send_to(asio::buffer(m_QueuePacketsOut.Front().body.data(), m_QueuePacketsOut.Front().body.size()), m_ReceiverEndpoint, 0, ec);

		if (!ec) {
			m_QueuePacketsOut.PopFront();
			if (!m_QueuePacketsOut.Empty())
				WriteBody();
				//WriteHeader();
		}
		else {
			std::cerr << "[ClientUdpConnection] ID: " << ID << " Write body fail." << std::endl;
			m_Socket.close();
		}
	}

	template<typename T>
	void ClientUdpConnection<T>::AddToIncomingPacketQueue()
	{
		static double baseTime = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() + BufferTime;
		if(m_QueuePacketsIn.Size() == 0)
			baseTime = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() + BufferTime;
		
		baseTime += 1.0 / serverTickRate;

		m_QueuePacketsIn.PushBack({ {m_TempPktIn}, baseTime });
		
	}

	template class ClientUdpConnection<MessageTypes>;
	template class ClientUdpConnection<GameMessages>;
}