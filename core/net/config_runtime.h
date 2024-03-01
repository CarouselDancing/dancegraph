#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_map>

#include <nlohmann/json.hpp>

namespace net
{
	// IP address and port
	struct Address
	{
		std::string ip;
		int port = 0;
	};
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Address, ip, port);

	namespace cfg
	{
		// The runtime signal is a specification of a signal name (e.g. zed/v1.0/replay) and options for the associated DLL

		struct RuntimeSignal
		{
			std::string name;
			nlohmann::json opts;
		};

		struct RuntimeTransformer
		{
			std::string name;
			nlohmann::json opts;
		};

		// Common data for clients and listeners 
		struct ClientListenerCommon
		{
			// friendly name, for display/IPC/etc uses
			std::string username;
			// network address of this client/listener
			Address address;
			// network address of the server to connect to
			Address server_address;

			// scene to connect to. TODO: this should be retrieved from the server normally, so that we can then decide the role
			std::string scene;

			// Allow a list of consumers to handle an environment signal
			//	  For clients, that's in ADDITION to whatever the scene supports
			std::unordered_map<std::string, std::vector<RuntimeSignal>> env_signal_consumers;
			// Allow a list of consumers to handle a user signal
			//	  For clients, that's in ADDITION to whatever the role supports
			std::unordered_map<std::string, std::vector<RuntimeSignal>> user_signal_consumers;

			// should we include IPC consumers by default? If so, we'll use the SignalProperties IPC related values
			bool include_ipc_consumers = false;

			// Restrict our port usage to a single source port (i.e. don't attempt to make multiple connections)
			bool single_port_operation = false;
			
			// Stop dialing out to ntp for latency corrections. Useful for testing in internet-less situations
			bool ignore_ntp = false;
		};

		// Client also specifies a role
		struct Client : ClientListenerCommon
		{
			// role we're going to join as -- this implies a set of signals
			std::string role;

			std::unordered_map<std::string, RuntimeSignal> producer_overrides;
			std::vector<RuntimeTransformer> transformers;
		};

		// Listener also specifies a list of clients it's interested in
		struct Listener : ClientListenerCommon
		{
			// The set of clients whose signals we'll be listening to
			std::vector<std::string> clients;
			// The set of signals we'll be listening to
			std::vector<std::string> signals;
		};

		// Server specifies its address and the scene to load
		struct Server
		{
			// network address of this client/listener
			Address address;

			// scene to connect to. TODO: this should be retrieved from the server normally, so that we can then decide the role
			std::string scene;

			bool ignore_ntp = false;
		};

		/*
		struct Avatar
		{
			std::string type; // Ybot, Louise, David, etc
			nlohmann::json opts; // Avatar-specific parameters
		};
		*/

		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RuntimeSignal, name, opts);
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RuntimeTransformer, name, opts);
		//NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ClientListenerCommon, username, address, server_address, scene, env_signal_consumers, user_signal_consumers);
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Client, username, address, server_address, scene, env_signal_consumers, user_signal_consumers, role, producer_overrides, include_ipc_consumers, single_port_operation, ignore_ntp, transformers);
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Listener, username, address, server_address, scene, env_signal_consumers, user_signal_consumers, clients, include_ipc_consumers, ignore_ntp, single_port_operation);
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Server, address, scene, ignore_ntp);
		//NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Avatar, type, opts);


		/////////////////////////////////////////////

		struct RuntimePresetsDb
		{
			std::unordered_map<std::string, Server> server;
			std::unordered_map<std::string, Client> client;
			std::unordered_map<std::string, Listener> listener;			
		};

/*
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RuntimePresetsDb, server, client, listener);

		// The top-level runtime config
		struct RuntimeConfigDb
		{
			RuntimePresetsDb presets;

		};

		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RuntimeConfigDb, presets);
*/

		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RuntimePresetsDb, server, client, listener);

	}
}