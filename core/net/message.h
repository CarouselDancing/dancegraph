#pragma once

//#define ENABLE_TEST_SIGNAL

#include <chrono>
#include <random>
#include <array>
#include <bitset>

#include <spdlog/spdlog.h>

#include <core/common/platform.h>
#include <enet/enet.h>

#include "net.h"
#include "formatter.h"
#include "channel.h"

#include <iostream>
#include <string>
#include <sstream>


#include "stats.h"

using time_point = std::chrono::time_point<std::chrono::system_clock>;


constexpr int MUSIC_TRACKNAME_MAX_SIZE = 1024;
constexpr int SCENENAME_MAX_SIZE = 1024;
constexpr int AVATARDESC_MAX_SIZE = 1024;
constexpr int USERNAME_MAX_SIZE = 1024;


namespace net
{
	// Helper struct for fixed-size messages that contain a list of strings
	template<int N>
	struct NameList
	{
		std::array<char, N> data;

		NameList() { data.fill(0); }

		bool Add(const std::string& name)
		{

			auto i = NumBytesWritten();
			auto numBytes = name.size() + 1;
			if ((i + numBytes) >= N)
				return false;
			std::copy(name.begin(), name.end(), data.begin() + i);
			data[0] += 1;
			return true;
		}

		int NumNames() const { return data[0]; }

		const char* Name(int idx) const
		{
			auto numNames = NumNames();
			auto it = data.begin() + 1;
			for (int i = 0; i < idx; ++i)
			{
				it = std::find(it, data.end(), '\0');
				assert(it != data.end());
				++it;
			}
			return &*it;
		}

		// Number of bytes used by all names so far 
		size_t NumBytesWritten() const
		{
			auto numNames = NumNames();
			auto it = data.begin() + 1;
			for (int i = 0; i < numNames; ++i)
			{
				it = std::find(it, data.end(), '\0');
				assert(it != data.end());
				++it;
			}
			return std::distance(data.begin(), it);
		}
	};

	namespace msg
	{
		// Currently formatted as a MAC address
		struct ClientID {
			std::array<char, 8> value;


			bool operator == (const ClientID& other) const { return value == other.value; }
			bool operator != (const ClientID& other) const { return !(value == other.value); }
			ClientID() { value.fill(0); }

			void populate(int portStart);

			std::string toString() const {
				std::stringstream ss;
				for (auto& c : value) {
					ss << std::hex << static_cast<int> (c & 0xff) << ":";
				}
				return ss.str();
			}
		};


		// Registering each port and the signal index from every given client
		struct NewConnection
		{
			static const ControlSignal kControlSignal = ControlSignal::NewConnection;
			ENetAddress address = { 0 };
			std::array<char, 44> name;

			float time_offset_ms_ntp = 0.0f;

			ClientID id;
			uint32_t pad; // to pad the struct to 8 bytes
		};


		// Sent by the client from a specific network port to suggest using it as the signal port from now on
		struct PortOverrideInformation
		{
			static const ControlSignal kControlSignal = ControlSignal::PortOverrideInformation;
			ClientID id;

			uint8_t sigType = 0;		//!< Type of signal: control, client or environment
			uint8_t sigIdx = 0;
			const uint8_t padding[6] = { 99,99,99,99,99,99 };   // 48 bits of advertising space for sale to anyone who wants it

		};


		// Broadcast to everybody from server
		struct ConnectionInfo
		{
			static const ControlSignal kControlSignal = ControlSignal::ConnectionInfo;
			std::array<char, 44> name;
			ENetAddress address = { 0 };
			ClientID id;
			float time_offset_ms_ntp = 0.0f;
			uint32_t pad;
		};

		// Send from a newly-connected listener client to the server
		struct NewListenerConnection
		{
			static const ControlSignal kControlSignal = ControlSignal::NewListenerConnection;

			ENetAddress address = { 0 };
			std::array<char, 44> name;

			ClientID id;

			uint32_t portcount;
			std::array<uint32_t, 31> ports; // a list of UDP ports that this connection will be using, in order

			// Clients to listen to
			NameList<1024> clients;

			// Signals to listen to
			NameList<1024> signals;

			// A Client Identifier

		};

		// Server -> listener, straight after connection request
		struct NewListenerConnectionResponse
		{
			static const ControlSignal kControlSignal = ControlSignal::NewListenerConnectionResponse;

			ClientID id;

			std::array<int, 64> clientIndices;
			std::array<int, 64> signalIndices;


		};

		// Server <-> listener/client, for syncing purposes
		struct LatencyTelemetry
		{
			enum class SenderType : uint8_t
			{
				Server = 0,
				Client,
				Listener
			};

			static const ControlSignal kControlSignal = ControlSignal::LatencyTelemetry;

			SenderType sender_type;
			sig::time_point server_time;
		};

		// Server -> listener/client, to inform them of their latency to the server
		struct Delay
		{
			static const ControlSignal kControlSignal = ControlSignal::Delay;

			// how many milliseconds do we need to reach the server, on average?
			uint32_t ms;
		};

		struct TelemetryRequest
		{
			static const ControlSignal kControlSignal = ControlSignal::TelemetryRequest;

			// TODO: Add more info here if needed to filter specific data, like time range, which signals, which users, etc
			uint64_t pad;
		};

		struct TelemetryClientData
		{
			static const ControlSignal kControlSignal = ControlSignal::TelemetryClientData;
			static const int MAX_COUNT = 1024;

			uint8_t sigType;
			uint8_t sigIdx;
			uint16_t count; // should be <= MAX_COUNT
			uint32_t packetIdOffset;
			std::array<sig::time_point, MAX_COUNT> timePoints;
			std::array<sig::time_point, MAX_COUNT> timePointsAtAdapter;
		};

		struct TelemetryOtherClientData
		{
			static const ControlSignal kControlSignal = ControlSignal::TelemetryOtherClientData;
			static const int MAX_COUNT = 1024;

			uint8_t sigType;
			uint8_t sigIdx;
			uint16_t count; // should be <= MAX_COUNT
			int32_t clientId;
			std::array<sig::time_point, MAX_COUNT> timePoints;
			std::array<sig::time_point, MAX_COUNT> timePointsAtAdapter;
			std::array < uint32_t, MAX_COUNT> packetIds;
		};

		struct PingRequest
		{
			static const ControlSignal kControlSignal = ControlSignal::PingRequest;
			uint64_t pad;
		};

		struct PingReply
		{
			static const ControlSignal kControlSignal = ControlSignal::PingReply;
			uint64_t pad;
		};

	}




	// Wrapper struct for signal metadata followed by a fixed control message
	template<class T>
	struct ControlMessageWithHeader
	{
		static inline uint32_t counter = 0;
		sig::SignalMetadata header;
		T payload;

		ControlMessageWithHeader(const T& data, const sig::time_point& time, int16_t userIdx, uint8_t sigIdx, uint8_t sigType) :header{ time, counter++, userIdx, sigIdx, sigType }, payload(data) {}

		// make sure T size is a multiple of 8 bytes, as the header is aligned to 8 bytes, and otherwise we have some data transfer issues
		static_assert((sizeof(T) % 8) == 0);
	};


	// Helper for sending a control message
	template<class T>
	inline sig::SignalMetadata send_control_msg(ENetPeer* peer, const T& payload, int16_t userIdx, const sig::time_point& time, ServerStats* stats)
	{
		ControlMessageWithHeader<T> m(payload, time, userIdx, (uint8_t)T::kControlSignal, (uint8_t)SignalType::Control);
		spdlog::trace("Sending message {} of size {} to peer {} as from user {}\n", magic_enum::enum_name(T::kControlSignal), sizeof(m), peer != nullptr ? peer->address : ENetAddress{ 0 }, userIdx);

		if (peer != nullptr)
		{
			auto packet = make_packet(m, true);
			auto err = enet_peer_send(peer, uint8_t(T::kControlSignal), packet);
			if (err != 0)
				spdlog::error("send_msg error {} while sending message {} of size {} to peer {} as from user {}", err, magic_enum::enum_name(T::kControlSignal), sizeof(m), peer != nullptr ? peer->address : ENetAddress{ 0 }, userIdx);
			if (stats != nullptr)
				stats->OnSentPacket(m.header, sizeof(m));
		}
		else
			spdlog::info("\t...but peer is null!\n");
		return m.header;
	}

	// Wrapper struct for signal metadata followed by a fixed control message
	template<class T>
	struct EnvironmentMessageWithHeader
	{
		static inline uint32_t counter = 0;
		sig::SignalMetadata header;
		T payload;

		EnvironmentMessageWithHeader(const T& data, const sig::time_point& time, int16_t userIdx, uint8_t sigIdx, uint8_t sigType) :header{ time, counter++, userIdx, sigIdx, sigType }, payload(data) {}

		// make sure T size is a multiple of 8 bytes, as the header is aligned to 8 bytes, and otherwise we have some data transfer issues
		static_assert((sizeof(T) % 8) == 0);
	};


	/*
	template<class T>
	inline sig::SignalMetadata send_env_msg(ENetPeer* peer, const T& payload, int16_t userIdx, const sig::time_point& time, ServerStats* stats)
	{
		EnvironmentMessageWithHeader<T> m(payload, time, userIdx, (uint8_t)T::kEnvSignal, (uint8_t)SignalType::Environment);
		spdlog::info("Sending message {} of size {} to peer {} as from user {}\n", magic_enum::enum_name(T::kEnvSignal), sizeof(m), peer != nullptr ? peer->address : ENetAddress{ 0 }, userIdx);
		if (peer != nullptr)
		{
			auto packet = make_packet(m, true);
			auto err = enet_peer_send(peer, uint8_t(T::kEnvSignal), packet);
			if (err != 0)
				spdlog::error("send_msg error while sending message {} of size {} to peer {} as from user {}", magic_enum::enum_name(T::kEnvSignal), sizeof(m), peer != nullptr ? peer->address : ENetAddress{ 0 }, userIdx);
			if (stats != nullptr)
				stats->OnSentPacket(m.header, sizeof(m));
		}
		else
			spdlog::info("\t...but peer is null!\n");
		return m.header;
	}
*/


	template<class T>
	inline sig::SignalMetadata send_env_msg(ENetPeer* peer, const T& payload, int16_t userIdx, const sig::time_point& time, ServerStats* stats)
	{
		uint8_t envSignalIndex = 0; // we have only one of those, e.g. env/v1.0
		EnvironmentMessageWithHeader<T> m(payload, time, userIdx, (uint8_t)0, (uint8_t)SignalType::Environment);
		spdlog::info("Sending message {} of size {} to peer {} as from user {}\n", (uint8_t)envSignalIndex, sizeof(m), peer != nullptr ? peer->address : ENetAddress{ 0 }, userIdx);
		if (peer != nullptr)
		{
			auto packet = make_packet(m, true);
			const int envSignalsChannel = 0; // all env signals go through the same channel!
			auto err = enet_peer_send(peer, (uint8_t)envSignalsChannel, packet);
			if (err != 0)
				spdlog::error("send_msg error while sending message {} of size {} to peer {} as from user {}", (uint8_t)payload.signalID, sizeof(m), peer != nullptr ? peer->address : ENetAddress{ 0 }, userIdx);
			if (stats != nullptr)
				stats->OnSentPacket(m.header, sizeof(m));
		}
		else
			spdlog::info("\t...but peer is null!\n");
		return m.header;
	}


}