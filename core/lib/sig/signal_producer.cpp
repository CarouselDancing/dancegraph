#include "signal_producer.h"

#include <spdlog/spdlog.h>

namespace sig
{
	struct ExampleProducer
	{
		bool init(const sig::SignalProperties& sigProp)
		{
			spdlog::info("Initializing producer\n");
			return true;
		}

		void shutdown()
		{
			spdlog::info("Shutting down producer\n");
		}

		int get_data(uint8_t* mem, sig::time_point& time)
		{
			static int value = 0;
			*(int*)mem = value++;
			time = time_now();
			return sizeof(int);
		}

	};

}

sig::ExampleProducer g_ExampleProducer;

// Initialize the signal producer (alloc resources/caches, etc)
DYNALO_EXPORT bool DYNALO_CALL SignalProducerInitialize(const sig::SignalProperties& sigProp)
{
	spdlog::set_default_logger(sigProp.logger);
	return g_ExampleProducer.init(sigProp);
}

// Get signal data and the time of generation. Return number of written bytes
DYNALO_EXPORT int DYNALO_CALL GetSignalData(uint8_t* mem, sig::time_point& time)
{
	return g_ExampleProducer.get_data(mem, time);
}

// Shutdown the signal producer (free resources/caches, etc)
DYNALO_EXPORT void DYNALO_CALL SignalProducerShutdown()
{
	g_ExampleProducer.shutdown();
}