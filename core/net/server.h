#pragma once

#include <unordered_map>
#include <chrono>
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <bitset>
#include <functional>

#include "net.h"
#include "state.h"
#include "channel.h"
#include "config_runtime.h"

#include "sync.h"
#include "telemetry.h"

#include "pubsub/pubsub.h"
#include "stats.h"

namespace std // any extension of std namespace is UB except from specialising templates
{
	// Provide a hash function as that's needed for the unordered_map<ENetAddress, ...>
	template <>
	struct hash<ENetAddress>
	{
		size_t operator()(const ENetAddress& addr) const
		{
			const uint16_t* ptr = addr.host.u.Word;
			constexpr int kNumShorts = sizeof(ENetAddress) / sizeof(uint16_t);
			static_assert(sizeof(ENetAddress) == sizeof(uint16_t) * kNumShorts);
			size_t h = std::hash<uint16_t>()(ptr[0]);
			for (int i = 1; i < kNumShorts; ++i)
				h ^= std::hash<uint16_t>()(ptr[i]);
			return h;
		};
	};

	// Provide an equals operator as that's also needed for the unordered_map<ENetAddress, ...>
	inline bool operator ==(const ENetAddress& lhs, const ENetAddress& rhs)
	{
		return lhs.port == rhs.port && 
			   lhs.sin6_scope_id == rhs.sin6_scope_id && 
			   memcmp(lhs.host.u.Word, rhs.host.u.Word, sizeof(lhs.host)) == 0;
	}
}

namespace net
{
	// Network-specific data per client
	struct ClientDataInServer
	{
		std::string name;
		// common state in client/server
		std::vector<ENetPeer*> peer_per_channel;
		sig::time_point last_seen_time_point;
		
		bool ping_reminder;
		

		sync_t sync_info;
		msg::ClientID unique_id;
		float time_offset_ms_ntp;

		TelemetryData telemetryData;

		// time_point connection_time;
		// int bytes_received;
		// int bytes_sent;

		void disconnect();
		bool is_connected() const { return !peer_per_channel.empty(); }
	};

	// Network-specific data per listener
	struct ListenerDataInServer
	{

		std::string name;
		// common state in client/server
		std::vector<ENetPeer*> peer_per_channel;
		sig::time_point last_seen_time_point;
		sync_t sync_info;

		ENetAddress address;

		// Names of clients : unresolved (listener might connect and request client data before client connects)
		std::set<std::string> unresolvedClients;
		
		// Indices of clients : resolved
		std::bitset<64> clientMask;

		// Signals to listen to
		std::bitset<64> signalMask;
	};


	class ServerState
	{
	public:
		virtual ~ServerState() = default;
		bool initialize(const cfg::Server& cfg);
		void deinitialize();
		void run(const cfg::Server& cfg, bool(*callback_fn)() = nullptr);
		virtual void tick();
		
		// derived classes can show their own gui over the server
		virtual void on_gui() {};

		void request_telemetry(std::function<void(const TelemetryData& tdLatest)> fnTcd);

		float get_time_offset_ms_ntp() const {return time_offset_ms_ntp;}

		sig::time_point start_time() const {return start_time_point;}

	protected:

		template<class T>
		sig::SignalMetadata broadcast_control_msg(const T& payload, int userIdx, bool skipPeer);


		// Temporarily cleared until we find a better way
		template<class T>
		sig::SignalMetadata send_control_msg(int tgtUser, const T& payload, int senderIdx, const sig::time_point& time_point);
		
		void send_initial_worldstate(int userIdx);

		void initialize_environment_user_state(int userIdx, const net::msg::NewConnection& newConnection);

		void handle_control_signal(ControlSignal channel, const sig::SignalMetadata& header, const void * packetData, int packetLength, ENetPeer* peer);
		void disconnect(int userIdx);
		void check_disconnected();

		// Returns the number of times enet_peer_send was used (since that relates to packet deallocation and statistics)
		int broadcast_signal(int userIdx, int signalIdx, enet_uint8 channelID, ENetPacket* packet);

		// Return the relative-to-server time offset of a client
		// Add this value to a time point of a client to get a server-aligned time
		std::chrono::microseconds ClientNtpTimeOffset( int iClient) const;

	protected:
		
		cfg::Server config;
		SceneRuntimeData scene_runtime_data;

		bool env_check_authorization(uint32_t, uint32_t);

		ENetAddress server_address;

		// Server-specific state for clients
		std::vector<ClientDataInServer> clients;
		// Server-specific state for listeners
		std::vector<ListenerDataInServer> listeners;
		// World state (replicated in clients)
		WorldState world;
		// When did the server start running
		sig::time_point start_time_point;
		// Time offset to NTP server
		float time_offset_ms_ntp = std::numeric_limits<float>::infinity();
		// A host per channel (control/env/sig0/sig1/...)
		std::vector<ENetHost*> hosts;

		// temp storage for messages
		std::vector<uint8_t> msgCache;

		// When we update telemetry data and this callback is valid, call it
		std::function<void(const TelemetryData& tdLatest)> telemetryDataHandler;

		// Currently, Clients and Listeners are indexed using ints
		pubsub::SubscriptionManager<int> sub_manager;
		// Named list of signals, for use in a lookup with the SubscriptionManager
		std::vector<std::string> signal_list;
		bool is_initialized = false;

		ServerStats stats;
	};
}