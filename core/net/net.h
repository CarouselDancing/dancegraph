#pragma once

#include <string>
#include <vector>
#include <chrono>

#include <enet/enet.h>

void set_log_callback(void(*fn)(const char*));
void log(const char* msg);
void exit_with_error(const char* format, ...);

namespace net
{
	enum PeerType {
		Unknown = 0,
		Server,
		Client,
		Listener
	};

	struct Address;

	constexpr int kMaxClients = 64;

	[[nodiscard]] bool initialize();
	std::string get_local_ip_address();
	
	ENetAddress make_address(const char* ip, int port);
	ENetAddress make_address(const Address& address);
	ENetAddress make_server_address(const Address& address);
	ENetHost* create_host(const ENetAddress* address, int maxChannels, bool is_server);
	bool create_hosts_multiport(std::vector<ENetHost*>& hosts, const ENetAddress& address, int numUserSignals, int numEnvSignals, bool is_server);
	ENetPeer* connect_to_host(ENetHost* host, const ENetAddress& address, int maxChannels);
	// return false if any of the generated peers are null
	bool connect_to_host_multiport(std::vector<ENetPeer*>& peers, const std::vector<ENetHost*>& hosts, const ENetAddress& address, int maxEnvSignals);


	//void initialize_multiport_peers(std::vector<ENetPeer*>& peers, ENetHost* host, int numPorts);

	inline ENetPacket* make_packet(const void * msg_data, int msg_size, bool reliable)
	{
		return enet_packet_create(msg_data, msg_size, reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
	}

	inline ENetPacket* make_packet(const std::vector<uint8_t>& data, bool reliable)
	{
		return make_packet(data.data(), data.size(), reliable);
	}

	template<class T>
	inline ENetPacket* make_packet(const T& data, bool reliable)
	{
		return make_packet(&data, sizeof(T), reliable);
	}
}
