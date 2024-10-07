using System;
using System.Collections.Generic;





namespace net
{
	// IP address and port
	[Serializable] public class Address
	{
		public string ip;
		public int port=0;
	};

	// Configuration for a signal module: the configuration and a producer
	[Serializable] public class ModuleConfig

	{
		public string config;
		public string producer;
	};

	// Signals required for a specific role
	[Serializable] public class UserRoleConfig
	{
		public string[] user_signals;
		public string[] env_signals;
	};

	// A scene is a collection of user roles that clients can potentially join as
	[Serializable] public class SceneConfig
	{
		public Dictionary<string, UserRoleConfig> user_roles;
	};

	// modules used for the consumer and adapter side, for client or environment signal sets
	[Serializable] public class ModuleAdapterConfig
	{
		public string consumer;
		public string adapter;
	};

	// Config for a particular adapter, such as "unity-ipc"
	[Serializable] public class AdapterConfig
	{
		// A single module for all user signals
		public ModuleAdapterConfig user_signal;
		// For each scene, a module for environment signals
		public Dictionary< string, string> scene_to_env_signal;
	};

	// Database of modules, scenes and adapters
	[Serializable] public class ModuleDatabase
	{
		public Dictionary<string, ModuleConfig> signal_modules;
		public Dictionary<string, string> consumer_modules;
		public Dictionary<string, SceneConfig> scenes;

		// keys could be "unity-ipc" or "unreal-ipc"
		public Dictionary<string, AdapterConfig > adapters;

		// given a module name and the path to all modules, con[Serializable] public class a filename. Later on could take system architecture into account
	};

	// The configuration for the server, for a given scenario
	[Serializable] public class ServerConfig
	{
		// Server address
		public Address address;
		// The path to all modules
		//string module_path;
		// filename to the module database
		//string module_database;
		// scene to use
		public string scene;
	};

	// Data per signal that we wish to override
	[Serializable] public class ClientSignalOverride
	{		
		// json filename with overridden producer options
		public string producer_options;
		// json filename with overridden config options
		public string config_options;
		// all other consumers here, besides any ones dictated by adapter
		public string[] extra_consumers;
	};

	[Serializable] public class ClientSignalConfig {
		// The signal's type
		public string type;
		public string producer;
		public string[] consumers;

		// optional overrides to the default signals/configurations
		public Dictionary<string, ClientSignalOverride> signal_overrides;

	};

	[Serializable] public class ClientListenerCommonConfig
	{
		// friendly name, for display/IPC/etc uses
		public string name;
		// network address of this client/listener
		public Address address;
		// network address of the server to connect to
		public Address server_address;
		// The path to modules
		//string module_path;
		// filename to the module database
		//string module_database;

		// The data for each type of user signal being sent to the consumer
		public ClientSignalConfig[] user_signals;

		// The data for each type of user signal being sent to the consumer
		public ClientSignalConfig[] env_signals;
		
		// scene to connect to
		public string scene;
		// adapter for the scene
		public string adapter;
		
		 {
				
					public return &signal_set[i];
			}
			return nullptr;
		}
		}
		}
	};
	
	// The configuration for the client
	[Serializable] public class ClientConfig
	{
		ClientListenerCommonConfig common;
		// this defines the set of client/env signals that we'll be using
		string user_role;

        // The set of clients whose signals we'll be listening to
		string[] clients;

	};

	[Serializable] public class ListenerConfig
	{
		ClientListenerCommonConfig common;
		// The set of signals we're interested in listening

		string[] signals;
		// The set of clients whose signals we're interested in
		string[] clients;
	};




}
