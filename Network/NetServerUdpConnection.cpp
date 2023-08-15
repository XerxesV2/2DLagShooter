#include "NetServerUdpConnection.hpp"
#include "NetServer.hpp"

namespace net
{
	template<typename T>
	ServerUdpConnection<T>::ServerUdpConnection(uint32_t id)
	{
		this->ID = id;
	}

	template<typename T>
	ServerUdpConnection<T>::~ServerUdpConnection()
	{
	}

	template<typename T>
	void ServerUdpConnection<T>::StartListen()
	{
		m_TempPktIn.body.reserve(256);
		m_TempPktIn.body.resize(256);
		static std::thread th([]()
			{
				while (1)
				{
						ReadBody();
				}
			});
	}

	template<typename T>
	void ServerUdpConnection<T>::AddRemoteEndpoint(asio::ip::tcp::endpoint endpoint, uint16_t port, uint32_t id, std::shared_ptr<ServerUdpConnection<T>> sptr)
	{
		m_RemoteEndpointsById.insert({ id, asio::ip::udp::endpoint{ endpoint.address().to_v4(), port }});
#ifdef FOR_LOCAL_DEBUG
		m_UdpClientsByIp.insert({ endpoint.address().to_v4().to_uint() + port, sptr});
#else
		m_UdpClientsByIp.insert({ id, sptr});
#endif // FOR_LOCAL_DEBUG
	}

	template<typename T>
	void ServerUdpConnection<T>::RemoveRemoteEndpoint(uint32_t id)
	{
		m_UdpClientsByIp.erase(m_RemoteEndpointsById[id].address().to_v4().to_uint());
		m_RemoteEndpointsById.erase(id);
	}

	template<typename T>
	void ServerUdpConnection<T>::Send(const udpPacket<T>& pkt, uint32_t id)
	{
		/*injecting work*/
		asio::post(*m_AsioContext,
			[pkt, id]() {
				bool WritingPacket = !m_QueuePacketsOut.Empty();
				m_QueuePacketsOut.PushBack(pkt);

				if (!WritingPacket) WriteBody(id);
			});
	}

	template<typename T>
	void ServerUdpConnection<T>::ReadHeader()
	{
		ReadBody();
		return;

		static std::error_code ec; 
		std::size_t length = m_Socket->receive_from(asio::buffer(&m_TempPktIn.header, sizeof(udpPacketHeader<T>)), m_RemoteEndpoint, 0, ec);

		if (!ec) {
			if (m_TempPktIn.header.size > 0) {
				if (m_TempPktIn.header.size <= 4096) {
					m_TempPktIn.body.resize(m_TempPktIn.header.size - sizeof(udpPacketHeader<T>));
					ReadBody();
				}
				else {
					return;
				}
			}
			else {
#ifdef FOR_LOCAL_DEBUG
				AddToIncomingPacketQueue(m_RemoteEndpoint.address().to_v4().to_uint() + m_RemoteEndpoint.port());
#else
				AddToIncomingPacketQueue(m_TempPktIn.header.id);
#endif
			}
		}
		else {
			std::cerr << "[ServerUdpConnection] ID:  Read header fail." << std::endl;
			//m_Socket->close();
			m_bAllAsyncReturned = true;
			return;
		}
			
	}

	template<typename T>
	void ServerUdpConnection<T>::ReadBody()
	{
		static std::error_code ec;
		std::size_t length = m_Socket->receive_from(asio::buffer(m_TempPktIn.body.data(), m_TempPktIn.body.size()), m_RemoteEndpoint, 0, ec);
			
		if (!ec) {
			m_TempPktIn.header = *(udpPacketHeader<T>*)m_TempPktIn.body.data();
			m_TempPktIn.body.erase(m_TempPktIn.body.begin(), m_TempPktIn.body.begin() + sizeof(udpPacketHeader<T>));

			if (m_TempPktIn.header.size > 0) {
				if (m_TempPktIn.header.size <= 4096) {
					m_TempPktIn.body.resize(m_TempPktIn.header.size - sizeof(udpPacketHeader<T>));
#ifdef FOR_LOCAL_DEBUG
					AddToIncomingPacketQueue(m_RemoteEndpoint.address().to_v4().to_uint() + m_RemoteEndpoint.port());
#else
					AddToIncomingPacketQueue(m_TempPktIn.header.id);
#endif // FOR_LOCAL_DEBUG
					m_TempPktIn.body.resize(256);
				}
				else {
					return;
				}
			}
			else {
#ifdef FOR_LOCAL_DEBUG
				AddToIncomingPacketQueue(m_RemoteEndpoint.address().to_v4().to_uint() + m_RemoteEndpoint.port());
#else
				AddToIncomingPacketQueue(m_TempPktIn.header.id);
#endif // FOR_LOCAL_DEBUG

			}
		}
		else {
			std::cerr << "[ServerUdpConnection] ID: Read body fail." << "  Error: " << ec.message() << std::endl;
			//m_Socket->close();
			m_bAllAsyncReturned = true;
			return;
		}

			
	}

	template<typename T>
	void ServerUdpConnection<T>::WriteHeader(uint32_t id)
	{
		WriteBody(id);
		return;

		//printf("sending udp to: %s on port: %d\n", m_RemoteEndpointsById[id].address().to_v4().to_string().c_str(), m_RemoteEndpointsById[id].port());
		std::error_code ec;
		std::size_t length = m_Socket->send_to(asio::buffer(&m_QueuePacketsOut.Front().header, sizeof(udpPacketHeader<T>)), m_RemoteEndpointsById[id], 0, ec);
		
		if (!ec) {
			if (m_QueuePacketsOut.Front().body.size() > 0) {
				WriteBody(id);
			}
			else {
				m_QueuePacketsOut.PopFront();

				if (!m_QueuePacketsOut.Empty())
					WriteHeader(id);
			}
		}
		else {
			std::cerr << "[ServerUdpConnection] ID: " << id << " Write header fail." << std::endl;
			//m_Socket->close();
			WriteHeader(id);
		}
		

		//m_Socket->async_send_to(asio::buffer(&m_QueuePacketsOut.Front().header, sizeof(packetHeader<T>)), m_RemoteEndpointsById[id],
		//	[=](std::error_code ec, std::size_t length)
		//	{
		//		if (!ec) {
		//			if (m_QueuePacketsOut.Front().body.size() > 0) {
		//				WriteBody(id);
		//				printf("id: %d\n", id);
		//			}
		//			else {
		//				m_QueuePacketsOut.PopFront();

		//				if (!m_QueuePacketsOut.Empty())
		//					WriteHeader(id);
		//			}
		//		}
		//		else {
		//			std::cerr << "[ServerUdpConnection] ID: " << id << " Write header fail." << std::endl;
		//			//m_Socket->close();
		//			WriteHeader(id);
		//		}
		//	});

	}

	template<typename T>
	void ServerUdpConnection<T>::WriteBody(uint32_t id)
	{
		std::error_code ec;
		std::size_t length = m_Socket->send_to(asio::buffer(m_QueuePacketsOut.Front().body.data(), m_QueuePacketsOut.Front().body.size()), m_RemoteEndpointsById[id], 0, ec);
		
		if (!ec) {
			m_QueuePacketsOut.PopFront();

			if (!m_QueuePacketsOut.Empty())
				WriteBody(id);
		}
		else {
			std::cerr << "[ServerUdpConnection] ID: " << id << " Write body fail." << std::endl;
			//m_Socket->close();
			WriteBody(id);
		}
		

		//m_Socket->async_send_to(asio::buffer(m_QueuePacketsOut.Front().body.data(), m_QueuePacketsOut.Front().body.size()), m_RemoteEndpointsById[id],
		//	[=](std::error_code ec, std::size_t length)
		//	{
		//		if (!ec) {
		//			m_QueuePacketsOut.PopFront();

		//			if (!m_QueuePacketsOut.Empty())
		//				WriteHeader(id);
		//		}
		//		else {
		//			std::cerr << "[ServerUdpConnection] ID: " << id << " Write body fail." << std::endl;
		//			//m_Socket->close();
		//			WriteHeader(id);
		//		}
		//	});
	}

	template<typename T>
	void ServerUdpConnection<T>::AddToIncomingPacketQueue(uint32_t uid)
	{
		if (m_UdpClientsByIp.find(uid) == m_UdpClientsByIp.end()) {
			printf("AddToIncomingPacketQueue non existent uip\n");
			return;
		}
		m_QueuePacketsIn->PushBack({ m_UdpClientsByIp[uid], m_TempPktIn });
		//ReadHeader();
	}

	template class ServerUdpConnection<MessageTypes>;
	template class ServerUdpConnection<GameMessages>;
}