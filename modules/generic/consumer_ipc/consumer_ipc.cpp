#include <sstream>
#include <memory>

#define DYNALO_EXPORT_SYMBOLS
#include <dynalo/symbol_helper.hpp>
#include <dynalo/dynalo.hpp>

#include <spdlog/sinks/basic_file_sink.h>

//#include <core/config.h>

#include "consumer_ipc.h"

namespace ipc {
	

	std::unique_ptr<IPCConsumer> g_IPCConsumer;


	IPCConsumer::IPCConsumer(const sig::SignalProperties &sigProp, const std::string name, int bufEntries) : ipcWriter(name, sigProp.consumerFormMaxSizeWithHeader, bufEntries) {		
	}

	bool IPCConsumer::init(const sig::SignalProperties& sigProp, sig::SignalConsumerRuntimeConfig& cfg) {
		
		assembly_buffer = new char [sigProp.consumerFormMaxSizeWithHeader]();
		return true;

	}


	void IPCConsumer::proc(const uint8_t* mem, int size, sig::SignalMetadata sigMeta) {
		
		static sig::SignalMetadata* sHeader = (sig::SignalMetadata*) g_IPCConsumer->assembly_buffer;


		static int packetCounter = 0;

		if (metadata_passthrough) {
			// Do nothing
		}
		else {
			sHeader->packetId = packetCounter;
			packetCounter += 1;
			sHeader->sigType = sigMeta.sigType;
			sHeader->sigIdx = sigMeta.sigIdx;
			sHeader->acquisitionTime = sig::time_now();
		}

		sHeader->userIdx = sigMeta.userIdx;
		
		memcpy((void*)(assembly_buffer + sizeof(sig::SignalMetadata)), mem, size);
		
		spdlog::info("IPC consumer Queuing up {}+{}={} bytes for write", size, sizeof(sig::SignalMetadata), size + sizeof(sig::SignalMetadata));

		RBError rberr = g_IPCConsumer->ipcWriter.write((void*)assembly_buffer, size + sizeof(sig::SignalMetadata));
		
		if (rberr != RBError::SUCCESS) {
			// TODO : Handle any errors somehow
			spdlog::warn("Write buffer error: {}", ipc::RB_ErrorString(rberr));
		}

		std::stringstream ss;
		for (int i = 0; i < size; i++) {
			ss << (int)mem[i] << " ";
		}
	}

	void IPCConsumer::shutdown() {
		if (assembly_buffer != nullptr)
			delete[] assembly_buffer;
	}

	
	// Initialize the signal consumer (alloc resources/caches, etc)
	DYNALO_EXPORT bool DYNALO_CALL SignalConsumerInitialize(const sig::SignalProperties& sigProp, sig::SignalConsumerRuntimeConfig& cfg) {
		
		int bufEntries;
		std::stringstream ss;
		std::string bufName;


		ss.str("");
		ss << "IPC_ConsumerLog_" << cfg.producer_type << ".txt";

		auto consumer_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(ss.str().c_str());


		nlohmann::json propJSON = nlohmann::json::parse(sigProp.jsonConfig);
		
		try {

			propJSON.at(sigProp.signalType).at("ipcBufferEntries").get_to(bufEntries);
			propJSON.at(sigProp.signalType).at("ipcOutBufferName").get_to(bufName);

		}
		catch (nlohmann::json::exception e) {
			// TODO: Need better fallback options
			spdlog::info("WARNING: Fallback IPC Buffer Count");
			bufEntries = 25;
			ss.str("");
			ss << "Dancegraph_Generic_Out";
			bufName = ss.str();
		}

		spdlog::info("{}: Setting up consumer IPC with entries {} and name {}", sigProp.signalType, bufEntries, bufName);
		g_IPCConsumer = std::unique_ptr<IPCConsumer>(new IPCConsumer(sigProp, bufName, bufEntries));
		g_IPCConsumer->metadata_passthrough = cfg.metadata_override;

		return g_IPCConsumer->init(sigProp, cfg);
	}

	// Process the signal data (mem: pointer to data, size: number of bytes, time: time that signal was acquired)
	DYNALO_EXPORT void DYNALO_CALL ProcessSignalData(const uint8_t* mem, int size, const sig::SignalMetadata& metadata) {
		static int counter = 0;
		spdlog::info("Counter for this instance is {}", counter++);

		g_IPCConsumer->proc(mem, size, metadata);
	}

	// Shutdown the signal consumer (free resources/caches, etc)
	DYNALO_EXPORT void DYNALO_CALL SignalConsumerShutdown() {
		g_IPCConsumer->shutdown();
	}

}