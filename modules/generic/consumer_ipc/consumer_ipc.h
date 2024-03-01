#pragma once

#define DYNALO_EXPORT_SYMBOLS
#include <dynalo/symbol_helper.hpp>

#include <sig/signal_common.h>

#include <ipc/ringbuffer.h>

#include <core/net/config.h>

namespace ipc {


	class IPCConsumer {
		
	public:
		DNRingbufferWriter ipcWriter;

		bool metadata_passthrough = true;

		bool init(const sig::SignalProperties& sigProp, sig::SignalConsumerRuntimeConfig & cfg);
		void proc(const uint8_t* mem, int size, sig::SignalMetadata sigMeta);
		void shutdown();

		IPCConsumer(const sig::SignalProperties& sigProp, const std::string name, int bufEntries);

		char * assembly_buffer;

	};


	// Initialize the signal consumer (alloc resources/caches, etc)
	DYNALO_EXPORT bool DYNALO_CALL SignalConsumerInitialize(const sig::SignalProperties& sigProp, sig::SignalConsumerRuntimeConfig& cfg);

	// Process the signal data (mem: pointer to data, size: number of bytes, time: time that signal was acquired)
	DYNALO_EXPORT void DYNALO_CALL ProcessSignalData(const uint8_t* mem, int size, const sig::SignalMetadata & meta);

	// Shutdown the signal consumer (free resources/caches, etc)
	DYNALO_EXPORT void DYNALO_CALL SignalConsumerShutdown();


	//DYNALO_EXPORT bool DYNALO_CALL SignalConsumerInitialize(const sig::SignalProperties& sigProp, const sig::SignalConsumerRuntimeConfig& cfg);


}