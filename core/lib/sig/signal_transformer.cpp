
#include "signal_transformer.h"


#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <core/net/config.h>
#include <core/net/config_master.h>


namespace sig {

	struct ExampleTransformer
	{
		bool init(const sig::SignalProperties& sigProp)
		{
			spdlog::info("Initializing Transformer Example!\n");
			return true;
		}
		
		int get_data(uint8_t* mem, sig::time_point& time, sig::SignalMemory sigMem)
		{
			spdlog::debug("Default Transformer signal production call - this shouldn't happen!");
			static int value = 0;
			*(int*)mem = value++;
			time = time_now();
			return sizeof(int);
		}


		void shutdown()
		{
			spdlog::info("Shutting down Transformer stub\n");
		}

		void proc(const uint8_t* mem, int size, const sig::SignalMetadata& sigMeta)
		{
			auto ts = time_point_as_string(sigMeta.acquisitionTime);
			spdlog::info("Got {} bytes at time {}\n", size, ts.c_str());
		}
	};

	sig::ExampleTransformer g_ExampleTransformer;


	

	// Initialize the signal producer (alloc resources/caches, etc)
	DYNALO_EXPORT bool DYNALO_CALL SignalTransformerInitializeProducer(const sig::SignalProperties& sigProp)
	{
		return g_ExampleTransformer.init(sigProp);
	}

	/*

	DYNALO_EXPORT bool DYNALO_CALL SignalTransformerInitializeConsumer(const sig::SignalProperties& sigProp, const sig::SignalConsumerRuntimeConfig& cfg)
	{
		return g_ExampleTransformer.init(sigProp);
	}
	*/


	// Get signal data and the time of generation. Return number of written bytes
	DYNALO_EXPORT int DYNALO_CALL GetSignalDataTransformer(uint8_t* mem, sig::time_point& time, sig::SignalMemory& sigMem)
	{		
		spdlog::debug("Default Transformer asked to produce data - this shouldn't happen!");
		return g_ExampleTransformer.get_data(mem, time, sigMem);
	}

	// Shutdown the signal producer (free resources/caches, etc)
	DYNALO_EXPORT void DYNALO_CALL SignalTransformerShutdownProducer()
	{
		g_ExampleTransformer.shutdown();
	}
	/*
	// Process the signal data (mem: pointer to data, size: number of bytes, time: time that signal was acquired)
	DYNALO_EXPORT void DYNALO_CALL ProcessSignalDataTransformer(const uint8_t* mem, int size, const sig::SignalMetadata& sigMeta)
	{
		g_ExampleTransformer.proc(mem, size, sigMeta);
	}
	*/
	
	// Shutdown the signal consumer (free resources/caches, etc)
	DYNALO_EXPORT void DYNALO_CALL SignalTransformerShutdownConsumer()
	{
		g_ExampleTransformer.shutdown();
	}

	bool sig::SignalLibraryTransformer::fnSignalConsumerInitialize(const SignalProperties& sigProp, const sig::SignalConsumerRuntimeConfig& sigRTcfg) {
		//spdlog::debug("Registering transformer to input signal {}", sigRTcfg.producer_type);
		//signalMemory.registerSignal(sigRTcfg.producer_type,sigProp.channelType, sigProp.signalIndex,sigProp.consumerFormMaxSize);
		return true;			
	}


	void sig::SignalLibraryTransformer::fnProcessSignalData(const uint8_t* mem, int size, const sig::SignalMetadata& metaData) {
		spdlog::debug("P{}: Transformer processing {} bytes for signal {}/{} in 'consumer' mode", metaData.packetId, size, metaData.sigType, metaData.sigIdx);
		
		switch (metaData.sigType) {
		case (int)net::SignalType::Client:
			signalMemory.signal_memories_user[metaData.sigIdx].add(mem, size, metaData);
			//signalMemory.signal_memories_user[metaData.sigIdx].dump_history("Client Add");
			break;
		case (int)net::SignalType::Environment:			
			signalMemory.signal_memories_env[metaData.sigIdx].add(mem, size, metaData);
			break;
		default:
			spdlog::warn("Transformer asked to Process unhandled signal type {}/{}", metaData.sigType, metaData.sigIdx);
			break;
		}		
	}

	void sig::SignalLibraryTransformer::RegisterSignal(std::string signalName, net::SignalType sigType, int signalIdx, int signalSize, int queueSize) {
		signalMemory.registerSignal(signalName, sigType, signalIdx, signalSize, queueSize);

	}

	const char* TRANSFORMER_NAME = "fallback/v1.0";
	
	DYNALO_EXPORT const char * DYNALO_CALL GetName() {
		return TRANSFORMER_NAME;
	}

	/*
	void sig::SignalLibraryTransformer::fnGetSignalData(uint8_t* mem, sig::time_point& time) {
		getSignalDataCore(mem, time, signalMemory);
	}
	*/
}
