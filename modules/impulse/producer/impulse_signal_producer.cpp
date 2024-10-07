#include "impulse_signal_producer.h"
#include "../config/impulse_signal_config.h"

#include <spdlog/spdlog.h>


namespace sig
{
	struct ExampleProducer
	{
		ImpulseSignalConfig cfg;
		sig::time_point last_time_point;
		int numBytesWrite;

		bool init(const sig::SignalProperties& sigProp)
		{
			spdlog::info("Initializing producer\n");
			auto j = nlohmann::json::parse(sigProp.jsonConfig);
			cfg = j[sigProp.signalType];
			spdlog::info("Impulse produces {} bytes @ {}ms intervals {}ms sleep\n", sigProp.producerFormMaxSize, cfg.interval_ms, cfg.sleep_ms);
			last_time_point = time_now();
			numBytesWrite = sigProp.producerFormMaxSize;
			return true;
		}

		void shutdown()
		{
			spdlog::info("Shutting down producer\n");
		}
		
		int get_data(uint8_t* mem, sig::time_point& time)
		{			
			sig::time_point now = time_now();
			time = now;
			auto elapsed = now - last_time_point;
			auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

			if (elapsed_ms < cfg.interval_ms) {				
				return 0;
			}

			// This sleeps AFTER "work" is done (time acquisition-wise)
			std::this_thread::sleep_for(std::chrono::milliseconds(cfg.sleep_ms));
			last_time_point = now;
			return numBytesWrite;
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