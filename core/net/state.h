#pragma once

#include <string>
#include <vector>
#include <memory>

#include <enet/enet.h>
#include <sig/signal_producer.h>

#include <modules/env/env_common.h>

#include "channel.h"

namespace net
{
	namespace msg
	{
		struct ConnectionInfo;
	}

	void RegisterPacketToTracker(const sig::SignalMetadata sigMeta, const sig::time_point& timeNow, const char * label);

	// A *very* unsafe way to store arbitrary signal data
	struct SignalData
	{
		// track signal producer counter
		uint32_t sentCounter = 0;
		// Signal state
		std::vector<uint8_t> data;
		// timestamp of last data
		sig::time_point timestamp;

		// Do we have signal data?
		bool is_valid() const { return !data.empty(); }

		// Telemetry data
		std::vector<sig::time_point> timePoints; // for both local and remote clients
		std::vector<uint32_t> packetIds; // for remote clients: store 1 per time point. For local clients: store the first
		std::vector<sig::time_point> timePointsAtAdapter; // this records the time when signals arrive at adapter
		
		std::string to_string() const;
		// update state based on newly arrived data
		void from_network(const uint8_t* data_in, int size, const sig::SignalProperties& props);
	};

	// Each client, on the server, needs a name, an address, and whatever signals are associated
	struct ClientInfo
	{
		std::string name;
		ENetAddress address = {0};
		std::vector<SignalData> signals;
		float time_offset_ms_ntp;

		
		std::string to_string() const;
		bool is_initialized() const { return address.port != 0; }
	};

	// the environment is represented with a set of signals
	struct EnvState
	{
		std::vector<SignalData> signals;

		EnvSceneState sceneState;
		EnvMusicState musicState;
		EnvTestState testState;
		std::vector<EnvUserState> userStates;

		// The music state isn't known until we ask the first client
		bool musicKnown = false;

		EnvState() {
			testState.mBody.payload = -1;			
		}

		std::string to_string() const;

		void reset_user_state(int user);
	};

	// the world is a collection of clients and the environment
	struct WorldState
	{
		// per-client state
		std::vector<ClientInfo> clients;
		EnvState environment;

		// update clients based on new connection
		void on_connection_info(const sig::SignalMetadata& header, const msg::ConnectionInfo& ci, int numUserSignals);
		void reset_user_state(int user) {
			environment.reset_user_state(user);
		}
		std::string to_string() const;
	};
}