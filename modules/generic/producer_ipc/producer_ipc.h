#pragma once

#define DYNALO_EXPORT_SYMBOLS
#include <dynalo/symbol_helper.hpp>

#include <sig/signal_common.h>

#include <ipc/ringbuffer.h>

#include <core/net/config.h>

namespace ipc {

	// We should also have a reliable IPC conduit, either built-in or otherwise


	class IPCProducer {
		
	public:
		DNRingbufferReader ipcReader;

		// Do we need this?
		// bool metadata_passthrough = true;

		bool init(const sig::SignalProperties& sigProp);
		int proc(const uint8_t* mem, sig::time_point& time);
		void shutdown();

		IPCProducer(const sig::SignalProperties& sigProp, const std::string name, int bufEntries);

	};

	// Initialize the signal consumer (alloc resources/caches, etc)
	DYNALO_EXPORT bool DYNALO_CALL SignalProducerInitialize(const sig::SignalProperties& sigProp);
	
	// Get signal data and the time of generation. Return number of written bytes
	DYNALO_EXPORT int DYNALO_CALL GetSignalData(uint8_t* mem, sig::time_point& time);

	// Shutdown the signal consumer (free resources/caches, etc)
	DYNALO_EXPORT void DYNALO_CALL SignalProducerShutdown();


}