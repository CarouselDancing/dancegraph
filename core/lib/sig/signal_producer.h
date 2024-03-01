/** @file signal_producer.h

File containing a template producer
*/

#ifndef __SIGNAL_PRODUCER_H__
#define __SIGNAL_PRODUCER_H__

//#include <memory>

#define DYNALO_EXPORT_SYMBOLS
#include <dynalo/symbol_helper.hpp>
#include <dynalo/dynalo.hpp>

#include "signal_common.h"

#include <spdlog/spdlog.h>

namespace sig
{
	/**
	* \struct SignalLibraryProducer
	* 
	* Example implementation
	* Wrapper around a Producer signal dynamic library, and the API we're interested in
	*/ 
	struct SignalLibraryProducer
	{
		SignalLibraryProducer() = default;
		SignalLibraryProducer(const char* sharedLibraryPath) { init(sharedLibraryPath); }
		virtual bool init(const char* sharedLibraryPath)
		{
			spdlog::info("Loading Producer Library from {}", sharedLibraryPath);
			auto actual_path = get_unique_shared_library_path_for_loading(sharedLibraryPath);
			library = std::make_unique<dynalo::library>(actual_path);
			if (library)
			{
				fnSignalProducerInitialize = library->get_function<bool(const sig::SignalProperties&)>("SignalProducerInitialize");
				
				fnGetSignalData = library->get_function<int(uint8_t*, time_point&)>("GetSignalData");
				fnSignalProducerShutdown = library->get_function<void()>("SignalProducerShutdown");
				//fnGetSignalProperties = library->get_function<void(sig::SignalProperties &)>("GetSignalProperties");
			}
			return library != nullptr;
		}
		bool is_initialized() const { return library != nullptr; }
	
		std::function<bool(const sig::SignalProperties&)> fnSignalProducerInitialize;		
		std::function<int(uint8_t*, time_point&)> fnGetSignalData;
		std::function<void()> fnSignalProducerShutdown;
		std::unique_ptr<dynalo::library> library;
		//std::function<void(SignalProperties &)> fnGetSignalProperties;
		const bool is_transformer = false;
	};
}

/** 
* \fn DYNALO_EXPORT bool SignalProducerInitialize(const sig::SignalProperties& sigProp);
* 
* This function is called at the initialization stage to allow the producer to initialize any variables, allocate resources, etc.
* Return true if successful
*/
DYNALO_EXPORT bool DYNALO_CALL SignalProducerInitialize(const sig::SignalProperties& sigProp);

/** 
* \fn DYNALO_EXPORT int DYNALO_CALL GetSignalData(uint8_t * mem, sig::time_point& time);
* 
* GetSignalData is called to allow the producer to write signal data to the memory pointed at by `mem`.
* The return value is the number of bytes written to `mem`, otherwise the return value should be 0.
* A timestamp `time` is provided which should be written to at the time of signal production.
* 
*/

DYNALO_EXPORT int DYNALO_CALL GetSignalData(uint8_t* mem, sig::time_point& time);

/**
* \fn void DYNALO_CALL SignalProducerShutdown();
* Called when the producer is being shutdown, used to allow the module writer to deallocate resources, free memory, and otherwise tidy themselves up
* 
*/

DYNALO_EXPORT void DYNALO_CALL SignalProducerShutdown();

#endif // __SIGNAL_PRODUCER_H__