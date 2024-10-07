#pragma once

#include <chrono>

#include <BS_thread_pool.hpp>

#include <sig/signal_consumer.h>
#include <sig/signal_producer.h>

#include "message.h"
#include "state.h"
#include "config_master.h"
#include "config_runtime.h"

#include "sync.h"


namespace net
{
	struct ClientState
	{
	public:

		// callback_fn is a function to allow for polite quitting due to user interaction.
		// Returns true if the 
		void run(const cfg::Client& cfg, bool (*callback_fn)() = nullptr);
		bool initialize(const cfg::Client& cfg);
		void deinitialize();
		virtual void tick();

		void register_signal_callback(void(*cb)(const uint8_t*, int, const sig::SignalMetadata&)) { fn_process_signal_data = cb; }
		void send_signal(const uint8_t* stream, int numBytes, const sig::SignalMetadata& sigMeta);
		void record_adapter_signal_telemetry(const sig::SignalMetadata& sigMeta, const sig::time_point& tp);

		int16_t UserIdx() const { return userIdx; }

		// derived classes can show their own gui over the server
		virtual void on_gui() {};

	protected:
		template<class T>
		sig::SignalMetadata send_control_msg(const T& payload, const sig::time_point& time_point, SignalType channel = SignalType::Control, uint8_t sigIdx = 0)
		{
			if (channel != SignalType::Control || sigIdx != 0)
				spdlog::info("Attempting control message {} send over nonstandard peer {}->{}/{}", magic_enum::enum_name(T::kControlSignal),
					server_peer_per_channel[sigIdx + (int)channel]->host->address, 
					server_peer_per_channel[sigIdx + (int)channel]->address,
					sigIdx);
			return net::send_control_msg(server_peer_per_channel[sigIdx + (int) channel], payload, userIdx, time_point, nullptr);
		}

		void handle_message(const uint8_t* data, int size, int channelId);

		// signal from hw to state/network/local consumer
		void _process_outgoing_signal(SignalType signalType, std::vector<net::SignalData>& signalData);
		// signal from state/network 
		void _process_incoming_signal(SignalType signalType, std::vector<net::SignalData>& signalData);

		void _process_transformer_output(SignalType signalType, std::vector<net::SignalData>& signalData);

		// Updates the local user index
		void update_local_user_index(uint16_t idx);

		

	protected:

		// The config we used to initialise this client
		cfg::Client config;

		BS::thread_pool signal_producer_thread_pool;
		BS::thread_pool signal_transformer_thread_pool;
		std::vector<std::future<int>> producer_futures; // results from producer calls (num bytes read)
		std::vector<std::future<int>> env_producer_futures; // results from producer calls (num bytes read)
		std::vector<std::future<int>> transformer_futures; // Results from producer calls to transformers. This suggests a limit of one transformer per signal (i.e. we're not chaining transformers)
		SceneRuntimeData scene_runtime_data;

		// An identifier so we know the server is talking to us
		msg::ClientID uniqueID;

		// Server peer per channel (control/env/sig0/sig1/...)
		std::vector<ENetPeer*> server_peer_per_channel;
		// A host per channel (control/env/sig0/sig1/...)
		std::vector<ENetHost*> hosts;
		// When did the client start running
		sig::time_point start_time_point;
		// Time offset to NTP server
		float time_offset_ms_ntp = std::numeric_limits<float>::infinity();

		// what's our address? 
		ENetAddress address;
		// what's our index?
		std::atomic<int16_t> userIdx = -1;
		WorldState world;
		bool is_initialized = false;

		// Optional callback for consumer signal processing
		void(*fn_process_signal_data)(const uint8_t* stream, int numBytes, const sig::SignalMetadata& sigMeta) = nullptr;

		// temp caches for multithreaded (MT) messages
		std::vector<std::vector<uint8_t>> caches_mt;

		// Need another set for transformers
		std::vector<std::vector<uint8_t>> caches_xform_mt;
		
		// Secondary caches
		std::vector<std::vector<uint8_t>> caches2_mt;
		std::vector<std::vector<uint8_t>> caches2_xform_mt;

		// And for environment signals
		std::vector<std::vector<uint8_t>> caches_env_mt;
		std::vector<std::vector<uint8_t>> caches_env_xform_mt;
		std::vector<std::vector<uint8_t>> caches2_env_mt;
		std::vector<std::vector<uint8_t>> caches2_env_xform_mt;

		std::vector<sig::time_point> time_points_mt;

		// caches used in single-threaded operations
		std::vector<uint8_t> cache_st;
		std::vector<uint8_t> cache2_st;
		
		// Transformed signals include metadata
		bool transform_metadata = false;

	};
}