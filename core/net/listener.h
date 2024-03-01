#pragma once

#include <chrono>

#include <sig/signal_consumer.h>
#include <sig/signal_producer.h>

#include "message.h"
#include "state.h"
#include "config_runtime.h"

namespace net
{
	struct ListenerState
	{
	public:
		void run(cfg::Listener& cfg, bool(*callback_fn)() = nullptr);
		bool initialize(cfg::Listener& cfg);
		void deinitialize();
		virtual void tick();

		// derived classes can show their own gui over the listener
		virtual void on_gui() {};

	protected:

		void handle_message(const uint8_t* data, int size, int channelId);

		// signal from state/network 
		void _process_incoming_signal(SignalType channelType, std::vector<net::SignalData>& signalData);

	protected:
		bool is_initialized = false;
		
		cfg::Listener config;
		SceneRuntimeData scene_runtime_data;

		int net_id = -1;
		ENetAddress address;

		// Which clients we're interested in? names and indices retrieved from the server 
		std::vector<std::pair<std::string,int>> clients;
		// Which signals we're interested in?
		std::vector< std::pair<std::string,int>> signals;

		// Server peer per channel (control/env/sig0/sig1/...)
		std::vector<ENetPeer*> server_peer_per_channel;
		// A host per channel (control/env/sig0/sig1/...)
		std::vector<ENetHost*> hosts;

		sig::time_point start_time_point;

		std::vector<uint8_t> cache;
		std::vector<uint8_t> cache2;
	};
}