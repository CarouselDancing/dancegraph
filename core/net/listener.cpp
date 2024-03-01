#define _WINSOCKAPI_    // stops windows.h including winsock.h
#include "listener.h"


#include <thread>
#include <filesystem>
#include <magic_enum/magic_enum.hpp>

#include "net.h"
#include "formatter.h"

#include "listener.h"

namespace net
{
	void ListenerState::handle_message(const uint8_t* data, int size, int channelId)
	{
		assert(size >= sizeof(sig::SignalMetadata));
		const auto& packetHeader = *(sig::SignalMetadata*)data;
		auto payload = data + sizeof(sig::SignalMetadata);
		auto payloadSize = size - sizeof(sig::SignalMetadata);

		const SignalType sigType = (SignalType)packetHeader.sigType;
		spdlog::trace("Got a message for channel {}/{}\n", magic_enum::enum_name(sigType), packetHeader.sigIdx);

		if (sigType == SignalType::Control)
		{
			// A client (that we care about) has just connected!
			//	  Set the client index in the (name,index) pairs
			if (packetHeader.sigIdx == (int)msg::ConnectionInfo::kControlSignal)
			{
				
				const auto& connectionInfo = *(msg::ConnectionInfo*)payload;
				const std::string clientName = connectionInfo.name.data();

				spdlog::info("Got a new Connection from {} {}", clientName, connectionInfo.address);

				auto it_found = std::find_if(clients.begin(), clients.end(), [&clientName](const auto& nameAndIndex) { return nameAndIndex.first == clientName; });
				assert(it_found != clients.end());
				it_found->second = packetHeader.userIdx;
			}
			// We just got our connection's response from the server, with info on client/signal indices
			else if (channelId == (int)msg::NewListenerConnectionResponse::kControlSignal)
			{
				const auto& packet = *(msg::NewListenerConnectionResponse*)payload;
				for (size_t i = 0; i < clients.size(); ++i)
					clients[i].second = packet.clientIndices[i];
				for (size_t i = 0; i < signals.size(); ++i)
					signals[i].second = packet.signalIndices[i];				
				net_id = packetHeader.userIdx;
				spdlog::info("Receiving ListenerConnectionResponse connection as user {}", net_id);
			}
			// We got a sync signal -- just bounce it back!
			else if (channelId == (int)msg::LatencyTelemetry::kControlSignal)
			{
				spdlog::info("Ignoring sync signals temporarily");
				
				auto time_now = sig::time_now();
				const auto& sync_packet = *(msg::LatencyTelemetry*)payload;
				net::send_control_msg(server_peer_per_channel[0], msg::LatencyTelemetry{ msg::LatencyTelemetry::SenderType::Listener, sync_packet.server_time }, int16_t(net_id), time_now,nullptr);
				
			}
		}
		else // Data signals from clients
		{
			// Sanity-check the incoming packet type to make sure we handle it
			if (!scene_runtime_data.CheckSignalType(packetHeader)) {
				spdlog::warn("Client Signal of unhandled type {}, idx {}. Do client and server configs match?", packetHeader.sigIdx, packetHeader.sigType);
				return;
			}

			const auto& sigCfg = scene_runtime_data.SignalConfigFromMetadata(packetHeader);
			// Record to state if needed
			int stateSize = 0;
			if (sigCfg.signalProperties.keepState)
			{
				cache.resize(sigCfg.signalProperties.stateFormMaxSize);
				stateSize = sigCfg.signalProperties.networkToState(payload, payloadSize, cache.data(), cache.size(), nullptr);
			}
			// By default (passthru) consumer data == network data
			const uint8_t* consumerDataPtr = payload;
			int consumerSize = payloadSize;
			// if signal is not passthru, we need to convert it before passing to consumer
			if (!sigCfg.signalProperties.isPassthru)
			{
				cache2.resize(sigCfg.signalProperties.consumerFormMaxSize);
				if (sigCfg.signalProperties.keepState) 
				{
					consumerSize = sigCfg.signalProperties.stateToConsumer(cache.data(), cache.size(), cache2.data(), cache2.size(), nullptr);
				}
				else 
				{
					consumerSize = sigCfg.signalProperties.networkToConsumer(payload, payloadSize, cache2.data(), cache2.size(), nullptr);
				}
				cache2.resize(consumerSize);
				consumerDataPtr = cache2.data();
			}

			// send to local consumer
			// TODO: below, time should really be network time (from the packet)
			for (auto& consumer : sigCfg.consumers) {
				spdlog::info("Info going out to consumer: {}", consumerSize);
				//consumer->fnProcessSignalData(consumerDataPtr, sigCfg.signalProperties.consumerSize, packetHeader);
				consumer->fnProcessSignalData(consumerDataPtr, consumerSize, packetHeader);
			}
		}
	}

	void ListenerState::tick()
	{
		// TODO: when server is down, wait a bit and try to reconnect
		static int counter = 0;
		++counter;
		ENetEvent event;
		for (int iHost = 0; iHost < hosts.size(); ++iHost)
		{
			while(enet_host_service(hosts[iHost], &event, 0) > 0)
			{
				spdlog::trace("LISTENER: Got a message!... ({})\n", counter);
				switch (event.type)
				{
				case ENET_EVENT_TYPE_CONNECT:
				{
					spdlog::info("A new client connected from {}", event.peer->address);
					event.peer->data = (void*)"Client information";
					break;
				}
				case ENET_EVENT_TYPE_RECEIVE:
				{
					spdlog::trace("A packet of length {} was received from {} on channel {}.\n", event.packet->dataLength, event.peer->address, event.channelID);
					handle_message(event.packet->data, event.packet->dataLength, event.channelID);
					/* Clean up the packet now that we're done using it. */
					enet_packet_destroy(event.packet);
					spdlog::trace("Received data packet dealt with and destroyed\n");
					break;
				}

				case ENET_EVENT_TYPE_DISCONNECT:
				{
					spdlog::info("%s disconnected.\n", event.peer->data);
					/* Reset the peer's client information. */
					event.peer->data = NULL;
				}
				}
			}

		}
	}

	void ListenerState::deinitialize()
	{
		for (auto peer : server_peer_per_channel)
			enet_peer_disconnect(peer, 0);
		server_peer_per_channel.resize(0);
		for (auto host : hosts)
			enet_host_destroy(host);
		hosts.resize(0);
	}

	bool ListenerState::initialize(cfg::Listener& cfg )
	{
		if (!scene_runtime_data.Initialize(cfg.scene, {}, {}, cfg.user_signal_consumers, cfg.include_ipc_consumers, false))
		{
			spdlog::error("Scene runtime data initialization failed {}\n", cfg.scene.c_str());
			return false;
		}

		auto num_user_signals = int(scene_runtime_data.user_signals.size());
		auto num_env_signals = int(scene_runtime_data.env_signals.size());

		start_time_point = sig::time_now();
		address = make_address(cfg.address);
		bool ok = create_hosts_multiport(hosts, address, num_user_signals, num_env_signals, false);
		if (!ok)
			return false;

		ENetAddress server_address = make_address(cfg.server_address);
		
		ENetEvent event;

		server_peer_per_channel.resize(ChannelNum(num_user_signals));
		if (!connect_to_host_multiport(server_peer_per_channel, hosts, server_address, num_env_signals))
			return false;

		// Connect to server, for each channel

		for (int iHost = 0; iHost < hosts.size(); ++iHost)
		{
			/* Wait up to 5 seconds for the connection attempt to succeed. */
			if (enet_host_service(hosts[iHost], &event, 5000) > 0 &&
				event.type == ENET_EVENT_TYPE_CONNECT)
			{
				spdlog::info("Connection to {} from host {} succeeded.\n", server_address, iHost);
			}
			else
			{
				/* Either the 5 seconds are up or a disconnect event was */
				/* received. Reset the peer in the event the 5 seconds   */
				/* had run out without any significant event.            */
				for (auto& peer_server : server_peer_per_channel)
					if (peer_server != nullptr)
						enet_peer_reset(peer_server);
				spdlog::error("Connection to {}:{} failed.\n", cfg.server_address.ip.c_str(), cfg.server_address.port);
				return false;
			}

		}

		clients.reserve(cfg.clients.size());
		signals.reserve(cfg.signals.size());

		msg::NewListenerConnection m;
		m.address = address;

		m.portcount = hosts.size();
		
		for (int i = 0; i < hosts.size(); i++) {
			m.ports[i] = address.port + i;
		}

		if (cfg.clients.size() == 0) {
			spdlog::info(  "No clients found");
		}
		strcpy(m.name.data(), cfg.username.c_str());
	
		for (const auto& client : cfg.clients)
		{
			spdlog::info("Listening to client {}\n", client);
			m.clients.Add(client);
			clients.emplace_back(client, -1);
		}

		if (cfg.signals.size() == 0) {
			spdlog::info("No signals found");
		}
		for (const auto& signal : cfg.signals)
		{
			spdlog::info("Listening to signal {}\n", signal);
			m.signals.Add(signal);
			signals.emplace_back(signal, -1);
		}
		
		spdlog::info("NewListenerConnection with {} clients, {} signals\n", m.clients.NumNames(), m.signals.NumNames());

		net::send_control_msg(server_peer_per_channel[0], m, -1, sig::time_now(), nullptr);

		is_initialized = true;
		return true;
	}

	void ListenerState::run(cfg::Listener& cfg, bool(*callback_fn)())
	{
		if (initialize(cfg))
		{
			bool run = true;
			while (run) {
				tick();
				if (callback_fn) {
					run = callback_fn();
				}

			}
		}
		deinitialize();
	}
}