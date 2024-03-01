/** @file signal_consumer.h

File containing a template consumer
*/


#ifndef __SIGNAL_CONSUMER_H__
#define __SIGNAL_CONSUMER_H__

#include <memory>

#define DYNALO_EXPORT_SYMBOLS
#include <dynalo/symbol_helper.hpp>
#include <dynalo/dynalo.hpp>

#include "signal_common.h"

#include <spdlog/spdlog.h>
#include <core/common/utility.h>

namespace sig
{

	/**
	* \struct SignalLibraryConsumer
	* Example implementation
	* Wrapper around a Consumer signal dynamic library, and the API we're interested in
	*
	*/
	struct SignalLibraryConsumer
	{
		using fn_process_signal_data_t = std::function<void(const uint8_t*, int, const sig::SignalMetadata&)>;
		SignalLibraryConsumer() = default;
		SignalLibraryConsumer(const char* sharedLibraryPath) { init(sharedLibraryPath); }
		virtual bool init(const char* sharedLibraryPath)
		{
			path = sharedLibraryPath;

			spdlog::info("Loading Consumer Library from {}", sharedLibraryPath);
			auto actual_path = get_unique_shared_library_path_for_loading(sharedLibraryPath);
			library = std::make_unique<dynalo::library>(actual_path);

			if (library)
			{
				fnSignalConsumerInitialize = library->get_function<bool(const SignalProperties&, const sig::SignalConsumerRuntimeConfig&)>("SignalConsumerInitialize");
				fnProcessSignalData = library->get_function<void(const uint8_t*, int, const SignalMetadata&)>("ProcessSignalData");
				fnSignalConsumerShutdown = library->get_function<void()>("SignalConsumerShutdown");
			}
			return library != nullptr;
		}

		// Do we feed transformed signals into other transformers?
		const bool is_transformer = false;

		bool is_initialized() const { return library != nullptr; }
	
		std::function<bool(const SignalProperties&, const sig::SignalConsumerRuntimeConfig&)> fnSignalConsumerInitialize;
		fn_process_signal_data_t fnProcessSignalData;
		std::function<void()> fnSignalConsumerShutdown;
		std::unique_ptr<dynalo::library> library;
		std::string path;
	};
}

/** 
* \fn DYNALO_EXPORT bool DYNALO_CALL SignalConsumerInitialize(const sig::SignalProperties& sigProp, const sig::SignalConsumerRuntimeConfig& cfg);
* This function is called on initialization with a previously populated `sig::SignalProperties` instance 
* and allows the module to initialize any variables, allocate resources, caches, etc.
* A `sig::SignalConsumerRuntimeConfig` instance is provided that contains additional signal information 
*/
DYNALO_EXPORT bool DYNALO_CALL SignalConsumerInitialize(const sig::SignalProperties& sigProp, const sig::SignalConsumerRuntimeConfig& cfg);

/**
* \fn DYNALO_EXPORT void DYNALO_CALL ProcessSignalData(const uint8_t * mem, int size, const sig::SignalMetadata& sigMeta);
* Called whenever there is an applicable signal ready to be processed by the consumer.
* The signal consists of `size` bytes pointed at by `mem`.
* Extra information on the origin and type of the signal is provided by the `sigMeta` SignalMetadata instance.
*/

DYNALO_EXPORT void DYNALO_CALL ProcessSignalData(const uint8_t* mem, int size, const sig::SignalMetadata& sigMeta);

/**
* \fn DYNALO_EXPORT void DYNALO_CALL SignalConsumerShutdown();
* 
*Called upon shutdown the signal consumer in order to free resources / caches, etc
*/

DYNALO_EXPORT void DYNALO_CALL SignalConsumerShutdown();

#endif