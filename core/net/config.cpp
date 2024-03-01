#include "config.h"

#include <iostream>
#include <unordered_set>

#include <nlohmann/json.hpp>

#include <core/common/utility.h>

#include "net.h"


using namespace nlohmann;

namespace net
{
	//NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Address, ip, port);
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ModuleConfig, producer, config);
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(UserRoleConfig, user_signals, env_signals);
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SceneConfig, user_roles);
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ModuleAdapterConfig, consumer, adapter);
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AdapterConfig, user_signal, scene_to_env_signal);
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ModuleDatabase, signal_modules, consumer_modules, scenes, adapters);	
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ServerConfig, address, scene);
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ClientSignalOverride, producer_options, config_options, extra_consumers);	
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ClientSignalConfig, type, producer, consumers, signal_overrides);
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TransformerConfig, inputs, outputs);	
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ClientListenerCommonConfig, name, address, server_address, scene, adapter, env_signals, user_signals, transformer);
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ClientConfig, common, user_role);
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ListenerConfig, common, signals, clients);

	template<class T>
	T loadConfig(const std::string& filename)
	{
		spdlog::info(  "LoadConfig string:{}", filename);
		auto text = dancenet::readTextFile(filename);
		auto j = json::parse(text);
		T obj;
		j.get_to(obj);
		return obj;
	}

	template <class T>
	T loadConfig(const json& jstext) {
		spdlog::info(  "LoadConfig js"  "");
		T obj;
		jstext.get_to(obj);
		return obj;
	}

	template <class T>
	json saveConfig(const T& obj) {
		return json(obj);
	}


	ServerConfig loadServerConfig(const std::string& filename) { 			
		return loadConfig<ServerConfig>(filename); 
	}

	ClientConfig loadClientConfig(const std::string& filename) { return loadConfig<ClientConfig>(filename); }
	ListenerConfig loadListenerConfig(const std::string& filename) { return loadConfig<ListenerConfig>(filename); }

	ModuleDatabase loadModuleDatabase(const std::string& filename) { 		
		return loadConfig<ModuleDatabase>(filename); 
	}

	std::string ModuleDatabase::ModulePath(const std::string& name, const std::string& module_path)
	{
		return module_path + "/" + name + ".dll";
	}

	ServerConfig loadServerConfig(nlohmann::json& jscfg) { 					
		return loadConfig<ServerConfig>(jscfg); 
	}

	ClientConfig loadClientConfig(nlohmann::json& jscfg) { return loadConfig<ClientConfig>(jscfg); }
	
	ListenerConfig loadListenerConfig(nlohmann::json& jscfg) { return loadConfig<ListenerConfig>(jscfg); }

	ModuleDatabase loadModuleDatabase(nlohmann::json& jscfg) { return loadConfig<ModuleDatabase>(jscfg); }

	ModuleConfig loadModuleConfig(nlohmann::json& jscfg) { return loadConfig<ModuleConfig>(jscfg); }

	TransformerConfig loadTransformerConfig(nlohmann::json& jscfg) { return loadConfig <TransformerConfig>(jscfg); }

	nlohmann::json saveServerConfig(const ServerConfig& cfg)
	{
		return saveConfig<ServerConfig>(cfg);
	}

	nlohmann::json saveListenerConfig(const ListenerConfig& cfg)
	{
		return saveConfig<ListenerConfig>(cfg);
	}
}