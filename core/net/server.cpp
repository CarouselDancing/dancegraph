#include <cstdlib>
#include <filesystem>
#include <future>

#include "formatter.h"

#include "server.h"
#include "net.h"
#include "message.h"
#include "config_master.h"

#include <magic_enum/magic_enum.hpp>

#include <sig/signal_producer.h>

#include <modules/env/env_common.h>


namespace net
{
	std::chrono::microseconds ServerState::ClientNtpTimeOffset(int iClient) const
	{
		return std::chrono::microseconds(int64_t(1000 * (clients[iClient].time_offset_ms_ntp - time_offset_ms_ntp)));
	}

	template<class T>
	sig::SignalMetadata ServerState::broadcast_control_msg(const T& payload, int senderIdx, bool skipPeer)
	{
		const int channelIdx = 0;
		spdlog::info("Broadcasting message {} as from user {}\n", magic_enum::enum_name(T::kControlSignal), senderIdx);
		ControlMessageWithHeader<T> m( payload, sig::time_now(), senderIdx, channelIdx, uint8_t(SignalType::Control));
		auto packet = make_packet(m, true);
		auto host = hosts[channelIdx];
		if (skipPeer)
		{
			auto peer = clients[senderIdx].peer_per_channel[channelIdx];
			enet_host_broadcast2(host, uint8_t(T::kControlSignal), packet, peer);

		}
		else
			enet_host_broadcast(host, uint8_t(T::kControlSignal), packet);
		// This should take into account if a peer is disconnected, so it could be a bit more accurate, but control messages are small
		auto numPeersSent = host->peerCount - (skipPeer ? 1 : 0);
		stats.OnSentPacket(m.header, sizeof(m) * numPeersSent);
		return m.header;
	}

	void ClientDataInServer::disconnect()
	{
		for (auto peer : peer_per_channel)
			if (peer != nullptr)
				enet_peer_reset(peer);
		peer_per_channel.resize(0);

	}

	void ServerState::disconnect(int userIdx)
	{
		spdlog::info("Disconnecting user {}", userIdx);


		// clear all data
		clients[userIdx].disconnect();
		world.clients[userIdx] = {};
		sub_manager.Unsubscribe(userIdx, pubsub::SubType::Client);

		// update the user state and send it to all clients
		// WARNING: disconnected client won't get this. 
		assert(world.environment.userStates.size() > userIdx);
		{
			auto& userState = world.environment.userStates[userIdx];
			//std::string s = userState.mBody.userName;
			std::string s = std::string("");

			strcpy(userState.mBody.avatarType, s.c_str());
			strcpy(userState.mBody.avatarParams, s.c_str());
			strcpy(userState.mBody.userName, s.c_str());			
			
			// Set the initial state to 'active'
			userState.mBody.isActive = 1;

			for(int iClient = 0; iClient < clients.size(); ++iClient)
				if(clients[iClient].is_connected())
					net::send_env_msg<EnvUserState>(clients[iClient].peer_per_channel[(int)SignalType::Environment],
						userState,
						userIdx,
						sig::time_now(),
						&stats);
		}
	}

	void ServerState::check_disconnected()
	{
		// Check last alive time: if more than our timeout value, mark as disconnected
		auto cur_time_point = sig::time_now();
		for (int i = 0; i < clients.size(); ++i)
		{
			auto& client = clients[i];
			if (!client.peer_per_channel.empty())
			{
				auto elapsed = sig::duration_sec(client.last_seen_time_point, cur_time_point);

				// We've received a timely signal, so reset the pinger
				if (elapsed <= cfg::Root::instance().networking.connectionPing) {
					client.ping_reminder = false;
				}
				// If the client is about to timeout, and we've not sent a ping request about it, send a ping request
				// Maybe the client is still there, but doesn't have much to say
				else if (client.ping_reminder == false)
				{
					const msg::PingRequest ci;
					spdlog::debug("Sending ping request to possibly moribund client {}", i);
					net::send_control_msg(client.peer_per_channel[(int)SignalType::Control], ci, i, sig::time_now(), &stats);
					
					// A flag to tell us not to send more pings until we get some sort of reply
					client.ping_reminder = true;
				}


				if (elapsed > cfg::Root::instance().networking.connectionTimeout)
				{
					spdlog::info("Disconnecting user {} ({}) due to inactivity for >= {} sec", i, world.clients[i].name, cfg::Root::instance().networking.connectionTimeout);
					disconnect(i);
				}
			}
		}
	}

	void ServerState::initialize_environment_user_state(int userIdx, const net::msg::NewConnection & newConnection ) {
		// Set up the user State in the world environment
		if (userIdx >= world.environment.userStates.size())
			world.environment.userStates.resize(1 + userIdx);

		world.environment.userStates[userIdx] = EnvUserState();
		world.environment.userStates[userIdx].mBody.userID = userIdx;
		world.environment.userStates[userIdx].mBody.isActive = 1;
		// Use the packet name as the new Connection name
		std::copy(newConnection.name.begin(), newConnection.name.end(), std::begin(world.environment.userStates[userIdx].mBody.userName));

	}
	void ServerState::send_initial_worldstate(int userIdx) {
		// TODO: if many clients connect at the same time, we might try to send an env state message without having received the first env state message

		if (userIdx >= 0) {
			spdlog::info("Sending env info to user {}", userIdx);
			if (world.environment.musicKnown) {
				spdlog::info("Sending existing music info to newly created userIdx {}, track {}, offset {}", userIdx, world.environment.musicState.mBody.trackName, world.environment.musicState.mBody.musicTime);
				net::send_env_msg<EnvMusicState>(clients[userIdx].peer_per_channel[(int)SignalType::Environment],
					world.environment.musicState,
					userIdx,
					sig::time_now(),
					&stats);
			}
			else {
				// We don't know what music is playing, so ask the first client to tell us

				const EnvMusicRequest mr = EnvMusicRequest{};
				spdlog::info("Music not known yet, requesting music from client {}", userIdx);
				net::send_env_msg<EnvMusicRequest>(clients[userIdx].peer_per_channel[(int)SignalType::Environment],
					mr,
					userIdx,
					sig::time_now(),
					& stats);
			}

			for (int iUserInfo = 0; iUserInfo < world.environment.userStates.size(); ++iUserInfo)
				if (iUserInfo != userIdx) { // ALSO SEND INFO FOR DISCONNECTED CLIENTS!

					spdlog::info("Sending Userstate entry {} to {}: {}", world.environment.userStates[iUserInfo].mBody.userID, userIdx, world.environment.userStates[iUserInfo].mBody.userName);
					net::send_env_msg<EnvUserState>(clients[userIdx].peer_per_channel[(int)SignalType::Environment],
						world.environment.userStates[iUserInfo],
						userIdx,
						sig::time_now(),
						&stats);

					spdlog::info("Sending New User info for {} to {}", userIdx, world.environment.userStates[iUserInfo].mBody.userID);
					net::send_env_msg<EnvUserState>(clients[iUserInfo].peer_per_channel[(int)SignalType::Environment],
						world.environment.userStates[userIdx],
						iUserInfo,
						sig::time_now(),
						&stats);
				}				
		}
	}

	void ServerState::request_telemetry(std::function<void(const TelemetryData& tdLatest)> fnTcd)
	{
		broadcast_control_msg<msg::TelemetryRequest>({}, -1, false);
		telemetryDataHandler = fnTcd;
	}

	void ServerState::handle_control_signal(ControlSignal channel, const sig::SignalMetadata& header, const void* packetData, int packetLength, ENetPeer * peer)
	{
		auto time_now = sig::time_now();
		spdlog::trace("Handling control signal of channel {}", magic_enum::enum_name<ControlSignal>(channel));
		if (channel == ControlSignal::NewConnection)
		{
			assert(packetLength == sizeof(msg::NewConnection));
			const auto& newConnection = *(msg::NewConnection*)(packetData);

			// Look if we've had this user in the past
			auto it_found = std::find_if(clients.begin(), clients.end(), [&newConnection](const ClientDataInServer& cd) { return cd.unique_id == newConnection.id; });
			bool is_reconnecting = it_found != clients.end();
			auto userIdx = int(clients.size());
			bool isNewUserIdx = true;
			if (is_reconnecting)
			{
				spdlog::info("NewConnection message contains unique ID found in existing client; That client is reconnecting, so we should disconnect the old data first");
				auto oldUserIdx = int(std::distance(clients.begin(), it_found));
				// if we're still not timed out, disconnect explicitly
				if (clients[oldUserIdx].is_connected())
					disconnect(oldUserIdx);
				userIdx = oldUserIdx;
				isNewUserIdx = userIdx != oldUserIdx;
			}
			
			if(userIdx >= clients.size())
				clients.resize(userIdx + 1);
			spdlog::info("Initializing client {} w/ userIdx {}, base address {}", clients[userIdx].name, userIdx, peer->address);
			clients[userIdx].peer_per_channel.clear();
			clients[userIdx].peer_per_channel.resize(hosts.size(), nullptr);
			clients[userIdx].peer_per_channel[(int)SignalType::Control] = peer;
			//clients[userIdx].peer_per_channel[newConnection.signal_index] = peer;			
			clients[userIdx].name = std::string(newConnection.name.data());
			clients[userIdx].last_seen_time_point = time_now;
			clients[userIdx].unique_id = newConnection.id;
			clients[userIdx].time_offset_ms_ntp = newConnection.time_offset_ms_ntp;
			clients[userIdx].ping_reminder = false;

			// All clients default to the initialization port until otherwise informed by a PortOverrideInfo signal
			
			clients[userIdx].peer_per_channel[(int)SignalType::Environment] = peer;
			
			for (int i = 0; i < scene_runtime_data.user_signals.size(); i++) {
				spdlog::info("Setting ppc {} to {}", i + (int)SignalType::Client, peer->address);
				clients[userIdx].peer_per_channel[i + (int)SignalType::Client] = peer;
			}

			
			// Currently, Clients listen to everything
			pubsub::SubRequest<int> subreq = pubsub::SubRequest<int>{
				pubsub::Op::Sub,
				pubsub::Quant::All,
				std::vector<int>{}, //  Clients list is irrelevant
				pubsub::Quant::All,
				std::vector<std::string>{} // Names of all the signals
			};

			// We always subscribe, because if a previously used userIdx has been disconnected, it's also unsubscribed
			sub_manager.Subscribe(userIdx, pubsub::SubType::Client, subreq);

			initialize_environment_user_state(userIdx, newConnection);

			spdlog::info("Created world state for idx {}, user {}, placeholder name {}", userIdx, world.environment.userStates[userIdx].mBody.userID, world.environment.userStates[userIdx].mBody.userName);

			for (int i = 0; i <= userIdx; i++) {
				spdlog::info("Current Userstate entry {}: {}, {}", i, world.environment.userStates[i].mBody.userID, world.environment.userStates[i].mBody.userName);
			}


			// Propagate connectionInfo to all other clients
			msg::ConnectionInfo ci;
			ci.name = newConnection.name;
			ci.id = newConnection.id;
			ci.time_offset_ms_ntp = newConnection.time_offset_ms_ntp;
			// The address in the returned packet to the user has to be the IP the client thinks it is (i.e. the one the client sent)			
			ci.address = peer->address;
			
			// Broadcast this info of the new client to ALL clients (and that client included, to fetch the ID)
			//spdlog::critical("broadcasting u={}", userIdx);
			auto ci_header = broadcast_control_msg(ci, userIdx, false);

			world.on_connection_info(ci_header, ci, int(scene_runtime_data.user_signals.size()));

			for (int i = 0; i < clients.size(); ++i)
			{
				if (i == userIdx || !clients[i].is_connected())
					continue;

				const auto& client = world.clients[i];
				
				std::copy(client.name.begin(), client.name.end(), ci.name.data());

				// These addresses have to be the real address (i.e. NOT newConnection.address)
				ci.address = client.address;
				ci.id = {}; // clear the ID, otherwise the client might think the message is about their address/info				
				// Send all other client information to this newly created client
				spdlog::info("Sending info from userIdx {} to newly created userIdx {}", i, userIdx);
				net::send_control_msg(clients[userIdx].peer_per_channel[(int)SignalType::Control], ci, i, time_now, &stats);

			}

			// if a listener is waiting on this client, update the listener mask AND send it a message
			//ci.address = newConnection.address;
			ci.address = peer->address;
			ci.id = newConnection.id;

			const auto& name = world.clients[userIdx].name;

			for (auto& listener : listeners)
			{
				auto itUnresolvedClient = listener.unresolvedClients.find(name);
				if (itUnresolvedClient != listener.unresolvedClients.end())
				{
					listener.unresolvedClients.erase(itUnresolvedClient);
					listener.clientMask.set(userIdx);
					net::send_control_msg(listener.peer_per_channel[(int)SignalType::Control], ci, userIdx, time_now, &stats);
				}
			}
			


			enet_host_flush(hosts[0]);

			// ... Also start a syncing procedure
			//send_control_msg(userIdx, msg::LatencyTelemetry{ msg::Sync::SenderType::Server, time_now }, userIdx, time_now);
			net::send_control_msg(peer, msg::LatencyTelemetry{ msg::LatencyTelemetry::SenderType::Server, time_now }, userIdx, time_now,&stats);

			// And send off the environment state to the new user
			spdlog::info("Sending the world state to new user {}", userIdx);
			send_initial_worldstate(userIdx);
		}
		else if (channel == ControlSignal::NewListenerConnection)
		{
			assert(packetLength == sizeof(msg::NewListenerConnection));
			const auto& newConnection = *(msg::NewListenerConnection*)(packetData);

			msg::NewListenerConnectionResponse lci;
			lci.clientIndices.fill(-1);
			lci.signalIndices.fill(-1);
			lci.id = newConnection.id;
			// Allocate the listener
			auto listenerIdx = int(listeners.size());
			listeners.resize(listenerIdx + 1);
			auto& listener = listeners[listenerIdx];
			listener.peer_per_channel.resize(hosts.size(), nullptr);

			for (int i = 0; i < newConnection.portcount; i++) {
				ENetAddress new_address = peer->address;
				new_address.port = newConnection.ports[i];
#if 0 // TODO: replicate like client! wrt peers
				auto it_peer = g_peers.find(new_address);
				auto fpeer = it_peer != g_peers.end() ? it_peer->second : nullptr;
				if (fpeer == nullptr) {
					spdlog::warn("Null peer found for {}\n", new_address);
				}
				else {
					spdlog::info("Found peer for {}\n", new_address);
				}
				listener.peer_per_channel[i] = fpeer;
#endif
			}
			//listener.peer_per_channel[0] = peer;

			listener.name = newConnection.name.data();
			//listener.address = newConnection.address;
			listener.address = peer->address;

			listener.clientMask.reset();
			listener.unresolvedClients.clear();
			for (int i = 0; i < newConnection.clients.NumNames(); ++i)
			{
				const auto& name = newConnection.clients.Name(i);
				auto it = std::find_if(world.clients.begin(), world.clients.end(), [name](const ClientInfo& ci) { return ci.name == name; });
				if (it != world.clients.end())
				{
					auto clientIdx = int(std::distance(world.clients.begin(), it));
					listener.clientMask.set(clientIdx);
					lci.clientIndices[i] = clientIdx;
				}
				else // listener needs a client that is not connected yet
				{
					listener.unresolvedClients.insert(name);
				}
			}
			spdlog::info("NewListener Connection for {}, {} clients, {} signals\n", newConnection.name.data(), newConnection.clients.NumNames(), newConnection.signals.NumNames());
			listener.signalMask.reset();
			const auto& signalConfigs = scene_runtime_data.user_signals;


			std::vector<std::string> sigNames;
			// If the Listener has 0 names, it listens to everybody
			pubsub::SubRequest<int> subreq;
			if (newConnection.signals.NumNames() > 0) {

				std::vector<int> cNames = std::vector<int>{};
				for (int i = 0; i < newConnection.signals.NumNames(); ++i) {
					const auto & name = newConnection.signals.Name(i);
					auto it = std::find_if(signalConfigs.begin(), signalConfigs.end(), [name](const SignalConfig& cc) { return cc.name == name; });
					if (it != signalConfigs.end())
					{
						sigNames.emplace_back(newConnection.signals.Name(i));
					}
				}
			}

			if ((newConnection.clients.NumNames()) > 0) {
				std::vector<int> cNames;
				for (int i = 0; i < newConnection.clients.NumNames(); ++i)
				{

					const auto& name = newConnection.clients.Name(i);
					spdlog::info("Listener {} listening to client {}\n", std::string(newConnection.name.data()), name);
					auto it = std::find_if(world.clients.begin(), world.clients.end(), [name](const ClientInfo& ci) { return ci.name == name; });
					if (it != world.clients.end())
					{
						auto clientIdx = int(std::distance(world.clients.begin(), it));
						cNames.emplace_back(clientIdx);
					}
				}
				pubsub::SubRequest<int> subreq = pubsub::SubRequest<int>{
					pubsub::Op::Sub,
					pubsub::Quant::List,
					cNames, //  Clients list is irrelevant
					pubsub::Quant::List,
					sigNames // Names of all the signals the Listener wants to hear
				};

			}
			else {
				spdlog::info("Listener {} listening to everyone\n", std::string(newConnection.name.data()));
				pubsub::Quant qval;
				if (sigNames.size() == 0) {
					qval = pubsub::Quant::All;
				}
				else
					qval = pubsub::Quant::List;

				// Listeners listen to what they're specified to listen to. Because we don't have a coherent list of names, we'll sub to all users
				// TODO: Get the list of names for listeners
				subreq = pubsub::SubRequest<int>{
					pubsub::Op::Sub,
					pubsub::Quant::All,
					std::vector<int>{}, //  Clients list is irrelevant
					qval,
					sigNames // Names of all the signal the Listener wants to hear
				};
			}


			sub_manager.Subscribe(listenerIdx, pubsub::SubType::Listener, subreq);
			sub_manager.DumpTable();

			// send the listener response back to the listener

			net::send_control_msg(peer, lci, listenerIdx, time_now, &stats);

			//For some reason, if we don't flush, the listener response doesn't get sent, and we end up with syncspam forever
			enet_host_flush(hosts[0]);


			// ... And start a syncing procedure
			//send_control_msg(listenerIdx, msg::Sync{ msg::Sync::SenderType::Server, time_now }, listenerIdx, time_now);
			net::send_control_msg(listeners[listenerIdx].peer_per_channel[(int)SignalType::Control], msg::LatencyTelemetry{msg::LatencyTelemetry::SenderType::Server, time_now}, listenerIdx, time_now, & stats);

		}
		// We got a sync message. Add the info and maybe send another one
		else if (channel == ControlSignal::LatencyTelemetry)
		{
			const auto& sync_packet = *((const msg::LatencyTelemetry*)packetData);
			auto sender_type = sync_packet.sender_type;
			if (sender_type == msg::LatencyTelemetry::SenderType::Client)
				clients[header.userIdx].last_seen_time_point = time_now;
			auto& sync_info = (sender_type == msg::LatencyTelemetry::SenderType::Client) ? clients[header.userIdx].sync_info : listeners[header.userIdx].sync_info;
			auto done_syncing = sync_info.add(std::chrono::duration_cast<std::chrono::milliseconds>((time_now - sync_packet.server_time) / 2).count());
			
			if (sender_type == msg::LatencyTelemetry::SenderType::Listener) {
				spdlog::trace("Got sync message from listener {}, done flag: {}, done count: {}", header.userIdx, done_syncing, sync_info.accum_num);
			}
			if(!done_syncing)
				net::send_control_msg(peer, msg::LatencyTelemetry{ msg::LatencyTelemetry::SenderType::Server, time_now }, header.userIdx, time_now, &stats);
			else
			{
				spdlog::info("Delay to {} {}: {} ms", magic_enum::enum_name(sender_type), header.userIdx, sync_info.delay_ms);
			}
		}
		// Client wants to use a new UDP port for communication specific signal types
		else if (channel == ControlSignal::PortOverrideInformation) {			
			const auto& port_info_packet = *((const msg::PortOverrideInformation*)packetData);
			spdlog::info("Received a port override for client {}, port {}, sigType {}", header.userIdx, peer->address.port, port_info_packet.sigType);
			if (clients[header.userIdx].unique_id == port_info_packet.id)
				clients[header.userIdx].peer_per_channel[port_info_packet.sigType] = peer; 
			else 
				spdlog::warn("Port override ID doesn't match registered ID, refusing to change ports");

		}
		// Client sends their telemetry data
		else if (channel == ControlSignal::TelemetryClientData) {
			const auto& telPacket = *((const msg::TelemetryClientData*)packetData);
			TelemetryData telDataNew;
			TelemetryKey elem{ telPacket.sigType, telPacket.sigIdx, header.userIdx, telPacket.packetIdOffset, TelemetryCapturePoint::SendingToServer };
			TelemetryCapturePoint recvAtFw = (TelemetryCapturePoint)((int)TelemetryCapturePoint::ReceivingAtClient0Framework + header.userIdx);
			const auto timePointOffset = ClientNtpTimeOffset(header.userIdx);
			for (int i = 0; i < telPacket.count; ++i)
			{
				elem.telemetryCapturePoint = TelemetryCapturePoint::SendingToServer;
				telDataNew[elem] = telPacket.timePoints[i] + timePointOffset; 
				elem.telemetryCapturePoint = recvAtFw;
				telDataNew[elem] = telPacket.timePointsAtAdapter[i] + timePointOffset;
				++elem.packetId;
			}
			clients[header.userIdx].telemetryData.insert(telDataNew.begin(), telDataNew.end());
			telemetryDataHandler(clients[header.userIdx].telemetryData);
			clients[header.userIdx].telemetryData.clear();
		}
		// Client sends telemetry data for other client signals
		else if (channel == ControlSignal::TelemetryOtherClientData) {
			const auto& telPacket = *((const msg::TelemetryOtherClientData*)packetData);
			const auto timePointOffset = ClientNtpTimeOffset(header.userIdx);
			TelemetryData telDataNew;
			TelemetryCapturePoint recvAtNative = (TelemetryCapturePoint)((int)TelemetryCapturePoint::ReceivingAtClient0Native + header.userIdx);
			TelemetryCapturePoint recvAtFw = (TelemetryCapturePoint)((int)TelemetryCapturePoint::ReceivingAtClient0Framework + header.userIdx);
			TelemetryKey elem{ telPacket.sigType, telPacket.sigIdx, telPacket.clientId, 0, recvAtNative };
			
			for (int i = 0; i < telPacket.count; ++i)
			{
				elem.packetId = telPacket.packetIds[i];
				elem.telemetryCapturePoint = recvAtNative;
				telDataNew[elem] = telPacket.timePoints[i] + timePointOffset;
				elem.telemetryCapturePoint = recvAtFw;
				telDataNew[elem] = telPacket.timePointsAtAdapter[i] + timePointOffset;
			}
			clients[header.userIdx].telemetryData.insert(telDataNew.begin(), telDataNew.end());
			telemetryDataHandler(clients[header.userIdx].telemetryData);
			clients[header.userIdx].telemetryData.clear();
		}
		else if (channel == ControlSignal::PingReply) {
			spdlog::debug("Received ping reply from {}", header.userIdx);
			clients[header.userIdx].last_seen_time_point = sig::time_now();
		}
		else {
			spdlog::warn("Unknown control message of type {} from {}", (int)channel, peer->address);

		}
	}

	// Returns the number of times enet_peer_send was used
	int ServerState::broadcast_signal(int userIdx, int signalIdx, enet_uint8 channelID, ENetPacket* packet)
	{

		static ENetPacket* outPacket;

		const std::string& signame = signal_list[signalIdx];
		
		std::set<pubsub::Subscriber<int>> sublist = sub_manager.GetRecipients(userIdx, signame);

		spdlog::trace("Outgoing packet to {} subscribers", sublist.size());
		
		int times_sent = 0;
		for (const auto& sub : sublist) {

			ENetPeer* currentPeer;

			// We need to make a new packet each time we send. enet_peer_send destroys it
			spdlog::trace("Making new outgoing packet");
			//outPacket = net::make_packet(packet->data, false);
			switch (sub.type) {

			case pubsub::SubType::Client:
				if (clients[sub.id].is_connected())
				{
					currentPeer = clients[sub.id].peer_per_channel[2 + signalIdx]; // This indexing pattern is super-brittle and should be fixed
					spdlog::trace("Sending to client {}, {}\n", sub.id, signame);
					if ((currentPeer == nullptr) || (currentPeer->state != ENET_PEER_STATE_CONNECTED)) {
						spdlog::trace("Peer not connected\n");
						continue;
					}
					enet_peer_send(currentPeer, channelID, packet);
					++times_sent;
				}
				break;
			case pubsub::SubType::Listener:
				// is the peer connected?
				spdlog::trace("Sending to listener {}, {}\n", sub.id, signame);
				currentPeer = listeners[sub.id].peer_per_channel[2 + signalIdx]; // This indexing pattern is super-brittle and should be fixed
				if ((currentPeer == nullptr) || (currentPeer->state != ENET_PEER_STATE_CONNECTED))
					continue;
				spdlog::trace("Listener send {}\n", currentPeer->address);

				// We need to make a new packet each time we send. enet_peer_send destroys it				
				enet_peer_send(currentPeer, channelID, packet);
				++times_sent;
				break;
			}
		}

		if (times_sent > 0) {
			spdlog::trace("Packet sent {} times", times_sent);
			//enet_packet_destroy(packet);
		}
		return times_sent;
		
	}
	
	// Should check user against a list of authorized users for a given message type
	bool ServerState::env_check_authorization(uint32_t message_type, uint32_t user) {
		return true;
	}

	std::string make_world_summary(const WorldState& ws, const std::vector<ClientDataInServer>& clients)
	{
		int numConnected = 0;
		for (const auto& client : clients)
			if (client.is_connected())
				++numConnected;
		auto s = fmt::format("WorldState clients: {} registered / {} connected:\n", clients.size(), numConnected);
		assert(clients.size() == ws.clients.size());
		auto it_client = clients.begin();
		for (const auto& client : ws.clients)
		{
			auto q = fmt::format("\t{} at {}, last seen at {}{}\n", client.name, client.address, sig::time_point_as_string(it_client->last_seen_time_point), it_client->is_connected() ? "" : " (DISCONNECTED)");;
			s += q;
			++it_client;
		}
		//s += fmt::format("Environment: {}\n", ws.environment.to_string());
		return s;
	}

	void ServerState::tick()
	{
		if (!is_initialized)
			return;
		// TODO: on disconnect, notify clients
		static int counter = 0;

		spdlog::trace("######## tick {} ############", counter);
		// This is expensive, so don't make the string anyway
		if (spdlog::get_level() <= spdlog::level::trace)
			spdlog::trace("{}", make_world_summary(world, clients));
		
		ENetEvent event;
		for(int iHost = 0; iHost < hosts.size(); ++iHost)
			while (enet_host_service(hosts[iHost], &event, 0) > 0)
			{
				spdlog::trace("New event {} on channel {} ({})", magic_enum::enum_name<ENetEventType>(event.type), iHost, counter);
				switch (event.type)
				{
				case ENET_EVENT_TYPE_CONNECT:
				{
					spdlog::info("A new client connected to {} from {}", hosts[iHost]->address, event.peer->address);
					event.peer->data = (void*)"Client information";
					
					break;
				}
				case ENET_EVENT_TYPE_RECEIVE:
				{
					auto time_now = sig::time_now();

					assert(event.packet->dataLength >= sizeof(sig::SignalMetadata));
					const auto& packetHeader = *(sig::SignalMetadata*)event.packet->data;
					auto packetData = event.packet->data + sizeof(sig::SignalMetadata);
					auto packetLen = event.packet->dataLength - sizeof(sig::SignalMetadata);

					auto sigType = (SignalType)packetHeader.sigType;
					stats.OnReceivedPacket(packetHeader, event.packet->dataLength);


					// Register client/environment data production telemetry entries
					if (sigType != SignalType::Control)
					{
						TelemetryKey elem{ packetHeader.sigType, packetHeader.sigIdx, packetHeader.userIdx, packetHeader.packetId, TelemetryCapturePoint::DataProduction };
						const auto timePointOffset = ClientNtpTimeOffset(packetHeader.userIdx);
						clients[packetHeader.userIdx].telemetryData[elem] = packetHeader.acquisitionTime + timePointOffset;

						TelemetryKey elem2{ packetHeader.sigType, packetHeader.sigIdx, packetHeader.userIdx, packetHeader.packetId, TelemetryCapturePoint::ReceivingAtServer };
						clients[packetHeader.userIdx].telemetryData[elem2] = time_now;
					}
					
					spdlog::trace("Packet {} of length {} was received from {} (userIdx={}), of signal type {}/{}.", packetHeader.packetId,
						event.packet->dataLength,
						event.peer->address,
						packetHeader.userIdx,
						magic_enum::enum_name(sigType),
						packetHeader.sigIdx);
					if (spdlog::get_level() <= spdlog::level::trace && sigType != SignalType::Control && packetHeader.userIdx >= 0 && packetHeader.userIdx < clients.size())
					{							
						spdlog::trace("There are {} peers\n", clients[packetHeader.userIdx].peer_per_channel.size());
						for (int i = 0; i < clients[packetHeader.userIdx].peer_per_channel.size(); i++) {
							auto ppc = clients[packetHeader.userIdx].peer_per_channel[i];
							if (ppc != nullptr) {
								spdlog::trace("Client {}, ppc {}, val {}, idx {}", packetHeader.userIdx, i, ppc->address, packetHeader.sigIdx);
							}
							else {
								spdlog::trace("Client {}, ppc {} null", packetHeader.userIdx, i);
							}
						}
					}

					bool destroyPacket = true;
					if(sigType == SignalType::Control)
						handle_control_signal((ControlSignal)packetHeader.sigIdx, packetHeader, packetData, packetLen, event.peer);
					else if(packetHeader.userIdx >= 0 && clients[packetHeader.userIdx].is_connected() && clients[packetHeader.userIdx].peer_per_channel[iHost] != nullptr) // only handle if it's connected
					{
						if (packetHeader.userIdx >= 0)
						{
							clients[packetHeader.userIdx].last_seen_time_point = time_now;

							RegisterPacketToTracker(packetHeader, time_now, "RECV");
						}
							
						if (sigType == SignalType::Environment) {
							// Now we peer inside the signal to find what type of signal it is, and store it if appropriate
							uint8_t* sigID = (uint8_t*)&packetHeader;
							/*
							std::stringstream ss;
							ss << std::hex;

							for (int i = 0; i < event.packet->dataLength; i++) {
								ss << (int)sigID[i] << " ";
							}
							spdlog::info("Received env sig of type {}: ({}): {}", sigEnvType, packetLen, ss.str());
							*/
							EnvMessageGeneric* sigInitial = (EnvMessageGeneric*)packetData;
							int32_t sigEnvType = sigInitial->signalID;
							
							
							EnvSceneState* ess;
							EnvUserState* eus;
							EnvMusicState* ems;
							EnvTestState* ets;
							EnvUserRequest * eur;

							// Is there a nice way to do this without either Switch or a table of handler functions?
							switch (sigEnvType) {

							case EnvSceneStateID: // State has been updated
								// TODO: Authorization checking
								ess = (EnvSceneState*)packetData;
								if (env_check_authorization(EnvSceneStateID, packetHeader.userIdx))
									world.environment.sceneState = *ess;

								break;
							case EnvUserStateID: // A user's individual state bundle has been updated
								eus = (EnvUserState*)packetData;
								spdlog::info("Received user state update from {}, ID: {}", packetHeader.userIdx, eus->mBody.userID);
								if (packetHeader.userIdx == eus->mBody.userID) {
									world.environment.userStates[packetHeader.userIdx] = *eus;
									spdlog::warn("Received User State update for {} from {}",eus->mBody.userName, packetHeader.userIdx);
									spdlog::info("User State has clienttype {}", eus->mBody.clientType);
									world.clients[packetHeader.userIdx].name = eus->mBody.userName;

									clients[packetHeader.userIdx].name = eus->mBody.userName; // Update the name used in the gui
								}
								else {

									spdlog::warn("User {} is trying to update user {}'s state, overriding with packet ID!", packetHeader.userIdx, eus->mBody.userID);
									// Because producers don't have easy access to the userID from the server, and we need produced user State packets, we'll disregard the userID field
									// and assume the user is updating their own field
									eus->mBody.userID = packetHeader.userIdx;
									world.environment.userStates[packetHeader.userIdx] = *eus;
									spdlog::warn("Received User State update for {} from {}", eus->mBody.userName, packetHeader.userIdx);
									spdlog::info("User State has clienttype {}", eus->mBody.clientType);
									
									world.clients[packetHeader.userIdx].name = eus->mBody.userName;

									clients[packetHeader.userIdx].name = eus->mBody.userName; // Update the name used in the gui



								}
								break;
							case EnvMusicStateID: // Music state has been updated
								// TODO: Authorization checking
								if (env_check_authorization(EnvMusicStateID, packetHeader.userIdx))
									ems = (EnvMusicState*)packetData;								
								world.environment.musicState = *ems;
								world.environment.musicKnown = true;
								spdlog::info("Received music state from user {}, track {}, offset {}, isPLaying {}", packetHeader.userIdx, world.environment.musicState.mBody.trackName, world.environment.musicState.mBody.musicTime, world.environment.musicState.mBody.isPlaying);

								break;
							case EnvTestStateID: // Just some test stuff	
								ets = (EnvTestState*)packetData;
								world.environment.testState = *ets;							
								spdlog::info("Test State: {}", world.environment.testState.toString());
								break;
							case EnvSceneRequestID: // Request scene state
								spdlog::info("Received Env Scene state Req from user {}", packetHeader.userIdx);
								net::send_env_msg<EnvSceneState>(clients[packetHeader.userIdx].peer_per_channel[(int)SignalType::Environment],
									world.environment.sceneState,
									packetHeader.userIdx,
									sig::time_now(),
									&stats);
								break;
							case EnvUserRequestID: // Request a user's state
								eur = (EnvUserRequest *)packetData;
								spdlog::info("Received User {} state Req from user {}", eur->mBody.userID, packetHeader.userIdx);

								try {
									net::send_env_msg<EnvUserState>(clients[packetHeader.userIdx].peer_per_channel[(int)SignalType::Environment],
										world.environment.userStates[eur->mBody.userID],
										packetHeader.userIdx,
										sig::time_now(),
										&stats);

								}
								catch (std::out_of_range e) {
									// User not found. Maybe we should send a message?
									spdlog::warn("Received request for non-existent user {}", eur->mBody.userID);
								}


								break;
							case EnvMusicRequestID: // Request the musical state
								spdlog::info("Received Env Music state Req from user {}", packetHeader.userIdx);
								net::send_env_msg<EnvMusicState>(clients[packetHeader.userIdx].peer_per_channel[(int)SignalType::Environment],
									world.environment.musicState,
									packetHeader.userIdx,
									sig::time_now(),
									& stats);
								break;
							case EnvTestRequestID: // Test message, please ignore
								spdlog::info("Received Env Test state Req from user {}", packetHeader.userIdx);
								net::send_env_msg<EnvTestState>(clients[packetHeader.userIdx].peer_per_channel[(int)SignalType::Environment],
									world.environment.testState,
									packetHeader.userIdx,
									sig::time_now(),
									&stats);
								break;
							case EnvMessageGenericID: default: // Nothing to see here
								spdlog::warn("Unknown Environment Message ID: {}", sigEnvType);


								break;
							}
						}
						
						spdlog::trace("Reflecting the recently received packet\n");

						// transform to state
						auto& signals = sigType == SignalType::Client ? world.clients[packetHeader.userIdx].signals: world.environment.signals;
						auto& sigData = signals[packetHeader.sigIdx];
						const auto& signal_cfgs = sigType == SignalType::Client ? scene_runtime_data.user_signals : scene_runtime_data.env_signals;
						const auto& sigCfg = signal_cfgs[packetHeader.sigIdx];
						sigData.from_network(packetData, packetLen, sigCfg.signalProperties);
						// broadcast (as-is) to other clients and listeners

						
						// From the ENet docs:
						// Once the packet is handed over to ENet with enet_peer_send(), ENet will handle its deallocation and enet_packet_destroy() should not be used upon it.
						// Therefore we don't destroy the packet if we ever send anything
						auto times_sent = broadcast_signal(packetHeader.userIdx, packetHeader.sigIdx, event.channelID, event.packet);
						destroyPacket = false;
						stats.OnSentPacket(packetHeader, event.packet->dataLength * times_sent);

					}

					/* Clean up the packet now that we're done using it. */
					if (destroyPacket) {
						enet_packet_destroy(event.packet);
					}
					break;
				}

				case ENET_EVENT_TYPE_DISCONNECT:
				{
					spdlog::info("{} disconnected.\n", event.peer->data);
					/* Reset the peer's client information. */
					event.peer->data = NULL;
					
					for (int i = 0; i < clients.size(); ++i)
					{
						for (int j = 0; j < clients[i].peer_per_channel.size(); ++j)
							if (clients[i].peer_per_channel[j] == event.peer)
							{
								disconnect(i);
							}						
					}

					break;
				}
				case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
				{
					spdlog::info("{} disconnected due to timeout.\n", event.peer->data);
					/* Reset the peer's client information. */
					event.peer->data = NULL;

					for (int i = 0; i < clients.size(); ++i)
					{
						for (int j = 0; j < clients[i].peer_per_channel.size(); ++j)
							if (clients[i].peer_per_channel[j] == event.peer)
							{
								disconnect(i);
							}

					}

					break;
				}
				}
			}
		check_disconnected();
		++counter;
	}

	void ServerState::deinitialize()
	{
		for (auto& client : clients)
			client.disconnect();
		for (auto& listener : listeners)
		{
			for (auto peer : listener.peer_per_channel)
				enet_peer_disconnect(peer, 0);
			listener.peer_per_channel.resize(0);
		}
		for (auto host : hosts)
			enet_host_destroy(host);
		clients.resize(0);
		listeners.resize(0);

		hosts.resize(0);
	}

	bool ServerState::initialize(const cfg::Server& cfg)
	{

		spdlog::info("## Dancegraph Native Server Built on {} ##", __TIMESTAMP__);
		// start a task to get our offset to the NTP server
		auto time_offset_ms_ntp_result = std::async(net::time_offset_ms_ntp);

		if (!scene_runtime_data.Initialize(cfg.scene, {}, {}, {}, false, false))
		{
			spdlog::error("Scene runtime data initialization failed {}\n", cfg.scene.c_str());
			return false;
		}

		auto num_user_signals = int(scene_runtime_data.user_signals.size());
		auto num_env_signals = int(scene_runtime_data.env_signals.size());

		start_time_point = sig::time_now();
		server_address = make_server_address(cfg.address);
		//server_address = make_address(cfg.address);
		
		//address.host = ENET_HOST_ANY;
		bool ok = create_hosts_multiport(hosts, server_address, num_user_signals, num_env_signals, true);
		if (!ok)
			return false;
		world.environment.signals.resize(num_env_signals);


		spdlog::info("Server running");

		for (auto& sig : scene_runtime_data.user_signals)
			signal_list.emplace_back(sig.name);

		sub_manager.Initialize(signal_list);

		// By now, expect to have the result. Use std::chrono::duration<float, std::milli> to convert for time math
		time_offset_ms_ntp = time_offset_ms_ntp_result.get();

		is_initialized = true;
		return true;
	}

	void ServerState::run(const cfg::Server& cfg, bool(*callback_fn)())

	{		
		if (initialize(cfg)) {
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