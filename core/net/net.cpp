#include "net.h"

// Link to ws2_32.lib
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>

#include "channel.h"
#include "config.h"

static void(*g_logCallback)(const char* msg) = [](const char* msg) {fprintf(stderr, "%s\n", msg);};
void set_log_callback( void(*fn)(const char*))
{
	g_logCallback = fn;
}

void log(const char* msg)
{
	(*g_logCallback)(msg);
}

void exit_with_error(const char* format, ...)
{
	va_list argptr;
	va_start(argptr, format);
#if 1
	static char buf[1024];
	vsprintf(buf, format, argptr);
	spdlog::error("{}", buf);
#else
	vfprintf(stderr, format, argptr);
#endif
	va_end(argptr);
	exit(EXIT_FAILURE);
}

namespace net
{
	ENetAddress make_address(const char* ip, int port)
	{
		ENetAddress a{ 0 };
		enet_address_set_host(&a, ip);
		a.port = port;
		return a;
	}

	ENetAddress make_address(const Address& address)
	{
		return make_address(address.ip.c_str(), address.port);
	}

	ENetAddress make_server_address(const Address & address) {		
		ENetAddress a{ 0 };
		a.host = ENET_HOST_ANY;
		a.port = address.port;
		return a;
	}

	ENetHost* create_host(const ENetAddress* address, int maxChannels, bool is_server = false)
	{
		auto numOutgoing = is_server ? kMaxClients : 1;
		
		if (false) {
			ENetHost* host = enet_host_create(0,
				numOutgoing,
				maxChannels,
				0,
				0);
			return host;
		}
		else {
			ENetHost* host = enet_host_create(address,
				numOutgoing,
				maxChannels,
				0 /* assume any amount of incoming bandwidth */,
				0 /* assume any amount of outgoing bandwidth */);
			return host;
		}
		
	}


	bool create_hosts_multiport(std::vector<ENetHost*>& hosts, const ENetAddress& address, int numUserSignals, int numEnvSignals, bool is_server)
	{
		auto curAddress = address;
		int numPorts = numUserSignals + 2;
		hosts.resize(numPorts);

		for (int i = 0; i < numPorts; ++i)
		{
			// We're now sending control signals over every port, so the channel count has to be the biggest possible

			int maxChannels = kNumControlSignals > numEnvSignals ? kNumControlSignals : numEnvSignals;

			spdlog::info("Creating host, port {}", curAddress.port);
			hosts[i] = create_host(&curAddress, maxChannels, is_server);
			
			if (hosts[i] == nullptr)
			{
				spdlog::error("An error occurred while trying to create an ENet client, at port {}", curAddress.port);
				for (int j = 0; j < i; ++j)
					enet_host_destroy(hosts[j]);
				return false;
			}
			
			++curAddress.port;
		}
		return true;
	}

	ENetPeer* connect_to_host(ENetHost* host, const ENetAddress& address, int maxChannels)
	{
		/* Initiate the connection, allocating the two channels 0 and 1. */
		auto peer = enet_host_connect(host, &address, maxChannels, 0);
		if (peer == nullptr)
			spdlog::error("No available peers for initiating an ENet connection.\n");
		return peer;
	}

	bool connect_to_host_multiport(std::vector<ENetPeer*>& peers, const std::vector<ENetHost*>& hosts, const ENetAddress& address, int maxEnvSignals)
	{
		peers.resize(hosts.size(), nullptr);
		auto curAddress = address;
		for (size_t i = 0; i < peers.size(); ++i)
		{
			int maxChannels = kNumControlSignals > maxEnvSignals ? kNumControlSignals : maxEnvSignals;

			auto peer = enet_host_connect(hosts[i], &curAddress, maxChannels, 0);

			if (peer == nullptr)
			{
				spdlog::error("No available peers for initiating an ENet connection.\n");
				peers.resize(i); // remove the other nullptr peers (so that cleanup won't try to free null pointers)
				return false;
			}
			peers[i] = peer;
			curAddress.port++;
		}
		return true;
	}

	bool initialize()
	{
		if (enet_initialize() != 0)
		{
			spdlog::error("An error occurred while initializing ENet.\n");
			return false;
		}
		atexit(enet_deinitialize);
		return true;
	}

	std::string get_local_ip_address()
	{
        WSADATA wsaData;
        WSAStartup(0x202, &wsaData);

        std::string ip;
        int sock = socket(PF_INET, SOCK_DGRAM, 0);
        sockaddr_in loopback;

        if (sock != -1)
        {
            std::memset(&loopback, 0, sizeof(loopback));
            loopback.sin_family = AF_INET;
            loopback.sin_addr.s_addr = 1337;   // can be any IP address
            loopback.sin_port = htons(9);      // using debug port

            if (connect(sock, reinterpret_cast<sockaddr*>(&loopback), sizeof(loopback)) != -1) 
            {
                socklen_t addrlen = sizeof(loopback);
                if (getsockname(sock, reinterpret_cast<sockaddr*>(&loopback), &addrlen) != -1)
                {
                    char buf[INET_ADDRSTRLEN];
                    if (inet_ntop(AF_INET, &loopback.sin_addr, buf, INET_ADDRSTRLEN) != 0x0)
                        ip = buf;
                }
            }
            closesocket(sock);
        }

        WSACleanup();
        return ip;
	}
}
#elif __linux__

#include <string>
#include <cstring>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace net
{

	const char arbitrary_sa[14] = {9, 0, 57, 5, 0,0,0,0,0,0,0,0,0};
	
	std::string get_local_ip_address() {
	
		std::string ip;
		int sock = socket(PF_INET, SOCK_DGRAM, 0);
//	sockaddr_in loopback;
		sockaddr loopback;
		
		if (sock != -1)
		{
			std::memset(&loopback, 0, sizeof(loopback));
			loopback.sa_family = AF_INET;
			memcpy(loopback.sa_data, arbitrary_sa, 14);

			if (connect(sock, reinterpret_cast<sockaddr*>(&loopback), sizeof(loopback)) != -1) 
				{
					socklen_t addrlen = sizeof(loopback);
					if (getsockname(sock, reinterpret_cast<sockaddr*>(&loopback), &addrlen) != -1)
					{
						char buf[INET_ADDRSTRLEN];
						if (inet_ntop(AF_INET, &(loopback.sa_data[2]), buf, INET_ADDRSTRLEN) != 0x0)
							ip = buf;
					}
				}
			close(sock);
		}

		return ip;
	}
}

#endif // WIN32 else __linux__
