#include "impulse_signal_consumer.h"

#include <cstdio>
#include <iostream>
#include <sstream>

#include <spdlog/spdlog.h>

namespace sig
{
	struct ExampleConsumer
	{
		bool init(const sig::SignalProperties& sigProp)
		{
			spdlog::info("Initializing consumer\n");
			return true;
		}

		void shutdown()
		{
			spdlog::info("Shutting down consumer\n");
		}

		void proc(const uint8_t* mem, int size, const sig::SignalMetadata& sigMeta)
		{
			auto ts = time_point_as_string(sigMeta.acquisitionTime);
			const auto counter = *(int32_t*)mem;
			spdlog::info("Got counter value {} at time {}\n", counter, ts.c_str());
		}
	};
}

sig::ExampleConsumer g_ExampleConsumer;

// Initialize the signal consumer (alloc resources/caches, etc)
DYNALO_EXPORT bool DYNALO_CALL SignalConsumerInitialize(const sig::SignalProperties& sigProp, const sig::SignalConsumerRuntimeConfig& cfg)
{
	spdlog::set_default_logger(sigProp.logger);
	return g_ExampleConsumer.init(sigProp);
}

// Process the signal data (mem: pointer to data, size: number of bytes, sigMeta : metadata struct relating to the incoming packet)
DYNALO_EXPORT void DYNALO_CALL ProcessSignalData(const uint8_t* mem, int size, const sig::SignalMetadata& sigMeta)
{
	g_ExampleConsumer.proc(mem, size, sigMeta);
}

// Shutdown the signal consumer (free resources/caches, etc)
DYNALO_EXPORT void DYNALO_CALL SignalConsumerShutdown()
{
	g_ExampleConsumer.shutdown();
}