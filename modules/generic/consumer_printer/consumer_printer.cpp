#include "consumer_printer.h"
#include <iostream>

#include <spdlog/spdlog.h>

namespace sig
{
	struct ConsumerPrinter
	{
		
		// This is now of variable size
		sig::SignalProperties props{};

		//std::unique_ptr<sig::SignalProperties> propsp;

		bool init(const sig::SignalProperties & sigProp)
		{
			spdlog::info("Initializing consumer\n");
			//props = sigProp;

			// For some reason, I need an explicit copy constructor
			
			props.jsonConfig = sigProp.jsonConfig;
			props.isReliable = sigProp.isReliable;
			props.keepState = sigProp.keepState;
			props.isPassthru = sigProp.isPassthru;

			props.producerFormMaxSize = sigProp.producerFormMaxSize;
			props.stateFormMaxSize = sigProp.stateFormMaxSize;
			props.consumerFormMaxSize = sigProp.consumerFormMaxSize;
			props.networkFormMaxSize = sigProp.networkFormMaxSize;
			props.toString = sigProp.toString;
			
			return true;
		}

		void shutdown()
		{
			spdlog::info("Shutting down consumer\n");
		}

		void proc(const uint8_t* mem, int size, const sig::SignalMetadata& sigMeta)
		{
			auto ts = time_point_as_string(sigMeta.acquisitionTime);
			std::string text;
						
			props.toString(text, mem, size, SignalStage::Consumer);
			spdlog::info("[{}] uID: {} , {}\n", ts, sigMeta.userIdx, text);
		}
	};
}

sig::ConsumerPrinter g_ConsumerPrinter;

// Initialize the signal consumer (alloc resources/caches, etc)
DYNALO_EXPORT bool DYNALO_CALL SignalConsumerInitialize(const sig::SignalProperties& sigProp)
{
	spdlog::set_default_logger(sigProp.logger);
	return g_ConsumerPrinter.init(sigProp);
}

// Process the signal data (mem: pointer to data, size: number of bytes, time: time that signal was acquired)
DYNALO_EXPORT void DYNALO_CALL ProcessSignalData(const uint8_t* mem, int size, const sig::SignalMetadata& sigMeta)
{
	g_ConsumerPrinter.proc(mem, size, sigMeta);
}

// Shutdown the signal consumer (free resources/caches, etc)
DYNALO_EXPORT void DYNALO_CALL SignalConsumerShutdown()
{
	g_ConsumerPrinter.shutdown();
}