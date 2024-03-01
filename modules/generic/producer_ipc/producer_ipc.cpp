#include <sstream>
#include <memory>

#define DYNALO_EXPORT_SYMBOLS
#include <dynalo/symbol_helper.hpp>
#include <dynalo/dynalo.hpp>

#include <spdlog/sinks/basic_file_sink.h>

#include <core/net/config.h>

#include "producer_ipc.h"

namespace ipc {


	std::unique_ptr<IPCProducer> g_IPCProducer;

	IPCProducer::IPCProducer(const sig::SignalProperties& sigProp, const std::string name, int bufEntries) : ipcReader(name, sigProp.consumerFormMaxSizeWithHeader, bufEntries) {
	}

	void IPCProducer::shutdown() {

	}

	DYNALO_EXPORT bool DYNALO_CALL SignalProducerInitialize(const sig::SignalProperties& sigProp) {
		int bufEntries;
		std::string bufName;

		spdlog::set_default_logger(sigProp.logger);

		nlohmann::json propJSON = nlohmann::json::parse(sigProp.jsonConfig);
		spdlog::info("prod_ipc Producer Initializing with options {}", sigProp.jsonConfig);
		try {
			/*
			propJSON.at(sigProp.signalType).at("ipcBufferEntries").get_to(bufEntries);
			propJSON.at(sigProp.signalType).at("ipcInBufferName").get_to(bufName);
			*/
			propJSON.at("ipcBufferEntries").get_to(bufEntries);
			propJSON.at("ipcInBufferName").get_to(bufName);


		}
		catch (nlohmann::json::exception e) {
			// TODO: Need better fallback behaviour
			spdlog::critical("Failed to initialize IPC input buffer {}", bufName);
			return false;
		}


		g_IPCProducer = std::unique_ptr<IPCProducer>(new IPCProducer(sigProp, bufName, bufEntries));

		return true;


	}

	int IPCProducer::proc(const uint8_t* mem, sig::time_point& time) {

		ipc::RBError err = ipcReader.read((void*)mem);
		if (err == ipc::RBError::SUCCESS) {
			int bytesRead = ipcReader.get_last_read_size();
			spdlog::debug("Successful read of {} bytes", bytesRead);
			return bytesRead;
		}
		spdlog::debug("Producer IPC read whiff, 0 bytes");
		return 0;
	}


	// Process the signal data (mem: pointer to data, size: number of bytes, time: time that signal was acquired)	
	DYNALO_EXPORT int DYNALO_CALL GetSignalData(uint8_t* mem, sig::time_point& time) {
		return g_IPCProducer->proc(mem, time);

	}



	// Shutdown the signal producer (free resources/caches, etc)
	DYNALO_EXPORT void DYNALO_CALL SignalProducerShutdown() {
		g_IPCProducer->shutdown();
	}

}