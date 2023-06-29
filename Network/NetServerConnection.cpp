#include "NetServerConnection.hpp"
#include "NetServer.hpp"

namespace net
{
	template<typename T>
	ServerConnection<T>::ServerConnection(asio::io_context& asioContext, asio::ip::tcp::socket socket, tsQueue<ownedPacket<T>>& qPacketsIn)
		: m_AsioContext(asioContext), m_Socket(std::move(socket)), m_QueuePacketsIn(qPacketsIn)
	{
		char hs[65];
		memset(hs, 0, 65);
		checksumPkt << hs;	//to set the correct size
		checksumPkt.body.resize(checksumPkt.body.size() + sizeof(packetHeader<T>));
	}

	template<typename T>
	ServerConnection<T>::~ServerConnection()
	{
	}

	template<typename T>
	const bool ServerConnection<T>::ConnectToClient(ServerInterface<T>* server, const char* const hash, uint32_t uid)
	{
		if (m_Socket.is_open()) {
			this->ID = uid;

			ReadChecksum(server, hash);

			//ReadHeader();
		}
		return true;	//wtf is this
	}

	template<typename T>
	const bool ServerConnection<T>::Disconnect()
	{
		if (IsConnected())
			asio::post(m_AsioContext, [this]() { m_Socket.close(); });
		else
			return false;
		return true;
	}

	template<typename T>
	const bool ServerConnection<T>::IsConnected() const
	{
		return m_Socket.is_open();
	}

	template<typename T>
	void ServerConnection<T>::Send(const packet<T>& pkt)
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
	void ServerConnection<T>::ReadHeader()
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
					std::cerr << "[ServerConnection] ID: " << ID << " Read header fail." << std::endl;
					m_Socket.close();
				}
			});
	}

	template<typename T>
	void ServerConnection<T>::ReadBody()
	{
		asio::async_read(m_Socket, asio::buffer(m_TempPktIn.body.data(), m_TempPktIn.body.size()),	//hmmm
			[this](std::error_code ec, std::size_t length)
			{

				if (!ec) {
					AddToIncomingPacketQueue();
				}
				else {
					std::cerr << "[ServerConnection] ID: " << ID << " Read body fail." << "  Error: " << ec.message() << std::endl;
					m_Socket.close();
				}

			});
	}

	template<typename T>
	void ServerConnection<T>::WriteHeader()
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
					std::cerr << "[ServerConnection] ID: " << ID << " Write header fail." << std::endl;
					m_Socket.close();
				}
			});

	}

	template<typename T>
	void ServerConnection<T>::WriteBody()
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
					std::cerr << "[ServerConnection] ID: " << ID << " Write body fail." << std::endl;
					m_Socket.close();
				}
			});
	}

	template<typename T>
	void ServerConnection<T>::AddToIncomingPacketQueue()
	{
		m_QueuePacketsIn.PushBack({ this->shared_from_this(), m_TempPktIn });
		ReadHeader();
	}

	template<typename T>
	void ServerConnection<T>::ReadChecksum(ServerInterface<T>* server, const char* const hash)
	{

		asio::async_read(m_Socket, asio::buffer(checksumPkt.body.data(), checksumPkt.body.size()),
			[this, server, hash](std::error_code ec, std::size_t length)
			{
				if (!ec) {
					memcpy(m_CheckSumIn, checksumPkt.body.data() + sizeof(packetHeader<T>), 65);
					if (!std::strcmp(m_CheckSumIn, hash)) {
						server->OnChecksumMatch(this->shared_from_this());
						ReadHeader();
					}
					else {
						if (server->OnChecksumMismatch(this->shared_from_this())) {
							std::cerr << "[ServerConnection] ID: " << ID << " Checksum validation fail." << std::endl;
							net::packet<MessageTypes> pkt;
							pkt << MessageTypes::BadClient;
							//Send(pkt);
							m_Socket.close();
						}
						else {
							server->OnChecksumMatch(this->shared_from_this());
							ReadHeader();
						}
					}
				}
				else {
					std::cerr << "[ServerConnection] ID: " << ID << " ReadChecksum fail." << std::endl;
					m_Socket.close();
				}
			});
	}

	template class ServerConnection<MessageTypes>;
	template class ServerConnection<GameMessages>;
}