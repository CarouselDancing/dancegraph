#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <map>

#include <spdlog/spdlog.h>

#include <nlohmann/json.hpp>

#include <iostream>

#include "config_runtime.h"

namespace net
{
	// Configuration for a signal module: the configuration and a producer
	struct ModuleConfig

	{
		std::string config;
		std::string producer;
	};

	// Signals required for a specific role
	struct UserRoleConfig
	{
		std::vector<std::string> user_signals;
		std::vector<std::string> env_signals;
	};

	// A scene is a collection of user roles that clients can potentially join as
	struct SceneConfig
	{
		std::unordered_map<std::string, UserRoleConfig> user_roles;
	};

	// modules used for the consumer and adapter side, for client or environment signal sets
	struct ModuleAdapterConfig
	{
		std::string consumer;
		std::string adapter;
	};

	// Config for a particular adapter, such as "unity-ipc"
	struct AdapterConfig
	{
		// A single module for all user signals
		ModuleAdapterConfig user_signal;
		// For each scene, a module for environment signals
		std::unordered_map< std::string, std::string> scene_to_env_signal;
	};

	// Database of modules, scenes and adapters
	struct ModuleDatabase
	{
		std::unordered_map<std::string, ModuleConfig> signal_modules;
		std::unordered_map<std::string, std::string> consumer_modules;
		std::unordered_map<std::string, SceneConfig> scenes;

		// keys could be "unity-ipc" or "unreal-ipc"
		std::unordered_map<std::string, AdapterConfig > adapters;

		// given a module name and the path to all modules, construct a filename. Later on could take system architecture into account
		static std::string ModulePath(const std::string& name, const std::string& module_path);
	};

	// The configuration for the server, for a given scenario
	struct ServerConfig
	{
		// Server address
		Address address;
		// The path to all modules
		//std::string module_path;
		// filename to the module database
		//std::string module_database;
		// scene to use
		std::string scene;
	};

	// Data per signal that we wish to override
	struct ClientSignalOverride
	{		
		// json filename with overridden producer options
		std::string producer_options;
		// json filename with overridden config options
		std::string config_options;
		// all other consumers here, besides any ones dictated by adapter
		std::vector<std::string> extra_consumers;
	};

	struct ClientSignalConfig {
		// The signal's type
		std::string type;
		std::string producer;
		std::vector<std::string> consumers;

		// optional overrides to the default signals/configurations
		std::unordered_map<std::string, ClientSignalOverride> signal_overrides;

	};


	struct TransformerConfig {
		std::vector<std::string> inputs; // Signals to be consumed
		std::string outputs; // Signal to be produced
		//std::map<std::string, nlohmann::json> options; // An options set
	};

	struct TransformersMapConfig {
		std::map < std::string, TransformerConfig> transformer;
	};

	struct ClientListenerCommonConfig
	{
		// friendly name, for display/IPC/etc uses
		std::string name;
		// network address of this client/listener
		Address address;
		// network address of the server to connect to
		Address server_address;
		// The path to modules
		//std::string module_path;
		// filename to the module database
		//std::string module_database;

		// The data for each type of user signal being sent to the consumer
		std::vector<ClientSignalConfig> user_signals;

		// The data for each type of user signal being sent to the consumer
		std::vector<ClientSignalConfig> env_signals;
		
		// scene to connect to
		std::string scene;
		// adapter for the scene
		std::string adapter;
		
		std::map<std::string, TransformerConfig> transformer;

		ClientSignalConfig* GetClientSignalConfigFromName(const std::string name, std::vector<ClientSignalConfig> & signal_set)
		 {
				
			for (int i = 0; i < signal_set.size(); i++) {
				spdlog::info(  "Checking sigtype {} versus {}", signal_set[i].type, name);
				if (signal_set[i].type == name)
					return &signal_set[i];
			}
			return nullptr;
		}
		ClientSignalConfig* GetClientEnvSignalConfigFromName(const std::string name) {
			return GetClientSignalConfigFromName(name, env_signals);
		}
		ClientSignalConfig* GetClientUserSignalConfigFromName(const std::string name) {
			return GetClientSignalConfigFromName(name, user_signals);
		}

		ClientSignalConfig* GetTransformerConfigFromName(const std::string name) {
			
		}
	};
	
	// The configuration for the client
	struct ClientConfig
	{
		ClientListenerCommonConfig common;
		// this defines the set of client/env signals that we'll be using
		std::string user_role;

        // The set of clients whose signals we'll be listening to
		std::vector<std::string> clients;

	};

	struct ListenerConfig
	{
		ClientListenerCommonConfig common;
		// The set of signals we're interested in listening

		std::vector<std::string> signals;
		// The set of clients whose signals we're interested in
		std::vector<std::string> clients;
	};

	ServerConfig loadServerConfig(const std::string& filename);
	ClientConfig loadClientConfig(const std::string& filename);
	ListenerConfig loadListenerConfig(const std::string& filename);
	ModuleDatabase loadModuleDatabase(const std::string& filename);

	ServerConfig loadServerConfig(nlohmann::json& js);
	ClientConfig loadClientConfig(nlohmann::json& js);
	ListenerConfig loadListenerConfig(nlohmann::json& js);
	ModuleDatabase loadModuleDatabase(nlohmann::json& js);
	TransformerConfig loadTransformerConfig(nlohmann::json& js);


	nlohmann::json saveServerConfig(const ServerConfig& cfg);
	nlohmann::json saveListenerConfig(const ListenerConfig& cfg);

	// Temp code to be able to instantly load scene configs from the .json file, located in modules/net/scenes
	const std::unordered_map<std::string, SceneConfig> SceneConfigs();
}