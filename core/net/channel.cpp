#include "channel.h"

#include <array>
#include <vector>
#include <filesystem>
#include <unordered_set>
#include <set>

#include <core/common/utility.h>

#include "message.h"

#include <sig/signal_producer.h>
#include <sig/signal_consumer.h>
#include <sig/signal_transformer.h>
#include <sig/signal_config.h>

#include <nlohmann/json.hpp>

#include <core/common/utility.h>

#include "config.h"
#include "config_master.h"
#include "config_runtime.h"

using json = nlohmann::json;

using namespace dancenet;

namespace net
{
	SignalType ChannelIdToSignalType(int channelId)
	{
		switch (channelId)
		{
		case 0: return SignalType::Control;
		case 1: return SignalType::Environment;
		default: return SignalType::Client;
		}
	}

	SignalConfig::SignalConfig() {};
	SignalConfig::~SignalConfig() {
		
		// Call the consumer and producer shutdowns before they go dark
		if(producer && producer->is_initialized())
			producer->fnSignalProducerShutdown();
		for (auto consumer : consumers)
			if (consumer && consumer->is_initialized())
				consumer->fnSignalConsumerShutdown();

		signalProperties.~SignalProperties(); // call the destructor explicitly, before the DLLs get unloaded! As they are involved in the destruction process
	};

	template<class T>
	std::shared_ptr<T> _LoadSignalLib(const std::string& signal_name, const cfg::Root::versioned_signal_map_t& versioned_signal_map, const std::unordered_map<std::string, cfg::Signal>& module_signals, nlohmann::json& j)
	{
		const auto& master_cfg = cfg::Root::instance();
		// Could be expecting something like generic/null/v1.0 or "camera" which is a name for a versioned zed producer
		const auto parts = string_split(signal_name, '/');

		const net::cfg::SignalModule* module_ptr = nullptr;
		const net::cfg::Signal* signal_ptr = nullptr;

		spdlog::info("Loading signal lib for {}", signal_name);
		if (parts[0] == "generic")
		{
			if (parts.size() != 3) {
				spdlog::error("Config error: {} is incorrect", signal_name);

			}
			assert(parts.size() == 3);
			try {
				signal_ptr = &versioned_signal_map.at(parts[1]).at(parts[2]);
			}
			catch (std::exception e) {
				spdlog::error("Can't find signal module {}", signal_name);
			}
		}
		else
		{
			if (parts.size() != 1) {
				spdlog::error("Config error: {} is incorrect", signal_name);
			}
			assert(parts.size() == 1);
			try {
				signal_ptr = &module_signals.at(signal_name);
			}
			catch(std::exception e) {
				spdlog::error("Can't find signal module {}", signal_name);
				return {};
			}
		}

		// add library-specific properties and load the signal
		const auto signal_lib_filename = master_cfg.make_library_filename(signal_ptr->dll);
		auto lib = std::make_shared<T>(signal_lib_filename.c_str());
		if (!lib->is_initialized())
			return {};
		//j = signal_ptr->opts;
		
		return lib;
	}
	
	template<class T>
	std::shared_ptr<T> _LoadTransformerLib(const std::string& transformer_name)
		// Just a raw dll load
	{
		const auto& master_cfg = cfg::Root::instance();

		std::string dllname = master_cfg.transformers.at(transformer_name).dll;

		const auto signal_lib_filename = master_cfg.make_library_filename(dllname);
		auto lib = std::make_shared<T>(signal_lib_filename.c_str());
		if (!lib->is_initialized())
			return {};

		return lib;
	}



	bool SignalConfig::Initialize(const std::string& signal, const cfg::RuntimeSignal& producer_rt, 
		const std::vector<cfg::RuntimeSignal>& consumers_extra,
		const std::shared_ptr<sig::SignalLibraryTransformer>& tform_output,
		const std::vector<std::shared_ptr<sig::SignalLibraryTransformer>>& tform_inputs,
		int sigIndex,
		bool isEnvSignal)
	{

		for (auto qq : consumers_extra) {
			spdlog::info("Extra {} has opts {}", signal, qq.opts.dump(4));

		}

		const auto& master_cfg = cfg::Root::instance();

		name = signal;
		// name (e.g. zed), version (e.g. v1.0), producer (e.g. replay or generic/undumper), consumers (e.g. generic/printer)


		auto signal_module_ptr = cfg::Root::get_versioned_signal(signal, isEnvSignal ? master_cfg.env_signals : master_cfg.user_signals);

		if (signal_module_ptr == nullptr)
			return false;
		const auto& signal_module = *signal_module_ptr;

		// Load the config library
		const auto config_filename = master_cfg.make_library_filename(signal_module.config.dll);
		config = std::make_shared<sig::SignalLibraryConfig>(config_filename.c_str());
		if (!config->is_initialized())
		{
			spdlog::error("SignalLibraryConfig {} failed to initialize", config_filename);
			return false;
		}
		
		// Get the signal properties from the master config
		nlohmann::json signal_options = {};
		signal_options[signal] = signal_module.opts;
		// TODO: Get the signal properties from the runtime preset, if available and override appropriately

		transformer_output = tform_output;
		// The transformer_to_X flags were pre-set before calling

		for (auto tfi : tform_inputs)
			transformer_inputs.push_back(tfi);

		signalProperties.signalType = signal;

		try {
			signalProperties.isReflexive = signal_module.opts.at("isReflexive");
		}
		catch (json::exception e) {
			// Should we just do nothing here?
			signalProperties.isReflexive = false;
		}

		// Get the properties and the runtime json configuration
		signalProperties.logger = spdlog::default_logger();
		
		signalProperties.jsonConfig = signal_options.dump(4);
		config->fnGetSignalProperties(signalProperties);

		// Add config-specific properties
		nlohmann::json default_sig_opts = json::parse(signalProperties.jsonConfig);


		// Producer (if needed)
		if (!producer_rt.name.empty())
		{
			producer = _LoadSignalLib<sig::SignalLibraryProducer>(producer_rt.name, master_cfg.generic_producers, signal_module.producers, default_sig_opts);

			if (!producer)
			{
				spdlog::error("SignalLibraryProducer {} failed to load", producer_rt.name);
				return false;
			}
			if(!producer_rt.opts.is_null())
				signal_options.update(producer_rt.opts);
		}
		auto producer_name = producer_rt.name;

		// Consumers (if needed)
		for (const auto& consumer_extra : consumers_extra) {
			// Merge the signal_options (from the master config) with the consumer-specific options from the runtime config
			
			if (!consumer_extra.opts.is_null()) {
				signal_options[signal].update(consumer_extra.opts);
			}

			auto consumer = _LoadSignalLib<sig::SignalLibraryConsumer>(consumer_extra.name, master_cfg.generic_consumers, signal_module.consumers, signal_options);
			if (!consumer)
			{
				spdlog::error("SignalLibraryConsumer {} failed to load", consumer_extra.name);
				return false;
			}
			else {
				consumers.push_back(consumer);
			}
		
		}
		signalProperties.jsonConfig = signal_options.dump(4);
		spdlog::info("Signal Properties are now {}", signalProperties.jsonConfig);

		if (producer != nullptr && !producer->fnSignalProducerInitialize(signalProperties))
			return false;

		if (transformer_output != nullptr && !transformer_output->fnSignalProducerInitialize(signalProperties))
			return false;

		for (const auto& consumer : consumers)
		{
			sig::SignalConsumerRuntimeConfig rtcfg;
			
			rtcfg.producer_name = producer_name;
			rtcfg.producer_type = signal;

			if (!consumer->fnSignalConsumerInitialize(signalProperties, rtcfg))
			{
				spdlog::error("SignalLibraryConsumer {} fnSignalConsumerInitialize failed", consumer->path);
				return false;
			}
		}

		for (const auto& transformer : tform_inputs) {
			sig::SignalConsumerRuntimeConfig rtcfg;
			rtcfg.producer_name = producer_name;
			rtcfg.producer_type = signal;			
			
			spdlog::debug("Pre-Xform Signal Properties Index: {}", signalProperties.signalIndex);
			transformer->fnSignalConsumerInitialize(signalProperties, rtcfg);
			spdlog::debug("Post-Xform Signal Properties Index: {}", signalProperties.signalIndex);

			const char * tname = transformer->fnGetName();
			spdlog::error("Got tname {}", tname);
			int queue_size;
			std::vector<std::string> tpair;
			try {
				 
				tpair = string_split(tname, '/');
				if (tpair.size() < 2) {
					spdlog::warn("Can't split transformer name {}", tname);
					return false;
				}

				queue_size = master_cfg.transformers.at(tpair[0]).at(tpair[1]).opts.at(signal).at("signal_queue_size");
			}
			catch(const json::exception & e) {
				spdlog::warn("Transformer {}/{} signal_queue_size option is mandatory", tpair[0], tpair[1]);
				return false;
			}
			catch (const std::exception & e) {
				spdlog::warn("Transformer {}/{} Config read failure: {}", tpair[0], tpair[1], e.what());
				return false;
			}
			net::SignalType sType = isEnvSignal ? net::SignalType::Environment : net::SignalType::Client;

			spdlog::debug("Transformer {} signal {} registered with type {}, idx: {}, size {} and queue {}",
				tname,
				signal,
				(int)signalProperties.channelType,
				signalProperties.signalIndex,
				signalProperties.consumerFormMaxSize,
				queue_size);
			transformer->RegisterSignal(signal, sType, sigIndex, signalProperties.consumerFormMaxSize, queue_size);
		}


		initialized = true;
		return true;
	}


	bool SignalConfig::Initialize(const std::string& signal, const cfg::RuntimeSignal& producer_rt,
		const std::vector<cfg::RuntimeSignal>& consumers_extra,
		int sigIndex,
		bool isEnvSignal) {
		return Initialize(signal,
			producer_rt,
			consumers_extra,
			nullptr,
			{},
			sigIndex,
			isEnvSignal);
	}



	bool SignalConfig::IsInitialized() const
	{
		return initialized;
	}

	// everybody needs this
	bool SceneRuntimeData::InitializeUserSignals(const std::string& scene_name,
		const std::string& user_role,
		const std::unordered_map<std::string,
		cfg::RuntimeSignal>& producer_overrides,
		const std::unordered_map<std::string, std::vector<cfg::RuntimeSignal>>& consumer_extras,
		const std::vector<cfg::RuntimeTransformer>& tformers,
		bool include_ipc_consumers,
		bool load_producers)

	{
		// For the given scene, load all signals that we might use
		const auto& master_cfg = cfg::Root::instance();
		const auto& scene = master_cfg.scenes.at(scene_name);
		// TODO: process env signals

		// Gather all user signal names for the scene, in a sorted set
		std::set<std::string> user_signal_names;
		for (const auto& [user_role_name, user_role] : scene.user_roles)
			for (const auto& user_role_signal : user_role.user_signals)
				user_signal_names.insert(user_role_signal.name);


		// Get the user role info for this scene, if a user_role is provided (e.g. for a client)
		// We will be loading producers for that user role
		//bool load_producers = !user_role.empty();
		auto it_user_role = scene.user_roles.find(user_role);
		if (load_producers && it_user_role == scene.user_roles.end())
		{
			spdlog::error("User role {} not found in scene {}, while loading SceneRuntimeData", user_role, scene_name);
			return false;
		}

		user_signals.resize(user_signal_names.size());
		auto it_user_signal = user_signals.begin();
		for (const auto& user_signal_name : user_signal_names)
		{

			std::vector<cfg::RuntimeSignal> signal_consumer_extras;
			if (include_ipc_consumers)
			{
				auto user_signal_ptr = cfg::Root::get_versioned_signal(user_signal_name, master_cfg.user_signals);
				if (user_signal_ptr == nullptr)
					return false;
				signal_consumer_extras.push_back({ "generic/ipc/v1.0", user_signal_ptr->opts["ipc"] });
			}
			std::string producer_name;
			nlohmann::json producer_options;
			cfg::RuntimeSignal producer_rt;

			if (load_producers)
			{
				// If we have an override, use that, otherwise use the default producer for that signal
				auto it_producer_override = producer_overrides.find(user_signal_name);

				//producer_name = it_producer_override == producer_overrides.end() ? "default" : it_producer_override->second.name;
				producer_name = it_producer_override == producer_overrides.end() ? "default" : it_producer_override->second.name;


				if (it_producer_override == producer_overrides.end()) {
					producer_name = "default";
					producer_options = * cfg::Root::get_versioned_signal(user_signal_name, master_cfg.user_signals);
				}
				else {
					producer_name = it_producer_override->second.name;
					producer_options = it_producer_override->second.opts;
				}
				producer_rt = it_producer_override == producer_overrides.end() ? cfg::RuntimeSignal{"default"} : it_producer_override->second;

			}
			// See if we need to add extra consumers
			auto it_consumer_extras = consumer_extras.find(user_signal_name);
			if (it_consumer_extras != consumer_extras.end()) {				
				signal_consumer_extras = it_consumer_extras->second;				
			}


			std::shared_ptr<sig::SignalLibraryTransformer> transformer_output;
			std::vector<std::shared_ptr<sig::SignalLibraryTransformer>> transformer_inputs;

			bool tformer_output_set = false;
			for (int j = 0; j < tformers.size(); j++) {
				auto tformer = tformers[j];
				auto tfs = string_split(tformer.name, '/');

				auto tconfig = master_cfg.transformers.at(tfs[0]).at(tfs[1]);
				auto tfout = tconfig.output;

				if (user_signal_name == tfout) {

					transformer_output = transformers[j];
					spdlog::debug("Connecting transformer output {} to signal {}, local flag: {}, network flag: {}, exclusivity: {}", tformer.name, user_signal_name, tconfig.local_output, tconfig.network_output, tconfig.exclusive_output);
					it_user_signal->transformer_to_network = tconfig.network_output;
					it_user_signal->transformer_to_local_client = tconfig.local_output;
					it_user_signal->transformed_input_only = tconfig.exclusive_output;
					it_user_signal->transform_multi_prod_max = tconfig.max_productions_per_tick;

					int queueSize;

					try {
						queueSize = tconfig.opts.at(user_signal_name).at("signal_queue_size");
					}
					catch (json::exception e) {
						spdlog::warn("Warning - transformer {} signal history queue for {} not found, defaulting to 1", tformer.name, user_signal_name);
						queueSize = 1;
					}

					//spdlog::debug("Registering user signal {}  ({}, size {}) to transformer {}", user_signal_name, it_user_signal - user_signals.begin(), it_user_signal->signalProperties.consumerFormMaxSize, j);
					//transformers[j]->signalMemory.registerSignal(user_signal_name, SignalType::Client, it_user_signal - user_signals.begin(), it_user_signal->signalProperties.networkFormMaxSize, queueSize);
					tformer_output_set = true;
					spdlog::info("Transformer {}; queue for signal {} set to size {}", tformer.name, user_signal_name, queueSize);

					try {
						it_user_signal->transform_includes_metadata = tconfig.metadata_passthrough;
					}
					catch(std::exception e) {
						it_user_signal->transform_includes_metadata = false;
					}				
				}
				auto siglist = master_cfg.transformers.at(tfs[0]).at(tfs[1]).user_inputs;
				for (auto e_inp : siglist) {
					if (e_inp == user_signal_name) {
						spdlog::debug("Connecting user signal {} to transformer {} input", user_signal_name, tformer.name);
						transformer_inputs.push_back(transformers[j]);
					}
					else {
						spdlog::debug("Checking signal {} vs transformer input {}", user_signal_name, tformer.name);
					}
				}
			}
			if (tformer_output_set == false)
				transformer_output = std::shared_ptr<sig::SignalLibraryTransformer>(nullptr);

			spdlog::info("Initializing {}, {} with {}", user_signal_name, producer_name, producer_options.dump(4));
			
			// Now initialize the signal
			if (!it_user_signal->Initialize(user_signal_name, producer_rt, signal_consumer_extras, transformer_output, transformer_inputs, it_user_signal - user_signals.begin(), false))
			{
				spdlog::error("User signal {} initialization failed", user_signal_name);
				return false;
			}
			++it_user_signal;
		}
		return true;
	}


	bool SceneRuntimeData::InitializeEnvSignals(const std::string& scene_name,
		const std::unordered_map<std::string, cfg::RuntimeSignal>& producer_overrides,
		const std::unordered_map<std::string, std::vector<cfg::RuntimeSignal>>&consumer_extras,
		const std::vector<cfg::RuntimeTransformer>& tformers, // Env signals will only ever be transformer inputs
		bool include_ipc_consumers,
		bool load_producers) {

		// For the given scene, load all signals that we might use
		const auto& master_cfg = cfg::Root::instance();
		const auto& scene = master_cfg.scenes.at(scene_name);
		// TODO: process env signals

		// Gather all user signal names for the scene, in a sorted set
		std::set<std::string> signal_names;

		for (const auto& signal_name : scene.env_signals) {
			signal_names.insert(signal_name);
		}

		//auto it_user_role = scene.user_roles.find(user_role);
		/*
		if (load_producers && it_user_role == scene.user_roles.end())
		{
			spdlog::error("User role {} not found in scene {}, while loading SceneRuntimeData", user_role, scene_name);
			return false;
		}
		*/
		env_signals.resize(scene.env_signals.size());
		
		auto it_signal = env_signals.begin();
		for (const auto& signal_name : signal_names)
		{

			std::vector<cfg::RuntimeSignal> signal_consumer_extras;
			if (include_ipc_consumers)
			{
				auto env_signal_ptr = cfg::Root::get_versioned_signal(signal_name, master_cfg.env_signals);
				if (env_signal_ptr == nullptr)
					return false;
				signal_consumer_extras.push_back({ "generic/ipc/v1.0", env_signal_ptr->opts["ipc"] });
			}



			std::string producer_name;
			nlohmann::json producer_options;
			cfg::RuntimeSignal producer_rt;

			if (load_producers)
			{
				spdlog::info("Loading the Env signal producers");
				// If we have an override, use that, otherwise use the default producer for that signal
				auto it_producer_override = producer_overrides.find(signal_name);

				//producer_name = it_producer_override == producer_overrides.end() ? "default" : it_producer_override->second.name;
				std::string producer_name = it_producer_override == producer_overrides.end() ? "default" : it_producer_override->second.name;


				if (it_producer_override == producer_overrides.end()) {
					producer_name = "default";
					producer_options = *cfg::Root::get_versioned_signal(signal_name, master_cfg.env_signals);
				}
				else {
					producer_name = it_producer_override->second.name;
					producer_options = it_producer_override->second.opts;
				}
				producer_rt = it_producer_override == producer_overrides.end() ? cfg::RuntimeSignal{ "default" } : it_producer_override->second;

			}

			spdlog::info("Producer_rt_opts: {}", producer_rt.opts.dump(4));

			// See if we need to add extra consumers
			auto it_consumer_extras = consumer_extras.find(signal_name);
			if (it_consumer_extras != consumer_extras.end()) {
				signal_consumer_extras = it_consumer_extras->second;
			}

			spdlog::info("Initializing {}, {} with {}", signal_name, producer_name, producer_rt.opts.dump(4));

			std::vector<std::shared_ptr<sig::SignalLibraryTransformer>> transformer_inputs;

			for (int j = 0; j < tformers.size(); j++) {
				auto tformer = tformers[j];
				auto tfs = string_split(tformer.name, '/');
				auto siglist = master_cfg.transformers.at(tfs[0]).at(tfs[1]).env_inputs;
				
				for (auto e_inp : siglist) {
					if (e_inp == signal_name) {
						spdlog::debug("Connecting env signal {} to transformer {} input", signal_name, tformer.name);
						transformer_inputs.push_back(transformers[j]);

						int queueSize;
						try {
							queueSize = master_cfg.transformers.at(tfs[0]).at(tfs[1]).opts.at(signal_name).at("signal_queue_size");
						}	
						catch (json::exception e) {
							spdlog::warn("Warning - transformer {} signal history queue for {} not found, defaulting to 1", tformer.name, signal_name);
							queueSize = 1;
						}
						//spdlog::debug("Registering signal {} for transformer {}", signal_name, j);
						//transformers[j]->signalMemory.registerSignal(signal_name, SignalType::Environment, it_signal - env_signals.begin(), it_signal->signalProperties.producerFormMaxSize, queueSize);
					}
					else {
						spdlog::debug("Checking env signal {} vs {}, transformer input {}", signal_name, e_inp, tformer.name);
					}
				}
			}
	

			// Now initialize the signal
			if (!it_signal->Initialize(signal_name, producer_rt, signal_consumer_extras, nullptr, transformer_inputs,  it_signal - env_signals.begin(), true))
			{
				spdlog::error("Env signal {} initialization failed", signal_name);
				return false;
			}

			++it_signal;
		}

		return true;

	}

	// Just load the transformers into memory first, and sort out the signalling later
	bool SceneRuntimeData::LoadTransformers(const std::vector<cfg::RuntimeTransformer>& tform_list) {
		
		const auto & master_cfg = cfg::Root::instance();
		//const auto& scene = master_cfg.scenes.at(scene_name);

		// Loads the transformers into memory and stashes them in SceneRuntimeData in order to be referred to by other signals
		for (auto& tform : tform_list) {

			auto tnamep = string_split(tform.name, '/');
			if (tnamep.size() != 2) {
				spdlog::debug("Failed to split transformer name: {}", tform.name);
				return false;
			}

			try {
				auto dllname = master_cfg.transformers.at(tnamep[0]).at(tnamep[1]).dll;
				// add library-specific properties and load the signal
				const auto signal_lib_filename = master_cfg.make_library_filename(dllname);
				auto lib = std::make_shared<sig::SignalLibraryTransformer>(signal_lib_filename.c_str());
				if (!lib->is_initialized())
					return false;
				transformers.push_back(lib);

			}

			catch (std::exception e) {
				spdlog::critical("Transformer {} not loadable: {}",tform.name, e.what());
				return false;

			}
		}
		
		

		return true;
	}


	bool SceneRuntimeData::LinkTransformersToSignals(const std::vector<cfg::RuntimeTransformer>& tform_list) {
		// Matches the already-loaded transformers to the already-loaded signals and adds consumer/producers as applicable
		const auto& master_cfg = cfg::Root::instance();

		std::set<std::string> user_signal_names;
		std::set<std::string> env_signal_names;

		for (auto& tform : tform_list) {
			auto tnamep = string_split(tform.name, '/');
			if (tnamep.size() != 2) {
				spdlog::debug("Failed to split transformer name: {}", tform.name);
				return false;
			}

			auto env_inputs = master_cfg.transformers.at(tnamep[0]).at(tnamep[1]).env_inputs;
			auto user_inputs = master_cfg.transformers.at(tnamep[0]).at(tnamep[1]).user_inputs;
			auto output = master_cfg.transformers.at(tnamep[0]).at(tnamep[1]).output;
			
			
		}

		return true;
	}



	bool SceneRuntimeData::Initialize(const std::string& scene_name, const std::string& user_role,
		const std::unordered_map<std::string, net::cfg::RuntimeSignal>& producer_overrides,
		const std::unordered_map<std::string, std::vector<cfg::RuntimeSignal>>& consumer_extras,
		bool include_ipc_consumers,
		bool load_producers) {
		bool r1 = InitializeUserSignals(scene_name, user_role, producer_overrides, consumer_extras, {}, include_ipc_consumers, load_producers);
		if (!r1)
			return false;

		// We may need transformers consuming environment signals
		bool r2 = InitializeEnvSignals(scene_name, producer_overrides, consumer_extras, {}, include_ipc_consumers, load_producers);

		spdlog::trace("No transformer found, using the simple initializer");


		return r2;
	}


	// The initialization scene when there's a transformer involved
	bool SceneRuntimeData::Initialize(const std::string& scene_name, const std::string& user_role,
		const std::unordered_map<std::string, net::cfg::RuntimeSignal>& producer_overrides,
		const std::unordered_map<std::string, std::vector<cfg::RuntimeSignal>>& consumer_extras,
		const std::vector<cfg::RuntimeTransformer> & transformers,
		bool include_ipc_consumers,
		bool load_producers) {

		//const auto& master_cfg = cfg::Root::instance();
		
		bool r0 = LoadTransformers(transformers);
		if (!r0)
			return false;

		bool r1 = InitializeUserSignals(scene_name, user_role, producer_overrides, consumer_extras, transformers, include_ipc_consumers, load_producers);

		if (!r1)
			return false;

		bool r2 = InitializeEnvSignals(scene_name, producer_overrides, consumer_extras, transformers, include_ipc_consumers, load_producers);
		return r2;
		

	}

	const SignalConfig& SceneRuntimeData::SignalConfigFromIndexAndType(int sigIdx, SignalType sigType) const
	{
		
		assert(sigType == SignalType::Client || sigType == SignalType::Environment);
		const auto& sig_configs = sigType == SignalType::Client ? user_signals : env_signals;
		return sig_configs.at(sigIdx);
	}

	const SignalConfig& SceneRuntimeData::SignalConfigFromMetadata(const sig::SignalMetadata& sigMeta) const
	{
		return SignalConfigFromIndexAndType(sigMeta.sigIdx, (SignalType)sigMeta.sigType);
	}


	bool SceneRuntimeData::CheckSignalType(int sigIdx, SignalType sigType) 
	{

		if (sigType == SignalType::Client || sigType == SignalType::Environment) {
			const auto& sig_configs = sigType == SignalType::Client ? user_signals : env_signals;
			return ((int)sigIdx < sig_configs.size());
		}
		else
			return false;
	}

	void SceneRuntimeData::BroadcastLocalUserIndex(uint16_t idx) {
		// Currently just calls the transformer 'setlocaluser' function

		for (auto t : transformers) {
			t->fnSetLocalUserIdx(idx);
		}


	}


	bool SceneRuntimeData::CheckSignalType(const sig::SignalMetadata& sigMeta) 
	{
		return CheckSignalType(sigMeta.sigIdx, (SignalType)sigMeta.sigType);
	}

	void SceneRuntimeData::Shutdown() {
	}


}