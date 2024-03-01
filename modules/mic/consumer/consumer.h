#pragma once

#define DYNALO_EXPORT_SYMBOLS

#include <dynalo/symbol_helper.hpp>

#include <sig/signal_common.h>
#include <iostream>
#include <fstream>

#include <ipc/ringbuffer.h>
//#include <modules/mic/mic_common.h>

// Initialize the signal consumer (alloc resources/caches, etc)
DYNALO_EXPORT bool DYNALO_CALL SignalConsumerInitialize(const sig::SignalProperties& sigProp, const sig::SignalConsumerRuntimeConfig& cfg);

// Process the signal data (mem: pointer to data, size: number of bytes, time: time that signal was acquired)
DYNALO_EXPORT void DYNALO_CALL ProcessSignalData(const uint8_t* mem, int size, const sig::SignalMetadata& sigMeta);

// Shutdown the signal consumer (free resources/caches, etc)
DYNALO_EXPORT void DYNALO_CALL SignalConsumerShutdown();