#define _WINSOCKAPI_    // stops windows.h including winsock.h
#include "client.h"

#include <thread>
#include <filesystem>
#include <magic_enum/magic_enum.hpp>

#include <core/common/utility.h>

#include "net.h"
#include "formatter.h"

//#include <chrono>
//#include <thread>

#include <core/net/config_master.h> // For DanceGraphAppDataPath

#include <sstream>

namespace net
{

	void ClientState::update_local_user_index(uint16_t idx) {
		userIdx = idx;
		scene_runtime_data.BroadcastLocalUserIndex(idx);
	}

	void ClientState::handle_message(const uint8_t* data, int size, int channelId)
	{
		auto time_now = sig::time_now();
		// With CH below: changes to server code
		assert(size >= sizeof(sig::SignalMetadata));
		const auto& packetHeader = *(sig::SignalMetadata*)data;
		auto payload = data + sizeof(sig::SignalMetadata);
		auto payloadSize = size - sizeof(sig::SignalMetadata);

		const SignalType sigType = (SignalType)packetHeader.sigType;

		if (sigType == SignalType::Control)
		{
			spdlog::trace("recv sig type {} at channel {}\n", int(sigType), int(channelId));
			if (channelId == (int)msg::ConnectionInfo::kControlSignal)
			{
				const auto& connectionInfo = *(msg::ConnectionInfo*)payload;
				world.on_connection_info(packetHeader, connectionInfo, scene_runtime_data.user_signals.size());

				spdlog::info("Comparing address {} to {}, coming from user {} ({})\n", connectionInfo.address, address, std::string(connectionInfo.name.data()), packetHeader.userIdx);
				//if (memcmp(&connectionInfo.address, &address, sizeof(ENetAddress)) == 0)
				if (memcmp(&connectionInfo.id.value, &uniqueID.value, sizeof(msg::ClientID)) == 0) {
					if (userIdx != -1)
						spdlog::critical("re-receiving userIdx: new={} old={}", packetHeader.userIdx, userIdx);

					spdlog::info("Got a matched ID {}: New UserIdx = {}", connectionInfo.id.toString(), userIdx);
					update_local_user_index(packetHeader.userIdx);


					// Okay, so we have our userID. Time to send out port override messages for environment and client signals


					if (!config.single_port_operation) {
						msg::PortOverrideInformation mEnv;
						mEnv.id = uniqueID;
						mEnv.sigType = (int)SignalType::Environment;

						// This is an odd control message because it's sent outwith the control port					
						send_control_msg(mEnv, sig::time_now(), SignalType::Environment);

						msg::PortOverrideInformation mClient;
						mClient.id = uniqueID;
						mClient.sigType = (int)SignalType::Client;

						for (int i = 0; i < scene_runtime_data.user_signals.size(); i++) {
							mClient.sigIdx = i;
							send_control_msg(mClient, sig::time_now(), SignalType::Client, i);
						}
					}
				}
			}
			// We got a sync signal -- server wondering if we're still there
			else if (channelId == (int)msg::PingRequest::kControlSignal)
			{
				spdlog::debug("Received Ping request, responding");
				send_control_msg(msg::PingReply{}, time_now);
			}

			// We got a sync signal -- just bounce it back!
			else if (channelId == (int)msg::LatencyTelemetry::kControlSignal)
			{
				const auto& sync_packet = *(msg::LatencyTelemetry*)payload;
				send_control_msg(msg::LatencyTelemetry{ msg::LatencyTelemetry::SenderType::Client, sync_packet.server_time }, time_now);
			}
			// We got a telemetry request
			else if (channelId == (int)msg::TelemetryRequest::kControlSignal)
			{
				const auto& packet = *(msg::TelemetryRequest*)payload;
				for (int iClient = 0; iClient < world.clients.size(); ++iClient)
				{
					const auto& client = world.clients[iClient];
					if (client.is_initialized())
					{
						for (int iSig = 0; iSig < client.signals.size(); ++iSig)
						{
							const auto& signalData = client.signals[iSig];
							if (!signalData.packetIds.empty())
							{
								// Is the server requesting telemetry data for this client?
								if (iClient == userIdx)
								{
									int numLeft = signalData.timePoints.size();
									int numCopied = 0;
									auto id0 = signalData.packetIds.front();
									while (numLeft > 0)
									{
										const int numCopy = std::min(numLeft, msg::TelemetryClientData::MAX_COUNT);
										auto msg = msg::TelemetryClientData{ (uint8_t)SignalType::Client, (uint8_t)iSig, (uint16_t)numCopy, id0, {} };
										std::copy(signalData.timePoints.begin() + numCopied, signalData.timePoints.begin() + numCopied + numCopy, msg.timePoints.begin());
										std::copy(signalData.timePointsAtAdapter.begin() + numCopied, signalData.timePointsAtAdapter.begin() + numCopied + numCopy, msg.timePointsAtAdapter.begin());
										send_control_msg(msg, time_now);
										numLeft -= numCopy;
										numCopied += numCopy;
										id0 += numCopy;
									}
								}
								else // the server is requesting telemetry data that this client stores for other client signals
								{
									int numLeft = signalData.timePoints.size();
									int numCopied = 0;
									while (numLeft > 0)
									{
										const int numCopy = std::min(numLeft, msg::TelemetryClientData::MAX_COUNT);
										auto msg = msg::TelemetryOtherClientData{ (uint8_t)SignalType::Client, (uint8_t)iSig, (uint16_t)numCopy, iClient, {}, {} };
										std::copy(signalData.timePoints.begin() + numCopied, signalData.timePoints.begin() + numCopied + numCopy, msg.timePoints.begin());
										std::copy(signalData.timePointsAtAdapter.begin() + numCopied, signalData.timePointsAtAdapter.begin() + numCopied + numCopy, msg.timePointsAtAdapter.begin());
										std::copy(signalData.packetIds.begin() + numCopied, signalData.packetIds.begin() + numCopied + numCopy, msg.packetIds.begin());
										send_control_msg(msg, time_now);
										numLeft -= numCopy;
										numCopied += numCopy;
									}
								}

							}
						}
					}
				}
			}
		}
		else
		{
			// Handle special case of bad user index, or unregistered user. in this case, don't process packet
			if (sigType == SignalType::Client && (packetHeader.userIdx < 0 || packetHeader.userIdx >= world.clients.size()))
			{
				spdlog::warn("Client signal with invalid user idx {}/{}", packetHeader.userIdx, world.clients.size());
				return;
			}

			if (sigType == SignalType::Client && !world.clients[packetHeader.userIdx].is_initialized()) {
				spdlog::warn("Client signal with uninitialized user {}/{}", packetHeader.userIdx, world.clients.size());
				return;
			}

			if (packetHeader.userIdx >= 0)
			{
				RegisterPacketToTracker(packetHeader, time_now, "RECV");
				if (sigType == SignalType::Client)
				{
					auto& signalData = world.clients[packetHeader.userIdx].signals[packetHeader.sigIdx];
					signalData.timePoints.push_back(time_now);
					signalData.packetIds.push_back(packetHeader.packetId);

					signalData.timePointsAtAdapter.emplace_back(); // initialize with some default value
					spdlog::trace("Emplacing timePointsAtAdapter to size {}", signalData.timePointsAtAdapter.size());
				}
			}

			// 
			// prepare the signal metadata
			std::vector<net::SignalData> signalData;

			if (sigType == SignalType::Client)
				signalData = world.clients[packetHeader.userIdx].signals;
			else
				signalData = world.environment.signals;

			//auto& signalData = (sigType == SignalType::Client) ? world.clients[packetHeader.userIdx].signals: world.environment.signals;
			if (!scene_runtime_data.CheckSignalType(packetHeader)) {

				spdlog::warn("Client Signal of unhandled type {}, idx {}. Do client and server configs match?", packetHeader.sigType, packetHeader.sigIdx);
				return;
			}

			auto& cache = cache_st;
			auto& cache2 = cache2_st;

			const auto& sigCfg = scene_runtime_data.SignalConfigFromMetadata(packetHeader);

			//spdlog::info(  "Received the packet for processing to the {} consumers!", sigCfg.consumers.size());
			int stateSize = -1;
			if (sigCfg.signalProperties.keepState)
			{

				// Which is right - packetLen or size?
				cache.resize(sigCfg.signalProperties.stateFormMaxSize);
				stateSize = sigCfg.signalProperties.networkToState(payload, payloadSize, cache.data(), cache.size(), nullptr);

			}

			// If signal is passthru, then we can use the network payload directly 
			const uint8_t* consumerDataPtr = payload;
			int consumerDataSize = payloadSize;
			// If it's not passthru, we need a transform!
			if (!sigCfg.signalProperties.isPassthru)
			{
				if (!sigCfg.transformed_input_only) {
					//spdlog::info("Signal {}/{} is apparently non-transformer exclusive", packetHeader.sigType, packetHeader.sigIdx);
					cache2.resize(sigCfg.signalProperties.consumerFormMaxSize);

					// if we keep state, then use the state->consumer function
					if (sigCfg.signalProperties.keepState)
					{
						consumerDataSize = sigCfg.signalProperties.stateToConsumer(cache.data(), stateSize, cache2.data(), cache2.size(), nullptr);
					}
					// Otherwise, if we don't keep state, then use the network->consumer function
					else
					{
						consumerDataSize = sigCfg.signalProperties.networkToConsumer(payload, payloadSize, cache2.data(), cache2.size(), nullptr);
					}
				}
				else {
					//spdlog::trace("Packet {} being skipped for transformer exclusivity");
				}
			}

			// send to local consumer
			// TODO: below, time should really be network time (from the packet)

			if (fn_process_signal_data)
			{
				std::string text;
				sigCfg.signalProperties.toString(text, consumerDataPtr, consumerDataSize, sig::SignalStage::Consumer);
				spdlog::trace("Signal data from net: {}\n", text.c_str());
				if (!sigCfg.transformed_input_only) {
					//spdlog::info("Signal {}/{} is apparently non-transformer exclusive", packetHeader.sigType, packetHeader.sigIdx);
					fn_process_signal_data(consumerDataPtr, consumerDataSize, packetHeader);
				}
				else {
					//spdlog::info("Packet {} being skipped for transformer exclusivity", packetHeader.packetId);
				}

			}

			// We send to local consumers if the transformer doesn't demand exclusivity
			if (!sigCfg.transformed_input_only) {
				for (auto& consumer : sigCfg.consumers) {
					spdlog::info("Packet {}: Info going out to consumer: {}", packetHeader.packetId, consumerDataSize);
					consumer->fnProcessSignalData(consumerDataPtr, consumerDataSize, packetHeader);
				}
			}
			else {
				spdlog::info("Packet {} being skipped for transformer exclusivity", packetHeader.packetId);
			}

			// Send to transformers too
			for (auto& trformer : sigCfg.transformer_inputs) {
				spdlog::debug("Info going out to transformer: {}", consumerDataSize);
				trformer->fnProcessSignalData(consumerDataPtr, consumerDataSize, packetHeader);

			}

		}
	}

	void ClientState::record_adapter_signal_telemetry(const sig::SignalMetadata& sigMeta, const sig::time_point& tp)
	{

		return;
		/*
		if (sigMeta.userIdx >= 0)
		{
			if ((SignalType)sigMeta.sigType == SignalType::Client)
			{
				auto& signalData = world.clients[sigMeta.userIdx].signals[sigMeta.sigIdx];
				if (signalData.timePointsAtAdapter.size() <= 0) {
					spdlog::warn("Timepoints vector has size zero, not writing to it!");
					return;
				}

				// for local user, serial packets are guaranteed, so no need to search
				if(sigMeta.userIdx == userIdx)
					signalData.timePointsAtAdapter[sigMeta.packetId-signalData.packetIds.front()] = tp;
				else // search for matching packet id
					for (int i = int(signalData.packetIds.size()) - 1; i >= 0; --i)
						if (signalData.packetIds[i] == sigMeta.packetId)
							signalData.timePointsAtAdapter[i] = tp;
			}
		}
		*/
	}

	void ClientState::tick()
	{
		// TODO: when server is down, wait a bit and try to reconnect
		static int counter = 0;
		++counter;
		for (int iHost = 0; iHost < int(hosts.size()); ++iHost)
		{
			//spdlog::info("Ticking counter {}, host {}", counter, iHost);
			ENetEvent event;
			while (enet_host_service(hosts[iHost], &event, 1) > 0)
			{
				spdlog::trace("CLIENT: Got a message!... ({})\n{}\n", counter, world);
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
					const auto& packetHeader = *(sig::SignalMetadata*)event.packet->data;
					auto sigType = (SignalType)packetHeader.sigType;
					spdlog::trace("A packet of length {} was received from {}, of signal type {}.\n", event.packet->dataLength, event.peer->address, magic_enum::enum_name(sigType));
					handle_message(event.packet->data, event.packet->dataLength, event.channelID);
					/* Clean up the packet now that we're done using it. */
					enet_packet_destroy(event.packet);
					break;
				}

				case ENET_EVENT_TYPE_DISCONNECT:
				{
					spdlog::info("Client disconnected");
					/* Reset the peer's client information. */
					event.peer->data = NULL;
					break;
				}
				}
			}
		}

		// If this user is valid in the server, record and transmit locally-generated signal messages (if we have)
		if (userIdx >= 0)
		{
			// First check if the user has any new signal data from their hardware
			auto& signalState = world.clients[userIdx].signals;
			_process_outgoing_signal(SignalType::Client, signalState);

			auto& envSignalState = world.environment.signals;
			_process_outgoing_signal(SignalType::Environment, envSignalState);
			_process_transformer_output(SignalType::Client, signalState);
		}
		else {

		}

	}


	// Revert to non-threaded code for the transformer since we're doing multiple calls with the same parameters
	void ClientState::_process_transformer_output(SignalType sType, std::vector<net::SignalData>& signalData) {
		// Pretty much the same as _process_outgoing_signal but only looking at the transformer output

		for (int sigIdx = 0; sigIdx < signalData.size(); ++sigIdx)
		{

			const auto& channelCfg = scene_runtime_data.SignalConfigFromIndexAndType(sigIdx, sType);
			
			// If there's no transformer here, we don't care
			if (channelCfg.transformer_output == nullptr)
				return;

			auto& cache = caches_xform_mt[sigIdx];
			auto& cache2 = caches2_xform_mt[sigIdx];

			const int hs = sizeof(sig::SignalMetadata);
			cache.resize(channelCfg.signalProperties.producerFormMaxSize + hs);

			auto& signal_time = time_points_mt[sigIdx];

			for (int call = 0; call < channelCfg.transform_multi_prod_max; call++) {
				auto& getDataFn = channelCfg.transformer_output->fnGetSignalData;
				int numBytesRead = getDataFn((uint8_t*)(cache.data()), std::ref(signal_time));
				spdlog::info("Transformer Pass {}/{}, sigIdx {}, read {} bytes", call, channelCfg.transform_multi_prod_max, sigIdx, numBytesRead);
				bool hasNewSignal = numBytesRead > 0;
				// There's no more data to read. Return!
				if (!hasNewSignal) {
					spdlog::info("Dropped transformer produce calls after iter {}", call);
					return;
				}
				auto& stateData = signalData[sigIdx].data;
				int stateSize = 0;

				if (channelCfg.transform_includes_metadata) {
					//spdlog::info("Transformer pass {}, sigIdx {}, signal size {} received with metadata", call, sigIdx, numBytesRead);
					sig::SignalMetadata* sm = (sig::SignalMetadata*)stateData.data();

					/*
					spdlog::info("Signal Meta : packet {}, Type {}, Idx{}, Usr {}",
						sm->packetId,
						sm->sigIdx,
						sm->sigType,
						sm->userIdx);

					if (numBytesRead >= 64) {
						std::stringstream ss = std::stringstream("");
						ss << std::hex;
						for (int i = 0; i < 64; i++) {
							ss << (int)*(cache.data() + i) << " ";
						}
						spdlog::info("Client.cpp xform: sigdump: {}", ss.str());
					}
					*/
					if (channelCfg.signalProperties.keepState)
					{
						stateData.resize(channelCfg.signalProperties.stateFormMaxSize);
						stateSize = channelCfg.signalProperties.producerToState(cache.data(), numBytesRead, stateData.data(), stateData.size(), nullptr);
					}
					int networkSize = numBytesRead;
					if (!channelCfg.signalProperties.isPassthru && channelCfg.transformer_to_network)
					{
						cache2.resize(channelCfg.signalProperties.networkFormMaxSize + hs);
						int stateSize = 0;
						if (channelCfg.signalProperties.keepState) {
							networkSize = channelCfg.signalProperties.stateToNetwork(stateData.data(), stateSize, cache2.data(), cache2.size(), nullptr);
						}
						else {
							networkSize = channelCfg.signalProperties.producerToNetwork(cache.data(), numBytesRead, cache2.data(), cache2.size(), nullptr);
						}
						cache2.resize(networkSize + hs);
					}
					else {
						cache.resize(numBytesRead);
					}
				}
				else {

					if (channelCfg.signalProperties.keepState)
					{
						stateData.resize(channelCfg.signalProperties.stateFormMaxSize);
						stateSize = channelCfg.signalProperties.producerToState(cache.data() + hs, numBytesRead, stateData.data(), stateData.size(), nullptr);
					}
					int networkSize = numBytesRead + hs;
					if (!channelCfg.signalProperties.isPassthru && channelCfg.transformer_to_network)
					{
						cache2.resize(channelCfg.signalProperties.networkFormMaxSize + hs);
						int stateSize = 0;
						if (channelCfg.signalProperties.keepState) {
							networkSize = channelCfg.signalProperties.stateToNetwork(stateData.data(), stateSize, cache2.data() + hs, cache2.size() - hs, nullptr);
						}
						else {
							networkSize = channelCfg.signalProperties.producerToNetwork(cache.data() + hs, numBytesRead, cache2.data() + hs, cache2.size() - hs, nullptr);
						}
						cache2.resize(networkSize + hs);
					}
					else {
						cache.resize(numBytesRead + hs);
					}
				}

				std::vector<uint8_t> packetVec = channelCfg.signalProperties.isPassthru ? cache : cache2;
				sig::SignalMetadata& header = *(sig::SignalMetadata*)cache.data();
				if (!channelCfg.transform_includes_metadata)
					header = sig::SignalMetadata{ signal_time, signalData[sigIdx].sentCounter++, userIdx, uint8_t(sigIdx), (uint8_t)sType };

				// Telemetry for PRODUCER data
				auto time_now = sig::time_now();
				signalData[sigIdx].timePoints.push_back(time_now);
				if (signalData[sigIdx].packetIds.empty())
					signalData[sigIdx].packetIds.push_back(header.packetId);
				RegisterPacketToTracker(header, signal_time, "SEND");

				if (channelCfg.transformer_to_network) {
					auto packet = make_packet(packetVec, channelCfg.signalProperties.isReliable);
					auto enet_channel_id = (sType == SignalType::Client) ? 0 : sigIdx;
					auto peer_server = server_peer_per_channel[(sType == SignalType::Client) ? 2 + sigIdx : 1];
					spdlog::debug("Sending transformer packet size {}, id {} as user {}\n", packet->dataLength, header.packetId, header.userIdx);
					auto err = enet_peer_send(peer_server, enet_channel_id, packet);

					if (err != 0)
						spdlog::error("send_msg error!");
				}
				// By default (if passthru), set the consumer data as the producer data
				const uint8_t* consumerPtr = cache.data() + hs;

				int consumerSize = numBytesRead;
				if (channelCfg.transform_includes_metadata) {
					consumerSize = consumerSize > hs ? consumerSize - hs : 0;
				}
				// Otherwise, transform from cache2 (that stores the network form)


				if (!channelCfg.signalProperties.isPassthru)
				{
					cache.resize(channelCfg.signalProperties.consumerFormMaxSize);
					consumerSize = channelCfg.signalProperties.networkToConsumer(cache2.data() + hs, cache2.size() - hs, cache.data(), cache.size(), nullptr);
				}

				spdlog::trace("Packet {} outgoing via {}", header.packetId, __FUNCTION__);
				if (channelCfg.signalProperties.isReflexive)
				{

					if (fn_process_signal_data)
					{
						std::string text;
						channelCfg.signalProperties.toString(text, consumerPtr, consumerSize, sig::SignalStage::Consumer);
						spdlog::debug("P{}, Signal data ({} bytes) from transformer for {}: {}\n", header.packetId, consumerSize, channelCfg.name, text.c_str());
						fn_process_signal_data(consumerPtr, consumerSize, header);
					}

					if (channelCfg.transformer_to_local_client) {
						spdlog::debug("Sending transformer packet {} to {} local consumers", header.packetId, channelCfg.consumers.size());
						for (auto& consumer : channelCfg.consumers) {
							consumer->fnProcessSignalData(consumerPtr, consumerSize, header);
						}
					}
					else {
						spdlog::debug("Transformed packet {} not going to {} consumers!", header.packetId, channelCfg.name);

					}

					// Currently, lets not have transformers sending information to themselves
					/*
					for (auto& transformer : channelCfg.transformer_inputs) {
						transformer->fnProcessSignalData(consumerPtr, consumerSize, header);
					}
					*/

				}
			}
		}
	}



	void ClientState::_process_outgoing_signal(SignalType signalType, std::vector<net::SignalData>& signalData)
	{
		if (signalType == SignalType::Control) {
			spdlog::error("_process_outgoing_signal has bogus signaltype {}", (int)signalType);
			return;
		}

		
		for (int sigIdx = 0; sigIdx < signalData.size(); ++sigIdx)
		{
			spdlog::debug("Outgoing signal with index {} and type {}", sigIdx, (int)signalType);
			auto& cache = (signalType == SignalType::Environment) ? caches_env_mt[sigIdx] : caches_mt[sigIdx];
			auto& cache2 = (signalType == SignalType::Environment) ? caches2_env_mt[sigIdx] : caches2_mt[sigIdx];

			const auto& channelCfg = scene_runtime_data.SignalConfigFromIndexAndType(sigIdx, signalType);
			// Allocate cache memory, with enough to support a header in front
			const int hs = sizeof(sig::SignalMetadata);
			cache.resize(channelCfg.signalProperties.producerFormMaxSize + hs);

			// get producer signal and place it at an offset, so we have space for the header
			auto& signal_time = time_points_mt[sigIdx];
			//std::fill(cache.begin(), cache.end(), 0);

			auto& producer_future = (signalType == SignalType::Environment)? env_producer_futures[sigIdx] : producer_futures[sigIdx];
			//auto& transformer_future = transformer_futures[sigIdx];

			bool request_data = false; // should we spawn a task to request data?
			bool process_data = false; // do we have valid data to process?

			// if the data is completely invalid, we need to request new data
			if (!producer_future.valid())
			{
				request_data = true;
			}
			// if the data is ready, we need to request new data AND we have to process the data
			else if (producer_future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
			{
				request_data = true;
				process_data = true;
			}
			
			// First process the data
			if (process_data)
			{
				auto numBytesRead = producer_future.get();
				assert(numBytesRead <= channelCfg.signalProperties.producerFormMaxSize);
				bool hasNewSignal = numBytesRead > 0;
				if (hasNewSignal)
				{
					// if we need to keep the state
					auto& stateData = signalData[sigIdx].data;
					int stateSize = 0;
					if (channelCfg.signalProperties.keepState)
					{
						// state is as big as the number of bytes read
						stateData.resize(channelCfg.signalProperties.stateFormMaxSize);
						// transform to state
						stateSize = channelCfg.signalProperties.producerToState(cache.data() + hs, numBytesRead, stateData.data(), stateData.size(), nullptr);
					}

					int networkSize = numBytesRead + hs;
					if (!channelCfg.signalProperties.isPassthru && !channelCfg.transformer_to_network)
					{
						cache2.resize(channelCfg.signalProperties.networkFormMaxSize + hs);
						int stateSize = 0;
						if (channelCfg.signalProperties.keepState)
						{
							// transform state to network
							networkSize = channelCfg.signalProperties.stateToNetwork(stateData.data(), stateSize, cache2.data() + hs, cache2.size() - hs, nullptr);
						}
						else {// straight to network!

							networkSize = channelCfg.signalProperties.producerToNetwork(cache.data() + hs, numBytesRead, cache2.data() + hs, cache2.size() - hs, nullptr);
						}
						cache2.resize(networkSize + hs);
					}
					else {
						cache.resize(numBytesRead + hs);
					}


					spdlog::trace("Producer, sigIdx {}, signal size {}, sigData size {} received without metadata", sigIdx, numBytesRead, stateData.size());
					const auto& packetVec = channelCfg.signalProperties.isPassthru ? cache : cache2;
					/*
					if (signalType == SignalType::Client) {
						if (numBytesRead > 64) {
							std::stringstream ss = std::stringstream("");
							ss << std::hex;

							for (int i = 0; i < 64; i++) {
								ss << (int)cache.data()[i] << " ";
							}
							spdlog::info("Client.cpp produce: sigdump: {}", ss.str());
						}
					}
					*/
					sig::SignalMetadata& header = *(sig::SignalMetadata*)cache.data();
					header = sig::SignalMetadata{ signal_time, signalData[sigIdx].sentCounter++, userIdx, uint8_t(sigIdx), (uint8_t)signalType };
			

					// Telemetry for PRODUCER data
					auto time_now = sig::time_now();
					assert(time_now >= signal_time); // signal cannot have been produced in the future
					signalData[sigIdx].timePoints.push_back(time_now);
					if (signalData[sigIdx].packetIds.empty())
						signalData[sigIdx].packetIds.push_back(header.packetId);
					RegisterPacketToTracker(header, signal_time, "SEND");
					// send message
					auto packet = make_packet(packetVec, channelCfg.signalProperties.isReliable);
					
					auto enet_channel_id = signalType == SignalType::Client ? 0 : sigIdx;
					//auto enet_channel_id = signalType == SignalType::Client ? 0 : sigIdx;


					auto peer_server = server_peer_per_channel[signalType == SignalType::Client ? 2 + sigIdx : 1];
					spdlog::trace("Sending produced packet size {} as user {}\n", packet->dataLength, header.userIdx);
					auto err = enet_peer_send(peer_server, enet_channel_id, packet);



					if (err != 0)
						spdlog::error("send_msg error!");

					// By default (if passthru), set the consumer data as the producer data
					const uint8_t* consumerPtr = cache.data() + hs;
					int consumerSize = numBytesRead;
					// Otherwise, transform from cache2 (that stores the network form)
					if (!channelCfg.signalProperties.isPassthru)
					{
						cache.resize(channelCfg.signalProperties.consumerFormMaxSize);

						consumerSize = channelCfg.signalProperties.networkToConsumer(cache2.data() + hs, cache2.size() - hs, cache.data(), cache.size(), nullptr);
					}

					spdlog::trace("Packet {} outgoing via {}", header.packetId, __FUNCTION__);

					if (channelCfg.signalProperties.isReflexive)
					{
						if (fn_process_signal_data && (!channelCfg.transformed_input_only))
						{
							std::string text;
							channelCfg.signalProperties.toString(text, consumerPtr, consumerSize, sig::SignalStage::Consumer);
							spdlog::trace("client.cpp: Signal data ({} bytes) from producer for {}: {}\n", consumerSize, channelCfg.name, text.c_str());
							fn_process_signal_data(consumerPtr, consumerSize, header);
						}

						if (!channelCfg.transformer_to_local_client) {
							spdlog::trace("client.cpp:Outputing Producer packet {} of size {} to {} local consumers", header.packetId, consumerSize, channelCfg.consumers.size());
							for (auto& consumer : channelCfg.consumers) {
								// Transformers shouldn't be eating their own shit
								if (!consumer->is_transformer)
									consumer->fnProcessSignalData(consumerPtr, consumerSize, header);
								else
									spdlog::debug("Possible output to a transformer in the consumer list");
								//consumer->fnProcessSignalData(consumerPtr, consumerSize, header);
							}
						}						
						for (auto& transformer : channelCfg.transformer_inputs) {
							transformer->fnProcessSignalData(consumerPtr, consumerSize, header);
						}
						

					}

				}
			}
			if (request_data && channelCfg.producer > 0)
			{
				spdlog::trace("Sending Producer thread to pool");
				
				auto& getDataFn = channelCfg.producer->fnGetSignalData;
				producer_future = signal_producer_thread_pool.submit_task([getDataFn, &cache, & signal_time] {
					return getDataFn((uint8_t*)cache.data() + hs, std::ref(signal_time) );
					}	
				);
			}
		}
	}

	
	// TODO: merge with _process_outgoing_Signal
	void ClientState::send_signal(const uint8_t* stream, int numBytesProduced, const sig::SignalMetadata& sigMeta)
	{
		int sigIdx = sigMeta.sigIdx;
		auto signalType = (SignalType)sigMeta.sigType;
		auto& signalData = world.environment.signals;

		auto& cache = cache_st;
		auto& cache2 = cache2_st;
		spdlog::info("Outgoing signal with index {} and type {}\n", sigMeta.sigIdx, (int)sigMeta.sigType);
		const auto& channelCfg = scene_runtime_data.SignalConfigFromIndexAndType(sigMeta.sigIdx, (SignalType)sigMeta.sigType);
		// Allocate cache memory, with enough to support a header in front
		const int hs = sizeof(sig::SignalMetadata);
		cache.resize(channelCfg.signalProperties.producerFormMaxSize + hs);

#if 0	// 
		// get producer signal and place it at an offset, so we have space for the header
		sig::time_point signal_time;
		auto numBytesRead = channelCfg.producer->fnGetSignalData(producedStreamPtr, signal_time);


		assert(numBytesRead <= channelCfg.signalProperties.producerFormMaxSize);
		bool hasNewSignal = numBytesRead > 0;
		if (hasNewSignal)
#else
		memcpy(cache.data() + hs, stream, numBytesProduced);
#endif
		{
			// if we need to keep the state
			auto& stateData = signalData[sigMeta.sigIdx].data;
			int stateSize = 0;
			if (channelCfg.signalProperties.keepState)
			{
				// state is as big as the number of bytes read
				stateData.resize(channelCfg.signalProperties.stateFormMaxSize);
				// transform to state
				stateSize = channelCfg.signalProperties.producerToState(cache.data() + hs, numBytesProduced, stateData.data(), stateData.size(), nullptr);
			}

			int networkSize = numBytesProduced + hs;
			if (!channelCfg.signalProperties.isPassthru)
			{
				cache2.resize(channelCfg.signalProperties.networkFormMaxSize + hs);
				int stateSize = 0;
				if (channelCfg.signalProperties.keepState)
				{
					// transform state to network
					networkSize = channelCfg.signalProperties.stateToNetwork(stateData.data(), stateSize, cache2.data() + hs, cache2.size() - hs, nullptr);
				}
				else // straight to network!
				{
					networkSize = channelCfg.signalProperties.producerToNetwork(cache.data() + hs, numBytesProduced, cache2.data() + hs, cache2.size() - hs, nullptr);
				}
				cache2.resize(networkSize + hs);
			}
			else {
				cache.resize(numBytesProduced + hs);
			}

			const auto& packetVec = channelCfg.signalProperties.isPassthru ? cache : cache2;

			sig::SignalMetadata& header = *(sig::SignalMetadata*)cache.data();
#if 0
			header = sig::SignalMetadata{ signal_time, signalData[sigIdx].sentCounter++, userIdx, uint8_t(sigIdx), (uint8_t)signalType };
#else
			header = sigMeta;
#endif
			// send message
			auto packet = make_packet(packetVec, channelCfg.signalProperties.isReliable);
			auto enet_channel_id = signalType == SignalType::Client ? 0 : sigIdx;
			auto peer_server = server_peer_per_channel[signalType == SignalType::Client ? 2 + sigIdx : 1];
			spdlog::info("sending packet size {} as user {}\n", packet->dataLength, header.userIdx);
			auto err = enet_peer_send(peer_server, enet_channel_id, packet);

			if (err != 0)
				spdlog::error("send_msg error!");

			// By default (if passthru), set the consumer data as the producer data
			const uint8_t* consumerPtr = cache.data() + hs;
			int consumerSize = numBytesProduced;
			// Otherwise, transform from cache2 (that stores the network form)
			if (!channelCfg.signalProperties.isPassthru)
			{
				cache.resize(channelCfg.signalProperties.consumerFormMaxSize);
				consumerSize = channelCfg.signalProperties.networkToConsumer(cache2.data() + hs, cache2.size() - hs, cache.data(), cache.size(), nullptr);
			}
			
			// Env signals should not go back to us (that's where they come from!)
			if (signalType == SignalType::Client && channelCfg.signalProperties.isReflexive)
			{
				if (fn_process_signal_data)
				{
					std::string text;
					channelCfg.signalProperties.toString(text, consumerPtr, consumerSize, sig::SignalStage::Consumer);
					//spdlog::info("Signal data for {}: {}\n", channelCfg.name, text.c_str());
					fn_process_signal_data(consumerPtr, consumerSize, header);
				}

				spdlog::debug("Outputing Transformerd packet {} of size {} to {} local consumers", header.packetId, consumerSize, channelCfg.consumers.size());
				for (auto& consumer : channelCfg.consumers) {

					consumer->fnProcessSignalData(consumerPtr, consumerSize, header);
				}
			}
		}
	}

	bool ClientState::initialize(const cfg::Client& cfg)
	{

		spdlog::info("## Dancegraph Native Client Built on {} ##", __TIMESTAMP__);

		
		// start a task to get our offset to the NTP server
		std::future<float> time_offset_ms_ntp_result;
		
		if (!cfg.ignore_ntp)
			time_offset_ms_ntp_result = std::async(net::time_offset_ms_ntp);

		srand(time(0));
		uniqueID.populate(cfg.address.port);
		spdlog::info("Populated Client ID: {}", dancenet::rangeToString<std::array<char,8>, char>(uniqueID.value, [](std::stringstream& ss, const char & c) {
			ss << std::hex << static_cast<int>(c & 0xff) << ":";
		}));
		

		// add the local appdata to the DLL search path so that we can put dlls there for modules to find

		const char* dgap = (net::cfg::DanceGraphAppDataPath() + std::string("/modules")).c_str();
		SetDllDirectoryA((LPCSTR) dgap);
		spdlog::info("Adding {} to DLL search path", dgap);
		// Make a copy of the cfg::Client for future reference
		config = cfg;
		
		

		if (!scene_runtime_data.Initialize(cfg.scene, cfg.role, cfg.producer_overrides, cfg.user_signal_consumers, cfg.transformers, cfg.include_ipc_consumers, true))
		{
			spdlog::error("Scene runtime data initialization failed {}\n", cfg.scene.c_str());
			return false;
		}

		auto num_user_signals = int(scene_runtime_data.user_signals.size());
		auto num_env_signals = int(scene_runtime_data.env_signals.size());

		start_time_point = sig::time_now();

		address = make_address(cfg.address);
		spdlog::info("Address for multiport is {}:{}", cfg.address.ip, cfg.address.port);
		spdlog::info("Single port operation is {}", config.single_port_operation);
		
		bool ok;
		if (!config.single_port_operation) {
			bool ok = create_hosts_multiport(hosts, address, num_user_signals, num_env_signals, false);
			if (!ok)
				return false;

		}
		else {			
			// All the hosts entries point to our single host
			int numPorts = num_user_signals + 2;
			hosts.resize(numPorts);

			int maxChannels = kNumControlSignals > num_env_signals ? kNumControlSignals : num_env_signals;
			ENetHost * host = create_host(& address, maxChannels, true);
			for (int i = 0; i < numPorts; i++) {
				hosts[i] = host;
			}
		}


		world.environment.signals.resize(num_env_signals);

		ENetAddress server_address = make_address(cfg.server_address);
		ENetEvent event;

		spdlog::info(  "Connecting to {}:{}", cfg.server_address.ip, cfg.server_address.port);

		/* Initiate the connection, allocating the two channels 0 and 1. */
		server_peer_per_channel.resize(ChannelNum(num_user_signals));

		if (!connect_to_host_multiport(server_peer_per_channel, hosts, server_address, num_env_signals))
			return false;

		// Connect to server, for each channel
		for (int iHost = 0; iHost < hosts.size(); ++iHost)
		{

			spdlog::debug("Connection attempt on {}", hosts[iHost]->address);
			int ehs_ret = enet_host_service(hosts[iHost], &event, 10000);
			/* Wait up to 5 seconds for the connection attempt to succeed. */
			if (ehs_ret >= 0 &&
				event.type == ENET_EVENT_TYPE_CONNECT)
			{
				spdlog::info("Connection to {} from host {} succeeded.", server_address, iHost);
			}
			else
			{
				/* Either the 5 seconds are up or a disconnect event was */
				/* received. Reset the peer in the event the 5 seconds   */
				/* had run out without any significant event.            */
				for (auto& peer_server : server_peer_per_channel)
					if (peer_server != nullptr)
						enet_peer_reset(peer_server);

				if (ehs_ret < 0) 
					spdlog::error("Connection to {}:{} enet_host_service failed: {}", cfg.server_address.ip.c_str(), cfg.server_address.port, ehs_ret);
				else {
					switch (event.type) {
					case ENET_EVENT_TYPE_DISCONNECT:
						spdlog::error("Connection to {}:{}, failed by disconnect", cfg.server_address.ip.c_str(), cfg.server_address.port); break;
					case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
						spdlog::error("Connection to {}:{}, failed by timeout", cfg.server_address.ip.c_str(), cfg.server_address.port); break;
					default:
						spdlog::error("Connection to {}:{}, unknown failure {}/{}", cfg.server_address.ip.c_str(), cfg.server_address.port, ehs_ret, event.type); break;
					}
				}
				
				return false;
			}
		}

		msg::NewConnection m;
		
		m.time_offset_ms_ntp = time_offset_ms_ntp;
		m.address = address;
		m.address.port = address.port;
		
		strcpy(m.name.data(), cfg.username.c_str());

		m.id = uniqueID;
		spdlog::debug("cfg.ignore_ntp is {}", cfg.ignore_ntp);
		if (cfg.ignore_ntp) {
			// If we're doing without ntp servers, just assume latency is zero
			m.time_offset_ms_ntp = 0.0;
		}
		else {
			// By now, expect to have the result. Use std::chrono::duration<float, std::milli> to convert for time math
			// So, store it locally AND put it in the message, just before we send it
			
			m.time_offset_ms_ntp = time_offset_ms_ntp_result.get();
		}

		send_control_msg(m, sig::time_now());
		
		spdlog::info("There are {} server chans",server_peer_per_channel.size());
		for (int i = 0; i < server_peer_per_channel.size(); i++) {
			auto ppc = server_peer_per_channel[i];
			if (ppc != nullptr) {
				spdlog::info("Server, ppc {}, val {} -> {}", i, ppc->host->address, ppc->address);
			}
			else {
				spdlog::info("Server, ppc {} null", i);
			}
		
		}

		// THREAD POOL INIT
		// Initialize thread pool with how many producers we will call (one for each signal + one for each consumer) + env signals might be produced too
		
		auto num_producer_signals = net::cfg::Root::instance().scenes.at(config.scene).user_roles.at(config.role).user_signals.size();
		// num_env_signals is already defined

		int transformer_count = config.transformers.size();
		spdlog::info("Transformer count is {}", transformer_count);
		
		int future_count = num_producer_signals;
		int env_future_count = num_env_signals;

		int trans_future_count = num_producer_signals;

		signal_producer_thread_pool.reset(future_count + env_future_count);
		signal_transformer_thread_pool.reset(future_count + env_future_count);
		// initialize futures with the user signals that the scene will use (this is for easier indexing)
		producer_futures.resize(future_count);

		caches_mt.resize(future_count);
		caches_xform_mt.resize(trans_future_count);

		caches2_xform_mt.resize(trans_future_count);
		caches2_mt.resize(future_count);

		env_producer_futures.resize(env_future_count);
		caches_env_mt.resize(future_count);
		caches_env_xform_mt.resize(trans_future_count);

		caches2_env_xform_mt.resize(trans_future_count);
		caches2_env_mt.resize(future_count);

		time_points_mt.resize(future_count);

		transformer_futures.resize(future_count);

		is_initialized = true;
		return true;
	}

	void ClientState::deinitialize()
	{
		for (auto peer : server_peer_per_channel)
			enet_peer_disconnect(peer, 0);
		server_peer_per_channel.resize(0);
		for (auto host : hosts)
			enet_host_destroy(host);
		hosts.resize(0);

		scene_runtime_data.Shutdown();

	}

	void ClientState::run(const cfg::Client& cfg, bool (* callback_fn)())

	{
		if(!initialize(cfg))
			return;
		spdlog::info("Run: Single port is now {}", cfg.single_port_operation);		
		spdlog::info("User signal Size is {}", scene_runtime_data.user_signals.size());
		for (int idx = 0; idx < scene_runtime_data.user_signals.size(); idx++) {
			const auto& channelCfg = scene_runtime_data.SignalConfigFromIndexAndType(idx, SignalType::Client);
			spdlog::info(  "Consumer count {} is {}", idx, channelCfg.consumers.size());
		}

		bool run = true;
		while (run) {
			tick();
			if (callback_fn != nullptr) {
				run = callback_fn();
			}
		}
		deinitialize();
	}


}