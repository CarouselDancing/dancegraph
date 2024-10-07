// A null producer, that produces no data at all, ever

#include "producer_null.h"
#include <spdlog/spdlog.h>
#include <sig/signal_common.h>

using namespace std;

namespace dll {


	// Initialize the signal producer (alloc resources/caches, etc)
	DYNALO_EXPORT bool DYNALO_CALL SignalProducerInitialize(const sig::SignalProperties& sigProp)
	{
		spdlog::set_default_logger(sigProp.logger);
		return true;
	}

	// Get signal data and the time of generation. Return number of written bytes
	DYNALO_EXPORT int DYNALO_CALL GetSignalData(uint8_t* mem, sig::time_point& time)
	{
		return 0;

	}

	// Shutdown the signal producer (free resources/caches, etc)
	DYNALO_EXPORT void DYNALO_CALL SignalProducerShutdown()
	{
	}
}