#include "NetClientConnection.hpp"
#include "NetServer.hpp"

namespace net
{
	template<typename T>
	ClientConnection<T>::ClientConnection(asio::io_context& asioContext, asio::ip::tcp::socket socket, tsQueue<ownedPacket<T>>& qPacketsIn)
		: m_AsioContext(asioContext), m_Socket(std::move(socket)), m_QueuePacketsIn(qPacketsIn)
	{

	}
	
	template<typename T>
	ClientConnection<T>::~ClientConnection()
	{
	}

	template<typename T>
	const bool ClientConnection<T>::ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints)
	{
		asio::async_connect(m_Socket, endpoints,
			[this](std::error_code ec, asio::ip::tcp::endpoint endpoint) 
			{
				if (!ec) {
					ReadHeader();
				}
				else {
					std::cerr << "[ClientConnection] ID: " << ID << " Connect to server fail.\nReason: " << ec.message() << std::endl;
					return false;
				}

			});
		return true;
	}

	template<typename T>
	const bool ClientConnection<T>::Disconnect()
	{
		if (IsConnected())
			asio::post(m_AsioContext, [this]() { m_Socket.close(); });
		else
			return false;
		return true;
	}

	template<typename T>
	const bool ClientConnection<T>::IsConnected() const
	{
		return m_Socket.is_open();
	}

	template<typename T>
	void ClientConnection<T>::Send(const packet<T>& pkt)
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
	void ClientConnection<T>::ReadHeader()
	{
		asio::async_read(m_Socket, asio::buffer(&m_TempPktIn.header, sizeof(packetHeader<T>)),
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
				std::cerr << "[ClientConnection] ID: " << ID << " Read header fail.\nReason: " << ec.message() << std::endl;
				m_Socket.close();
			}
		});
			
	}

	template<typename T>
	void ClientConnection<T>::ReadBody()
	{
		asio::async_read(m_Socket, asio::buffer(m_TempPktIn.body.data(), m_TempPktIn.body.size()),	//hmmm
			[this](std::error_code ec, std::size_t length)
			{

				if (!ec) {
					AddToIncomingPacketQueue();
				}
				else {
					std::cerr << "[ClientConnection] ID: " << ID << " Read body fail." << "  Error: " << ec.message() << std::endl;
					m_Socket.close();
				}
					
			});
	}

	template<typename T>
	void ClientConnection<T>::WriteHeader()
	{
		asio::async_write(m_Socket, asio::buffer(&m_QueuePacketsOut.Front().header, sizeof(packetHeader<T>)),
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
				std::cerr << "[ClientConnection] ID: " << ID << " Write header fail.\nReason: " << ec.message() << std::endl;
				m_Socket.close();
			}
		});
			
	}

	template<typename T>
	void ClientConnection<T>::WriteBody()
	{
		asio::async_write(m_Socket, asio::buffer(m_QueuePacketsOut.Front().body.data(), m_QueuePacketsOut.Front().body.size()),	//optmiez
			[this](std::error_code ec, std::size_t length)
			{
				if (!ec) {
					m_QueuePacketsOut.PopFront();

					if (!m_QueuePacketsOut.Empty())
						WriteHeader();
				}
				else {
					std::cerr << "[ClientConnection] ID: " << ID << " Write body fail." << std::endl;
					m_Socket.close();
				}
			});
	}

	template<typename T>
	void ClientConnection<T>::AddToIncomingPacketQueue()
	{
		m_QueuePacketsIn.PushBack({ m_TempPktIn });
		ReadHeader();
	}

	template class ClientConnection<MessageTypes>;
	template class ClientConnection<GameMessages>;
}