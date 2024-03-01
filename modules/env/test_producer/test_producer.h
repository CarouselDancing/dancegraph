#pragma once

#define DYNALO_EXPORT_SYMBOLS
#include <dynalo/symbol_helper.hpp>

#include <vector>

#include <core/lib/sig/signal_common.h>
#include <chrono>

using namespace std;


namespace dll {
	
}
// Initialize the signal producer (alloc resources/caches, etc)
DYNALO_EXPORT bool DYNALO_CALL SignalProducerInitialize(const sig::SignalProperties& sigProp);

// Get signal data and the time of generation. Return number of written bytes
DYNALO_EXPORT int DYNALO_CALL GetSignalData(uint8_t* mem, sig::time_point& time);

// Shutdown the signal producer (free resources/caches, etc)
DYNALO_EXPORT void DYNALO_CALL SignalProducerShutdown();