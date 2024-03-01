
#include <memory>
#include <cstdlib>

#include <spdlog/sinks/basic_file_sink.h>

#include <ipc/ringbuffer.h>

#include "sig/signal_common.h"
#include <sig/signal_config.h>

//#include <chrono>
#include <modules/env/env_common.h>

#include <sstream>

#include "../dancegraph.h"

#include "env_adapter.h"

namespace dll {

	//extern net::cfg::Root master_cfg;
	template 
	<typename T>
	void populate_with_fallback(T & target, const nlohmann::json & opt, const  std::string & key, const T& defaultValue) {
		
		try {
			target = opt.at(key);
		}
		catch(json::exception e) 
		{
			spdlog::warn("Populated json-sourced value with fallback");
			target = defaultValue;
		}
	}

	//std::unique_ptr<ZedAdapter> zedAdapterp;
	int8_t* envSignalData; // A unique pointer to be freed
	sig::SignalMetadata* envSigMetadata;

	std::unique_ptr<EnvAdapter> envAdapterp;
	
	void env_shutdown() {
		delete[] envSignalData;
	}

	void env_initialize() {

		

		uint32_t bufEntries;
		std::string bufInName;
		std::string bufOutName;
		
		nlohmann::json env_opts, env_globals;

		bool loader = net::cfg::Root().load();
		if (!loader) {
			spdlog::error("Can't load master config");
			return;
		}
		
		
		
		net::cfg::Root master_cfg = net::cfg::Root().instance();



		try {
			env_opts = master_cfg.env_signals.at("env").at("v1.0").opts;
		}
		catch (json::exception e) {
			env_opts = nlohmann::json({});
		}
		catch (std::exception e) {
			env_opts = nlohmann::json({});
		}


		try {
		nlohmann::json env_globals = master_cfg.env_signals.at("env").at("v1.0").globalopts;
		}
		catch (json::exception e) {
			env_globals = nlohmann::json({});
		}
		catch (std::exception e) {
			env_globals = nlohmann::json({});
		}

		env_opts.merge_patch(env_globals);
		
		
		populate_with_fallback<std::string>(bufInName, env_opts, std::string("ipcInBufferEntries"), std::string("Dancegraph_Env_In"));
		populate_with_fallback<std::string>(bufOutName, env_opts, std::string("ipcOutBufferName"), std::string("Dancegraph_Env_Out"));

		// To make the template accept it
		uint32_t annoying_placeholder_variable = 5;
		populate_with_fallback<uint32_t>(bufEntries, env_opts, std::string("ipcBufferEntries"), annoying_placeholder_variable);
/*
		try {
			auto master_cfg = net::cfg::Root();
			auto master_opts = master_cfg.env_signals.at("env").at("1.0").opts;

			bufEntries = master_opts.at("ipcBufferEntries");
			bufInName = master_opts.at("ipcInBufferName");
			bufOutName = master_opts.at("ipcOutBufferName");

		}
		catch (json::exception e) {
			// TODO: Need better fallback options
			spdlog::info("WARNING: Fallback IPC Buffer Count");
			bufEntries = 3;
			bufInName = std::string("IPC_Env_In");
			bufOutName = std::string("IPC_Env_Out");
		}
		catch (std::exception e) {
			spdlog::info("Warning - std exception called {}", e.what());
		}
		*/


		if (bufEntries < 3) {
			spdlog::warn("Buffer entry count too low, setting to 3");
			bufEntries = 3;
		}

		
		

		//int bufSize = sizeof(sig::SignalMetadata) + zed::ZedBodiesCompact::size(bufferProperties.maxBodyCount);
		int bufSize = sizeof(sig::SignalMetadata) + ENV_SIGNAL_MAX_SIZE;
		envSignalData = new int8_t[bufSize];

		spdlog::info("Initializing EnvAdapter with size {}, entries {} and names {} and {}", bufSize, bufEntries, bufInName, bufOutName);
		envAdapterp = std::unique_ptr<EnvAdapter>(new EnvAdapter(bufOutName, bufInName, bufSize, bufEntries));
		

	}

	int EnvAdapter::get_env_msg(char mp[]) {
		// Just copy the message over to C# to be sorted out by Unity
		memcpy((void*)mp, (char*)envSignalData, lastReadSize);

		std::stringstream ss; 		
		for (int i = 0; i < lastReadSize; i++) {
			ss << (int)envSignalData[i] << " ";
		}

		spdlog::info("Signal Data received: {}", ss.str());


		return lastReadSize;
	}

	bool EnvAdapter::send_env_msg(char mp[], int size) {


		memcpy(envSignalData, (void*)mp, size);
		
		ipc::RBError err = ipcWriter.write(mp, size);
		
		return err == ipc::RBError::SUCCESS;
	}

	bool EnvAdapter::read_ipc() {
		ipc::RBError err = ipcReader.read((void*)envSignalData);
		if (err == ipc::RBError::SUCCESS) {
			lastReadSize = ipcReader.get_last_read_size();
			return true;
		}
		else {
			return false;
		}
	}
	/*
	bool EnvAdapter::write_ipc() {
		return false;

	}
	*/

	EnvAdapter::EnvAdapter(std::string nameout, std::string namein, int size, int entries) : ipcReader(nameout, size, entries), ipcWriter(namein, size, entries) {
	}


	EnvAdapter::~EnvAdapter() {
	}


}
