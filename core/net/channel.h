#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <map> // Don't care if it's ordered or unordered

#include <sig/signal_common.h>
#include <magic_enum/magic_enum.hpp>
#include <sig/signal_transformer.h>
#include <core/net/config_runtime.h>

namespace sig
{
	struct SignalLibraryProducer;
	struct SignalLibraryConsumer;
	struct SignalLibraryTransformer;
	struct SignalLibraryConfig;
	struct SignalMetadata;
}

namespace net
{
	namespace cfg
	{
		struct Client;
		struct Listener;
		struct Server;
		struct RuntimeSignal;
	}

	struct ModuleDatabase;
	struct ClientListenerCommonConfig;


	// Fixed control signal types
	enum class ControlSignal
	{
		NewConnection=0,
		ConnectionInfo,					// Broadcast to everybody from server
		NewListenerConnection,			// Send from a newly-connected listener client to the server
		NewListenerConnectionResponse,  // Server -> listener, straight after connection request
		RegisterSignal,
		LatencyTelemetry,				// Server <-> listener/client, for syncing purposes
		Delay,							// Server -> listener/client, to inform them of their latency to the server
		PortOverrideInformation,			// Client -> server, to inform the server that the client has specific ports it wants to use for signal types
		TelemetryRequest,				// Server -> client, so that client(s) share their captured telemetry data
		TelemetryClientData,			// client -> server, info about the client itself and the signals it produces
		TelemetryOtherClientData,		// client -> server, info about all clients
		PingRequest,					// Server -> client, insist on a response prior to timeout
		PingReply                       // client -> server, response
	};

	//constexpr int kNumControlSignals = 7;
	constexpr int kNumControlSignals = magic_enum::enum_count<ControlSignal>();


	enum class EnvSignal
	{
		MusicTrackToggle,
		MusicTrackChange,
		MusicTrackGetInfoReq,
		MusicTrackGetInfoReply,
		UserDataReq,
		UserDataReply,
		UserDataSet,
		SceneInfoReq,
		SceneInfoSet,
		SceneInfoReply,
		TestReq,
		TestReply
	};

	constexpr int kNumEnvSignals = magic_enum::enum_count<EnvSignal>();


	// Helper, to be clear on how many channels of communication we need (ports/connections) : one for control signals, one for environment signals and one per user signal
	inline int ChannelNum(int numClientSignals) { return 2 + numClientSignals; }

	// Helper, to get the signal type from the channel ID
	SignalType ChannelIdToSignalType(int channelId);

	// The dynamic libraries for a signal module:
	//		Server needs config
	//		Client needs config, producer and consumers
	//		Listener needs config and consumers
	struct SignalConfig
	{
		// The below help compilation, because of unique_ptr requirements
		SignalConfig();
		~SignalConfig();
		SignalConfig(SignalConfig&&) noexcept = default; // allows use of move ctor for vector<SignalConfigFromIndexAndType>::resize()

		// All parameters are versioned as needed, e.g. zed/v1.0, generic/v1.0/printer or replay, etc
		bool Initialize(const std::string& signal,
			const cfg::RuntimeSignal& producer,
			const std::vector<cfg::RuntimeSignal>& consumers,
			const std::shared_ptr<sig::SignalLibraryTransformer>& transformer_output,
			const std::vector<std::shared_ptr<sig::SignalLibraryTransformer>>& transformer_inputs,
			int signalIndex,
			bool isEnvSignal);

		bool Initialize(const std::string& signal,
			const cfg::RuntimeSignal& producer,
			const std::vector<cfg::RuntimeSignal>& consumers,
			int signalIndex,
			bool isEnvSignal);

		// A way to check if a signal is correctly initialized
		bool IsInitialized() const;
		
		// properties and config
		std::string name;
		sig::SignalProperties signalProperties;
		std::shared_ptr<sig::SignalLibraryConfig> config;

		// Using shared_ptr instead of unique_ptr since transformers have multiple references
		std::shared_ptr<sig::SignalLibraryProducer> producer;
		std::vector<std::shared_ptr<sig::SignalLibraryConsumer>> consumers;

		// Transformers

		// Transformer-as-consumer
		std::vector<std::shared_ptr<sig::SignalLibraryTransformer>> transformer_inputs;

		// Transformer-as-producer
		std::shared_ptr<sig::SignalLibraryTransformer> transformer_output;

		// Does the transformer spit data to the network?
		bool transformer_to_network = false;

		// Does the transformer spit data to the local consumers?
		bool transformer_to_local_client = false;

		
		// This is to indicate that there was an error fetching signal configs
		bool is_error;

	private:
		bool initialized = false;
	};

	// Runtime scene data, useful for server/clients/listeners
	struct SceneRuntimeData
	{
		// what client signals are we using
		std::vector<SignalConfig> user_signals;
		// what environment signals are we using
		std::vector<SignalConfig> env_signals;

		// The actual transformer libraries need to be stashed outside of the SignalConfig lists
		std::vector<std::shared_ptr<sig::SignalLibraryTransformer>> transformers;



		// Common initialisation function for server/client/listener
		// scene_name: frame of reference for everything else
		// user_role: if set, use to load producers. Pick the default ones unless overriden (see below)
		// producer_overrides: if set, allow for a particular signal to use a producer override (generic or specific)
		// consumer_extras: if set, allow for a particular signal to use more consumers (generic or specific)

		bool LoadTransformers(const std::vector<cfg::RuntimeTransformer>& tform_list);
		bool LinkTransformersToSignals(const std::vector<cfg::RuntimeTransformer>& tform_list);

		bool InitializeUserSignals(const std::string& scene_name, const std::string& user_role,
			const std::unordered_map<std::string, net::cfg::RuntimeSignal>& producer_overrides,
			const std::unordered_map<std::string, std::vector<cfg::RuntimeSignal>>& consumer_extras,
			const std::vector<cfg::RuntimeTransformer>& transformers,
			bool include_ipc_consumers,
			bool load_producers
		);

		bool InitializeEnvSignals(const std::string& scene_name,
			const std::unordered_map<std::string, cfg::RuntimeSignal>& producer_overrides,
			const std::unordered_map<std::string, std::vector<cfg::RuntimeSignal>>& consumer_extras,
			const std::vector<cfg::RuntimeTransformer>& transformers,
			bool include_ipc_consumers,
			bool load_producers
		);

		bool Initialize(const std::string& scene_name, const std::string& user_role,
			const std::unordered_map<std::string, net::cfg::RuntimeSignal>& producer_overrides,
			const std::unordered_map<std::string, std::vector<cfg::RuntimeSignal>>& consumer_extras,
			bool include_ipc_consumers,
			bool load_producers);

		bool Initialize(const std::string& scene_name, const std::string& user_role,
			const std::unordered_map<std::string, net::cfg::RuntimeSignal>& producer_overrides,
			const std::unordered_map<std::string, std::vector<cfg::RuntimeSignal>>& consumer_extras,
			const std::vector<cfg::RuntimeTransformer> & transformers,
			bool include_ipc_consumers,
			bool load_producers);


		void Shutdown();

#ifdef OLD_CONFIG
		// Populate the vectors with dynamically-populated SignalConfigs
		bool Initialize(const ModuleDatabase& db,
			const std::string& scene_name,
			const std::string& module_path,
			bool load_producers,
			ClientListenerCommonConfig* cl_cfg);

		bool Initialize(config::ConfigServer & cfg,
			const std::string& scene_name,
			bool load_producers,
			ClientListenerCommonConfig* cl_cfg);
#endif

		// Helpers to get a signal configuration based on data from signal metadata
		const SignalConfig& SignalConfigFromIndexAndType(int sigIdx, SignalType sigType) const;
		const SignalConfig& SignalConfigFromMetadata(const sig::SignalMetadata& sigMeta) const;

		// More helpers, to try to evade crashes
		bool CheckSignalType(int sigIdx, SignalType sigType);
		bool CheckSignalType(const sig::SignalMetadata& sigMeta);

	private:
#ifdef OLD_CONFIG
		config::ConfigServer cfgServ;

		// Make a signal config, based on signal name, adapter, need for producers, etc
		SignalConfig MakeSignalConfig(const std::string& sig_name, 
			const ModuleDatabase& db,
			const std::string& module_path,
			bool load_producer,
			ClientListenerCommonConfig* cl_cfg,
			const std::string& consumer_adapter,
			bool isenvsignal);

		SignalConfig MakeSignalConfig(const std::string& sig_name,
			bool load_producer,
			ClientListenerCommonConfig* cl_cfg,
			const std::string& consumer_adapter,
			bool isenvsignal);
#endif

	};

}