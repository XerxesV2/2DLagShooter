#include <fstream>
#include "NetServer.hpp"

namespace net
{
	template<typename T>
	ServerInterface<T>::ServerInterface(uint16_t tcp_port)
		: m_AsioAcceptor(m_AsioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), tcp_port)),
		m_UdpSocket(m_AsioContext, asio::ip::udp::endpoint(asio::ip::udp::v4(), m_client_send_udp_port))
	{
		printf("Udp receive port: %d\nUdp send port: %d", m_client_receive_udp_port, m_client_send_udp_port);
	
		ServerUdpConnection<T>::m_AsioContext = &m_AsioContext;
		ServerUdpConnection<T>::m_Socket = &m_UdpSocket;
		ServerUdpConnection<T>::m_QueuePacketsIn = &m_QueueUdpPacketsIn;

		ServerUdpConnection<T>::StartListen();

		std::ifstream hashFile("hash.txt", std::ios::in);
		if (hashFile.is_open()){
			m_ClientChecksum = new char[65];
			std::string str;
			std::getline(hashFile, str);
			memcpy(m_ClientChecksum, str.c_str(), 65);
			hashFile.close();
		}

		CreateNewConnectionGroup((uint32_t)ConnectionGroup::LOBBY);
		CreateNewConnectionGroup((uint32_t)ConnectionGroup::GAME);
	}

	template<typename T>
	ServerInterface<T>::~ServerInterface()
	{
		delete m_ClientChecksum;
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
				try
				{
					if (!ec) {
						printf("\n+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n");
						std::cout << "[SERVER] New connection from: " << socket.remote_endpoint() << std::endl;

						std::shared_ptr<ServerUdpConnection<T>> newUdpConnection =
							std::make_shared<ServerUdpConnection<T>>(m_CurrentID);
						ServerUdpConnection<T>::AddRemoteEndpoint(socket.remote_endpoint(), m_client_receive_udp_port, m_CurrentID, newUdpConnection);

						std::shared_ptr<ServerConnection<T>> newTcpConnection =
							std::make_shared<ServerConnection<T>>(m_AsioContext, std::move(socket), m_QueuePacketsIn);

						newTcpConnection->SetGroupID((uint32_t)ConnectionGroup::LOBBY);
						newUdpConnection->SetGroupID((uint32_t)ConnectionGroup::LOBBY);

						if (OnClientConnect(newTcpConnection, newUdpConnection)) {
							newTcpConnection->ConnectToClient(this, m_ClientChecksum, m_CurrentID++);
							std::cout << "[SERVER] New connection ID: " << newTcpConnection->GetID() << std::endl;
							//newUdpConnection->SetPort(m_client_receive_udp_port);
							
							AddConnectionToGroup(newTcpConnection, newUdpConnection, newTcpConnection->GetGroupID());
							//m_DeqConnections.push_back({ std::move(newTcpConnection), std::move(newUdpConnection) });
							SendUdpPort();

#ifdef _DEBUG
							++m_client_receive_udp_port;
#endif // _DEBUG
						}
						else {
							std::cout << "[SERVER] ServerConnection from: " << socket.remote_endpoint() << " is refused." << std::endl;
						}
					}
					else {
						std::cerr << "[SERVER] Listen Error: " << ec.message() << std::endl;
					}
					ListenForClientConnection();
				}
				catch (std::exception e)
				{
					std::cout << "[NEW CONNECTION] Error: " << e.what() << std::endl;
					ListenForClientConnection();
				}
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

				/*expensive*/

				RemoveConnection(client);
			}
		}
		else std::cerr << "[SERVER] -SEND- client was nullptr." << std::endl;
	}

	template<typename T>
	void ServerInterface<T>::SendTcpPacketToAll(const uint32_t gid, const packet<T>& pkt, std::shared_ptr<ServerConnection<T>> ignore)
	{
		std::deque<Connection>& deqConnections = m_MapConnectionGroups[gid];

		bool bFoundDeadClient = false;

		for (auto& client : deqConnections) {
			if (client.tcp) {
				if (client.tcp->IsConnected()) {
					if(client.tcp != ignore) client.tcp->Send(pkt);
				}
				else if (client.tcp != ignore) {
					OnClientDisconnect(client.tcp);
					client.tcp.reset();
					ServerUdpConnection<T>::RemoveRemoteEndpoint(client.udp->GetID());
					
					bFoundDeadClient = true;
				}
			}
			else std::cerr << "[SERVER] -SENDALL- client was nullptr." << std::endl;
		}

		if (bFoundDeadClient) {
			deqConnections.erase(std::remove_if(deqConnections.begin(), deqConnections.end(), [](Connection& conn)
				{
					return (conn.tcp == nullptr);
				}), deqConnections.end());
		}
	}

	template<typename T>
	void ServerInterface<T>::SendUdpPacket(std::shared_ptr<ServerUdpConnection<T>> client, const udpPacket<T>& pkt)
	{
		if (client) {
			if (client->IsConnected()) {
				client->Send(pkt, client->GetID());
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
	void ServerInterface<T>::SendUdpPacketToAll(const uint32_t gid, const udpPacket<T>& pkt, std::shared_ptr<ServerUdpConnection<T>> ignore)
	{
		std::deque<Connection>& deqConnections = m_MapConnectionGroups[gid];

		bool bFoundDeadClient = false;

		for (auto& client : deqConnections) {
			if (client.udp) {
				if (client.udp->IsConnected()) {
					if (client.udp != ignore) client.udp->Send(pkt, client.udp->GetID());
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
	void ServerInterface<T>::RemoveConnection(std::shared_ptr<ServerConnection<T>> client)
	{
		std::deque<Connection>& deqConnections = m_MapConnectionGroups[client->GetGroupID()];
		deqConnections.erase(std::remove_if(deqConnections.begin(), deqConnections.end(), [&client, this](Connection& conn)
			{
				if (client == conn.tcp) {
					if (conn.tcp->IsConnected()) conn.tcp->GetSocket().cancel();
					ServerUdpConnection<T>::RemoveRemoteEndpoint(conn.udp->GetID());
					m_DeadConnections.push_back(std::move(conn));
					return true;
				}
			}), deqConnections.end());
		//client.reset();

		m_DeadConnections.erase(std::remove_if(m_DeadConnections.begin(), m_DeadConnections.end(), [](Connection& conn)
			{
				return ((conn.tcp && conn.tcp->ReadyToDestroy()));
			}), m_DeadConnections.end());
	}

	template<typename T>
	void ServerInterface<T>::CreateNewConnectionGroup(uint32_t groupId)
	{
		if (m_MapConnectionGroups.find(groupId) != m_MapConnectionGroups.end()) {
			std::cout << "[CreateNewConnectionGroup] id: " << groupId << " is already exists\n";	//should never happen
			return;
		}
		m_MapConnectionGroups.insert({ groupId, std::deque<Connection>{} });
	}

	template<typename T>
	void ServerInterface<T>::AddConnectionToGroup(std::shared_ptr<ServerConnection<T>> tcp, std::shared_ptr<ServerUdpConnection<T>> udp, uint32_t groupId)
	{
		if (m_MapConnectionGroups.find(groupId) == m_MapConnectionGroups.end()) {
			std::cout << "[AddConnectionToGroup] id: " << groupId << " is not found\n";	//should never happen either
			return;
		}
		m_MapConnectionGroups[groupId].push_back({ std::move(tcp), std::move(udp) });
	}

	template<typename T>
	void ServerInterface<T>::MoveConnectionToGroup(std::shared_ptr<ServerConnection<T>> tcp, uint32_t groupId)
	{
		//bit fckin slow
		if (m_MapConnectionGroups.find(groupId) == m_MapConnectionGroups.end()) {
			std::cout << "[MoveConnectionToGroup] id: " << groupId << " is not found\n";	//should never happen
			return;
		}

		std::deque<Connection>& deqFromConnections = m_MapConnectionGroups[tcp->GetGroupID()];
		std::deque<Connection>& deqToConnections = m_MapConnectionGroups[groupId];
		deqFromConnections.erase(std::remove_if(deqFromConnections.begin(), deqFromConnections.end(), [&, this](Connection& conn)
			{
				if (tcp == conn.tcp) {
					conn.tcp->SetGroupID(groupId);
					conn.udp->SetGroupID(groupId);
					deqToConnections.push_back(std::move(conn));
					return true;
				}
			}), deqFromConnections.end());
	}

	template<typename T>
	void ServerInterface<T>::AddFreedUdpPort(const uint16_t port)
	{
		m_FreeUdpPorts.PushFront(port);
	}

	template<typename T>
	bool ServerInterface<T>::OnClientConnect(std::shared_ptr<ServerConnection<T>> tcpClient, std::shared_ptr<ServerUdpConnection<T>> udpClient)
	{
		return false;
	}

	template<typename T>
	void ServerInterface<T>::OnClientDisconnect(const std::shared_ptr<ServerConnection<T>>& client)
	{
		
	}

	template<typename T>
	void ServerInterface<T>::OnTcpPacketReceived(std::shared_ptr<ServerConnection<T>> client, packet<T>& packet)
	{

	}

	template<typename T>
	void ServerInterface<T>::OnUdpPacketReceived(std::shared_ptr<ServerUdpConnection<T>> client, udpPacket<T>& packet)
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
	void ServerInterface<T>::SendUdpPort()
	{
		net::packet<T> UdpPortpkt;
		UdpPortpkt << GameMessages::AssignUdpPort;
		UdpPortpkt << m_client_receive_udp_port;
		UdpPortpkt << m_client_send_udp_port; //m_MapConnectionGroups[(uint32_t)ConnectionGroup::LOBBY].back().udp->GetPort();
		SendTcpPacket(m_MapConnectionGroups[(uint32_t)ConnectionGroup::LOBBY].back().tcp, UdpPortpkt);
	}

	template class ServerInterface<MessageTypes>;
	template class ServerInterface<GameMessages>;
}