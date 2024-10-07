#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <map>

#include <spdlog/spdlog.h>

#include <nlohmann/json.hpp>

#include <core/common/utility.h>

namespace net
{
	namespace cfg
	{
		const std::string& DanceGraphAppDataPath();

		// Specify a signal name, and optionally a transformer, applied just before the signal gets consumed (e.g. for prediction)
		struct UserRole_UserSignal
		{
			// Name in the form e.g.
			//  zed/v1.0/camera : use the camera producer
			//	zed/v1.0/generic/undumper : use the generic undumper producer against the zed/v1.0 config
			std::string name;
			// If set, use this transformer from the appropriate signal (e.g. zed/v1.0 above)
			std::string transformer; 

			// Whether the signal is bounced back to the user's client
			bool isReflexive;
		};

		// A user role represents a set of user signals. Making the assumption here that each user sees all the environment signals for this scene
		struct UserRole
		{
			std::vector<UserRole_UserSignal> user_signals;
		};

		// A scene is a collection of environment signals and supported user roles
		struct Scene
		{
			std::vector<std::string> env_signals;
			std::unordered_map<std::string, UserRole> user_roles;
		};

		// A signal is specified as a .dll path, and configuration options (that can be handled by the dll)
		struct Signal
		{
			std::string dll;
			//nlohmann::json opts;

		};

		// A signal module is the set of the config signal, and producers/consumers/transformers associated with it
		struct SignalModule
		{
			Signal config;

			nlohmann::json opts;

			// Globalopts function the same as options, but they're separated off, because they alter the actual signal, and must be constant
			// Across network users
			nlohmann::json globalopts;

			std::unordered_map<std::string, Signal> producers;
			std::unordered_map<std::string, Signal> consumers;
			std::unordered_map<std::string, Signal> transformers;
		};

		struct Transformer
		{
			std::string dll;
			std::vector<std::string> user_inputs;
			std::vector<std::string> env_inputs;
			std::string output;
			nlohmann::json opts;
			// Whether the output of the transformer is passed to the local client
			bool local_output;
			// Whether the output of the transformer is passed to the network
			bool network_output;
			// Whether the output signal includes the metadata
			bool metadata_passthrough;

			// Whether this replaces all of the signals of the output type, or merely adds to them
			bool exclusive_output;

			// How many times can we produce in a single tick?
			int max_productions_per_tick;

		};


		struct NetworkingSettings
		{
			double connectionPing = 4;
			double connectionTimeout = 5;
		};

		// The master config contains all of the above
		struct Root
		{
			using versioned_signal_t = std::map<std::string, Signal>;
			using versioned_signal_module_t = std::map<std::string, SignalModule>;
			using versioned_signal_map_t = std::unordered_map<std::string, versioned_signal_t>;
			using versioned_signal_module_map_t = std::unordered_map<std::string, versioned_signal_module_t>;
			
			using versioned_transformer_t = std::map<std::string, Transformer>;
			using versioned_transformer_map_t = std::unordered_map<std::string, versioned_transformer_t>;



			std::string dll_folder;
			NetworkingSettings networking;
			std::unordered_map<std::string, Scene> scenes;
			versioned_signal_map_t generic_producers;
			versioned_signal_map_t generic_consumers;
			versioned_signal_module_map_t user_signals;
			versioned_signal_module_map_t env_signals;
			
			versioned_transformer_map_t transformers;

			std::string make_library_filename(const std::string& name) const;

			Root() = default;

			static const auto& instance() {
				static Root root;
				return root;
			}

			// load from file
			static bool load();
			template<class T>
			static const T* get_versioned_signal(const std::string& signal, const std::unordered_map<std::string, std::map<std::string, T>>& data)
			{
				const auto signal_parts = dancenet::string_split(signal, '/');
				if (signal_parts.size() < 2)
				{
					spdlog::error("Expecting name in versioned form (e.g. zed/v1.0) but got {}", signal);
					return nullptr;
				}
				auto it_outer = data.find(signal_parts[0]);
				if (it_outer == data.end())
				{
					spdlog::error("Name (first) part of versioned form not found: {}", signal);
					return nullptr;
				}
				auto& inner = it_outer->second;
				auto it_inner = inner.find(signal_parts[1]);
				if (it_inner == inner.end())
				{
					spdlog::error("Version (second) part of versioned form not found: {}", signal);
					return nullptr;
				}
				return &it_inner->second;
			}
		};

		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(UserRole_UserSignal, name, transformer, isReflexive);
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(UserRole, user_signals);
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Scene, env_signals, user_roles);
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Signal, dll);
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SignalModule, config, producers, consumers, opts, globalopts);
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(NetworkingSettings, connectionPing, connectionTimeout);
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Transformer, dll, user_inputs, env_inputs, output, opts, local_output, network_output, metadata_passthrough, exclusive_output, max_productions_per_tick);
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Root, dll_folder, networking, scenes, generic_producers, generic_consumers, user_signals, env_signals, transformers);

	}
}