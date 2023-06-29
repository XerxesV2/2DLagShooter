#include <fstream>
#include "NetServer.hpp"

namespace net
{
	template<typename T>
	ServerInterface<T>::ServerInterface(uint16_t tcp_port, uint16_t udp_port)
		: m_AsioAcceptor(m_AsioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), tcp_port))
	{
		m_udp_port = udp_port;
		std::ifstream hashFile("hash.txt", std::ios::in);
		if (hashFile.is_open()){
			m_ClientCheckSum = new char[65];
			std::string str;
			std::getline(hashFile, str);
			memcpy(m_ClientCheckSum, str.c_str(), 65);
			hashFile.close();
		}

	}

	template<typename T>
	ServerInterface<T>::~ServerInterface()
	{
		delete m_ClientCheckSum;
		Stop();
	}

	template<typename T>
	bool ServerInterface<T>::Start()
	{
		try {
			ListenForClientConnection();
			m_ContextThread = std::thread([this]() {m_AsioContext.run(); });
		}
		catch (std::exception& e) {
			std::cerr << "[SERVER] Exception: " << e.what() << std::endl;
			return false;
		}

		std::cout << "[Server] Started!" << std::endl;
		return true;
	}

	template<typename T>
	bool ServerInterface<T>::Stop()
	{
		try {
			m_AsioContext.stop();
			if (m_ContextThread.joinable()) m_ContextThread.join();
		}
		catch (std::exception& e) {
			std::cerr << "[SERVER] Exception: " << e.what() << std::endl;
			return false;
		}

		std::cout << "[Server] Stopped." << std::endl;
		return true;
	}

	template<typename T>
	void ServerInterface<T>::Update(size_t maxPackets, bool onReceive)	//onReceive is blocking
	{
		if (onReceive) m_QueuePacketsIn.Wait();	//issue
		size_t packetCount = 0;
		while (packetCount < maxPackets && !m_QueuePacketsIn.Empty()) {
			auto pkt = m_QueuePacketsIn.PopFront();
			OnTcpPacketReceived( pkt.remote, pkt.pkt );
			++packetCount;
		}

		while (packetCount < maxPackets && !m_QueueUdpPacketsIn.Empty()) {
			auto pkt = m_QueueUdpPacketsIn.PopFront();
			OnUdpPacketReceived(pkt.remote, pkt.pkt);
			++packetCount;
		}
	}

	template<typename T>
	void ServerInterface<T>::ListenForClientConnection()
	{
		m_AsioAcceptor.async_accept(
			[this](std::error_code ec, asio::ip::tcp::socket socket) {
				if (!ec) {
					std::cout << "[SERVER] New connection from: " << socket.remote_endpoint() << std::endl;

					std::shared_ptr<ServerUdpConnection<T>> newUdpConnection =
						std::make_shared<ServerUdpConnection<T>>(m_AsioContext, socket.remote_endpoint(), m_udp_port, m_QueueUdpPacketsIn);

#ifdef _DEBUG
					++m_udp_port;
#endif // _DEBUG

					std::shared_ptr<ServerConnection<T>> newConnection =
						std::make_shared<ServerConnection<T>>(m_AsioContext, std::move(socket), m_QueuePacketsIn);

					if (OnClientConnect(newConnection, newUdpConnection)) {
						newConnection->ConnectToClient(this, m_ClientCheckSum, m_CurrentID++);
						std::cout << "[SERVER] New connection ID: " << newConnection->GetID() << std::endl;
						newUdpConnection->StartListen(newConnection->GetID());
						m_DeqConnections.push_back({ std::move(newConnection), std::move(newUdpConnection) });
						//m_DeqUdpConnections.push_back(std::move(newUdpConnection));
						SendUdpPort();
					}
					else {
						std::cout << "[SERVER] ServerConnection from: " << socket.remote_endpoint() << " is refused." << std::endl;
					}
				}
				else {
					std::cerr << "[SERVER] Listen Error: " << ec.message() << std::endl;
				}
				ListenForClientConnection();
			});
	}

	template<typename T>
	void ServerInterface<T>::SendTcpPacket(std::shared_ptr<ServerConnection<T>> client, const packet<T>& pkt)
	{
		if (client) {
			if (client->IsConnected()) {
				client->Send(pkt);
			}
			else {
				OnClientDisconnect(client);	//TODO send keepalive
				client.reset();

				/*expensive*/

				m_DeqConnections.erase(std::remove_if(m_DeqConnections.begin(), m_DeqConnections.end(), [&client, this](Connection& conn)
					{
						if (client == conn.tcp) {
							conn.udp->GetSocket().cancel();
							m_DeadUdpConnections.push_back(std::move(conn.udp)); return true; }
					}), m_DeqConnections.end());

				m_DeadUdpConnections.erase(std::remove_if(m_DeadUdpConnections.begin(), m_DeadUdpConnections.end(), [](std::shared_ptr<ServerUdpConnection<T>>& conn)
					{
						return (conn->ReadyToDestroy());
					}), m_DeadUdpConnections.end());
			}
		}
		else std::cerr << "[SERVER] -SEND- client was nullptr." << std::endl;
	}

	template<typename T>
	void ServerInterface<T>::SendTcpPacketToAll(const packet<T>& pkt, std::shared_ptr<ServerConnection<T>> ignore)
	{
		bool bFoundDeadClient = false;

		for (auto& client : m_DeqConnections) {
			if (client.tcp) {
				if (client.tcp->IsConnected()) {
					if(client.tcp != ignore) client.tcp->Send(pkt);
				}
				else if (client.tcp != ignore) {
					OnClientDisconnect(client.tcp);
					client.tcp.reset();
					client.udp->GetSocket().cancel();
					m_DeadUdpConnections.push_back(std::move(client.udp));
					
					bFoundDeadClient = true;
				}
			}
			else std::cerr << "[SERVER] -SENDALL- client was nullptr." << std::endl;
		}

		if (bFoundDeadClient) {
			m_DeqConnections.erase(std::remove_if(m_DeqConnections.begin(), m_DeqConnections.end(), [](Connection& conn)
				{
					return (conn.tcp == nullptr);
				}), m_DeqConnections.end());

			m_DeadUdpConnections.erase(std::remove_if(m_DeadUdpConnections.begin(), m_DeadUdpConnections.end(), [](std::shared_ptr<ServerUdpConnection<T>>& conn)
				{
					return (conn->ReadyToDestroy());
				}), m_DeadUdpConnections.end());
		}
	}

	template<typename T>
	void ServerInterface<T>::SendUdpPacket(std::shared_ptr<ServerUdpConnection<T>> client, const packet<T>& pkt)
	{
		if (client) {
			if (client->IsConnected()) {
				client->Send(pkt);
			}
			else {
				//OnClientDisconnect(client);	
				client.reset();
				//m_DeqUdpConnections.erase(std::remove(m_DeqUdpConnections.begin(), m_DeqUdpConnections.end(), client), m_DeqUdpConnections.end());
			}
		}
		else std::cerr << "[SERVER] -SEND UDP- client was nullptr." << std::endl;
	}

	template<typename T>
	void ServerInterface<T>::SendUdpPacketToAll(const packet<T>& pkt, std::shared_ptr<ServerUdpConnection<T>> ignore)
	{
		bool bFoundDeadClient = false;

		for (auto& client : m_DeqConnections) {
			if (client.udp) {
				if (client.udp->IsConnected()) {
					if (client.udp != ignore) client.udp->Send(pkt);
				}
				else if (client.udp != ignore) {
					//OnClientDisconnect(client);
					client.udp.reset();
					bFoundDeadClient = true;
				}
			}
			else std::cerr << "[SERVER] -SENDALL UDP- client was nullptr." << std::endl;
		}

		//if (bFoundDeadClient)
			//m_DeqUdpConnections.erase(std::remove(m_DeqUdpConnections.begin(), m_DeqUdpConnections.end(), nullptr), m_DeqUdpConnections.end());
	}

	template<typename T>
	bool ServerInterface<T>::OnClientConnect(std::shared_ptr<ServerConnection<T>> tcpClient, std::shared_ptr<ServerUdpConnection<T>> udpClient)
	{
		return false;
	}

	template<typename T>
	void ServerInterface<T>::OnClientDisconnect(std::shared_ptr<ServerConnection<T>> client)
	{
		
	}

	template<typename T>
	void ServerInterface<T>::OnTcpPacketReceived(std::shared_ptr<ServerConnection<T>> client, packet<T>& packet)
	{

	}

	template<typename T>
	void ServerInterface<T>::OnUdpPacketReceived(std::shared_ptr<ServerUdpConnection<T>> client, packet<T>& packet)
	{
	}

	template<typename T>
	bool ServerInterface<T>::OnChecksumMismatch(std::shared_ptr<ServerConnection<T>> client)
	{
		return false;
	}

	template<typename T>
	void ServerInterface<T>::OnChecksumMatch(std::shared_ptr<ServerConnection<T>> client)
	{
		
	}

	template<typename T>
	void ServerInterface<T>::CheckClients()
	{
		return;
		static std::thread th([this]()
			{
				
			});
	}

	template<typename T>
	void ServerInterface<T>::SendUdpPort()
	{
		net::packet<T> UdpPortpkt;
		UdpPortpkt << GameMessages::AssignUdpPort;
		UdpPortpkt << m_DeqConnections.back().udp->GetPort();
		SendTcpPacket(m_DeqConnections.back().tcp, UdpPortpkt);
	}

	template class ServerInterface<MessageTypes>;
	template class ServerInterface<GameMessages>;
}