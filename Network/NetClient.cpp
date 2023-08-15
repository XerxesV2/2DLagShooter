#include "NetClient.hpp"

namespace net
{
	template<typename T>
	ClientInterface<T>::ClientInterface() : m_Socket(m_AsioContext)
	{
	}

	template<typename T>
	ClientInterface<T>::~ClientInterface()
	{
		Disconnect();
	}

	template<typename T>
	bool ClientInterface<T>::Connect(const std::string& host, const uint16_t tcp_port) {
		std::cout << "Server ip: " << host << "\nport: " << tcp_port << std::endl;

		try {
			asio::ip::tcp::resolver resolver(m_AsioContext);
			m_Endponts = resolver.resolve(host, std::to_string(tcp_port));

			m_Connection = std::make_unique<ClientConnection<T>>(m_AsioContext, asio::ip::tcp::socket(m_AsioContext), m_QueuePacketsIn);
			m_Connection->ConnectToServer(m_Endponts);

			/*UDP*/
			asio::ip::udp::resolver udpResolver(m_AsioContext);
			m_UdpEndpoints = udpResolver.resolve(host, std::to_string(16000));
			
			

			m_ContextThread = std::thread([this]() { m_AsioContext.run(); });
		}
		catch (std::exception& e) {
			std::cerr << "[Client] Connect Exception: " << e.what() << '\n';
			return false;
		}

		return true;
	}

	/*bool port_in_use(unsigned short port) {
		asio::io_service svc;
		asio::ip::tcp::acceptor a(svc);

		asio::error_code ec;
		a.open(asio::ip::tcp::v4(), ec) || a.bind({ asio::ip::tcp::v4(), port }, ec);

		return ec == asio::error::address_in_use;
	}*/

	template<typename T>
	bool ClientInterface<T>::SetUdp(const uint16_t udp_receive_port, const uint16_t udp_send_port)
	{
		m_UdpConnection = std::make_unique<ClientUdpConnection<T>>(m_AsioContext, *m_UdpEndpoints.begin(), udp_receive_port, udp_send_port, m_QueueUdpPacketsIn);
		m_UdpConnection->ListenToServer();
		return true;
	}

	template<typename T>
	void ClientInterface<T>::SendTcpPacket(const packet<T>& pkt)
	{
		if (IsConnected()) m_Connection->Send(pkt);
	}

	template<typename T>
	void ClientInterface<T>::SendUdpPacket(const udpPacket<T>& pkt)
	{
		if (IsConnected()) m_UdpConnection->Send(pkt);
	}

	template<typename T>
	bool ClientInterface<T>::IsConnected()
	{
		if (m_Connection)
			return m_Connection->IsConnected();
		else
			return false;
	}

	template<typename T>
	void ClientInterface<T>::Disconnect()
	{
		if (IsConnected()) {
			m_Connection->Disconnect();
		}

		m_AsioContext.stop();

		if (m_ContextThread.joinable())
			m_ContextThread.join();

		m_Connection.release();
	}

	template<typename T>
	inline tsQueue<ownedPacket<T>>& ClientInterface<T>::IncomingPackets()
	{
		return this->m_QueuePacketsIn; 
	}

	template<typename T>
	tsQueue<std::pair<ownedUdpPacket<T>, double>>& ClientInterface<T>::IncomingUdpPackets()
	{
		return this->m_QueueUdpPacketsIn;
	}

	template class ClientInterface<MessageTypes>;	//hat ezert kurvara megerte
	template class ClientInterface<GameMessages>;	
}