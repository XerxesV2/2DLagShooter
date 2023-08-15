#pragma once
#include "includes.hpp"
#include "NetThreadSafeQueue.hpp"

namespace net 
{
	template <typename T>
	struct packetHeader
	{
#ifdef OLD_PACKET
		T id{};
#endif
		float sendTime = 0.f;
		uint32_t size = 0;
	};

	template <typename T>
	struct packet
	{
		packetHeader<T> header{};
		std::vector<uint8_t> body;	//in bytes
		uint32_t bytesRead = 0;

		size_t Size() const {
			return sizeof(packetHeader<T>) + body.size();
		}

		void Clear() {
			body.clear();
			header.size = sizeof(packetHeader<T>);
		}

		GameMessages GetNextSubPacketType() {
			bytesRead += sizeof(GameMessages);	//no temp var pls
			return *(GameMessages*)(body.data() + bytesRead - sizeof(GameMessages));
		}

		template<typename dataType>
		dataType* GetSubPacketPtr() {
			bytesRead += sizeof(dataType);
			if (bytesRead > body.size()) return nullptr;
			return (dataType*)(body.data() + bytesRead - sizeof(dataType));
		}

		GameMessages PeekNextPacketType() {
			if (bytesRead == body.size()) return GameMessages::None;
			return *(GameMessages*)(body.data() + bytesRead);
		}

		friend std::ostream& operator << (std::ostream& os, const packet<T>& pac) {
			os << "ID: " << (int)pac.header.id << "  Size: " << pac.header.size << "\n";
			return os;
		}

		template<typename dataType>
		friend packet<T>& operator << (packet<T>& pac, const dataType& data) {

#ifdef _WIN32
			//static_assert(std::is_standard_layout<dataType>::value, "Data is too complex to be pushed at line: " + __LINE__);
			static_assert(std::is_standard_layout<dataType>::value, "Data is too complex to be pushed");
#else
			static_assert(std::is_standard_layout<dataType>::value, "Data is too complex to be pushed");
#endif
			const size_t offset = pac.body.size();	//prev size, the point to memcopy the data
			pac.body.resize(pac.body.size() + sizeof(dataType));	//resize preformance issue
			std::memcpy(pac.body.data() + offset, &data, sizeof(dataType));
			pac.header.size = pac.Size();

			return pac;
		}

		template<typename dataType>
		friend packet<T>& operator >> (packet<T>& pac, dataType& data) {

#ifdef _WIN32
			//static_assert(std::is_standard_layout<dataType>::value, "Data is too complex to be pushed at line: " + __LINE__);
			static_assert(std::is_standard_layout<dataType>::value, "Data is too complex to be pushed");
#else
			static_assert(std::is_standard_layout<dataType>::value, "Data is too complex to be pushed");
#endif
			const size_t offset = pac.body.size() - sizeof(dataType);	//offset from th back
			std::memcpy(&data, pac.body.data() + offset, sizeof(dataType));
			pac.body.resize(offset);
			pac.header.size = offset;

			return pac;
		}
	};


	template <typename T>
	struct udpPacketHeader
	{
		float sendTime = 0.f;
		uint32_t id = 0;
		uint32_t size = 0;
	};

	template <typename T>
	struct udpPacket
	{
		udpPacket()
		{
			body.resize(sizeof(udpPacketHeader<T>));
		}

		udpPacketHeader<T> header{};
		std::vector<uint8_t> body;	//in bytes
		uint32_t bytesRead = 0;

		size_t Size() const {
			return body.size();
		}

		void Clear() {
			//body.clear();
			body.resize(sizeof(udpPacketHeader<T>));	//is this enought?
			header.size = sizeof(udpPacketHeader<T>);
		}

		void AddHeaderToBody() {
			std::memcpy(body.data(), &header, sizeof(udpPacketHeader<T>));
		}

		GameMessages GetNextSubPacketType() {
			bytesRead += sizeof(GameMessages);	//no temp var pls
			return *(GameMessages*)(body.data() + bytesRead - sizeof(GameMessages));
		}

		template<typename dataType>
		dataType* GetSubPacketPtr() {
			bytesRead += sizeof(dataType);
			if (bytesRead > body.size()) return nullptr;
			return (dataType*)(body.data() + bytesRead - sizeof(dataType));
		}

		GameMessages PeekNextPacketType() {
			if (bytesRead == body.size()) return GameMessages::None;
			return *(GameMessages*)(body.data() + bytesRead);
		}

		friend std::ostream& operator << (std::ostream& os, const udpPacket<T>& pac) {
			os << "ID: " << (int)pac.header.id << "  Size: " << pac.header.size << "\n";
			return os;
		}

		template<typename dataType>
		friend udpPacket<T>& operator << (udpPacket<T>& pac, const dataType& data) {

#ifdef _WIN32
			//static_assert(std::is_standard_layout<dataType>::value, "Data is too complex to be pushed at line: " + __LINE__);
			static_assert(std::is_standard_layout<dataType>::value, "Data is too complex to be pushed");
#else
			static_assert(std::is_standard_layout<dataType>::value, "Data is too complex to be pushed");
#endif
			const size_t offset = pac.body.size();	//prev size, the point to memcopy the data
			pac.body.resize(pac.body.size() + sizeof(dataType));	//resize preformance issue
			std::memcpy(pac.body.data() + offset, &data, sizeof(dataType));
			pac.header.size = pac.Size();

			return pac;
		}

		template<typename dataType>
		friend udpPacket<T>& operator >> (udpPacket<T>& pac, dataType& data) {

#ifdef _WIN32
			//static_assert(std::is_standard_layout<dataType>::value, "Data is too complex to be pushed at line: " + __LINE__);
			static_assert(std::is_standard_layout<dataType>::value, "Data is too complex to be pushed");
#else
			static_assert(std::is_standard_layout<dataType>::value, "Data is too complex to be pushed");
#endif
			const size_t offset = pac.body.size() - sizeof(dataType);	//offset from th back
			std::memcpy(&data, pac.body.data() + offset, sizeof(dataType));
			pac.body.resize(offset);
			pac.header.size = offset;

			return pac;
		}
	};


#ifdef SERVER_CONNECTION
	template<typename T>
	class ServerConnection;

	template<typename T>
	struct ownedPacket 
	{
		std::shared_ptr<ServerConnection<T>> remote = nullptr;
		packet<T> pkt;

		friend std::ostream& operator << (std::ostream& os, const ownedPacket<T>& pkt) {
			os << pkt;
			return os;
		}
	};

	template<typename T>
	class ServerUdpConnection;

	template<typename T>
	struct ownedUdpPacket
	{
		std::shared_ptr<ServerUdpConnection<T>> remote = nullptr;
		udpPacket<T> pkt;

		friend std::ostream& operator << (std::ostream& os, const ownedUdpPacket<T>& pkt) {
			os << pkt;
			return os;
		}
	};

#else
	template<typename T>
	struct ownedPacket
	{
		packet<T> pkt;

		friend std::ostream& operator << (std::ostream& os, const ownedPacket<T>& pkt) {
			os << pkt;
			return os;
		}
	};

	template<typename T>
	struct ownedUdpPacket
	{
		udpPacket<T> pkt;

		friend std::ostream& operator << (std::ostream& os, const ownedUdpPacket<T>& pkt) {
			os << pkt;
			return os;
		}
	};

#endif

}
