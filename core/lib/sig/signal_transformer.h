/** @file signal_transformer.h

File containing a template transformer
*/

#pragma once


#define DYNALO_EXPORT_SYMBOLS

#include <vector>

#include <dynalo/symbol_helper.hpp>
#include <dynalo/dynalo.hpp>

#include "signal_common.h"

#include "signal_history.h"

#include "signal_producer.h"
#include "signal_consumer.h"


#include <spdlog/spdlog.h>

namespace sig
{
	struct SignalLibraryTransformer : public SignalLibraryConsumer, SignalLibraryProducer
	{
		SignalLibraryTransformer() = default;
		SignalLibraryTransformer(const char* sharedLibraryPath) {
			init(sharedLibraryPath);
		}

		SignalMemory signalMemory;

		std::function<int(uint8_t*, time_point&, SignalMemory &)> getSignalDataCore;
		std::function<int(uint8_t* mem, sig::time_point& time)> fnGetSignalData;
		std::function<const char *()> fnGetName;

	public:

		const bool is_transformer = true;

		bool init(const char* sharedLibraryPath)
		{

			spdlog::info("Loading Transformer Library from {}", sharedLibraryPath);


			signalMemory = SignalMemory{};
			signalMemory.initialize();

			// We also need to worry about the library throwing an exception
			try {
				library = std::make_unique<dynalo::library>(sharedLibraryPath);
			}
			catch (std::exception & e) {
				spdlog::critical("Transformer Init failure: {}", e.what());
				library = nullptr;
						
			}
			if (library)
			{			
				fnSignalProducerInitialize = library->get_function<bool(const sig::SignalProperties&)>("SignalTransformerInitializeProducer");

				getSignalDataCore = library->get_function<int(uint8_t*, time_point&, SignalMemory& mem)>("GetSignalDataTransformer");

				fnSignalProducerShutdown = library->get_function<void()>("SignalTransformerShutdownProducer");
				fnSignalConsumerShutdown = library->get_function<void()>("SignalTransformerShutdownConsumer");

				spdlog::info("Library Successfully loaded and library functions got");
				fnGetSignalData = [ & gSDC = getSignalDataCore, &sM = signalMemory](uint8_t* mem, sig::time_point& time) { return gSDC(mem, time, sM); };

				fnGetName = library->get_function<const char *()>("GetName");
			}

			return library != nullptr;

		}
		bool is_initialized() const { return library != nullptr; }

		// Annoying housekeeping - we only find out the signal size long AFTER we initialize the transformer and signalmemory
		void RegisterSignal(std::string signalName, net::SignalType sigType, int signalIdx, int signalSize, int queueSize);

		std::unique_ptr<dynalo::library> library;

		bool fnSignalConsumerInitialize(const SignalProperties&, const sig::SignalConsumerRuntimeConfig&);

		// This looks like a dll function, but it's actually inbuilt, and adds entries to the history queue
		void fnProcessSignalData(const uint8_t* mem, int size, const sig::SignalMetadata& metaData);
		
	};
}




/**
* \fn DYNALO_EXPORT bool SignalTransformerInitializeProducer(const sig::SignalProperties& sigProp);
*
* This function is called at the initialization stage to allow the transformer - in producer mode - to initialize any variables, allocate resources, etc.
* Return true if successful
*/
DYNALO_EXPORT bool DYNALO_CALL SignalTransformerInitializeProducer(const sig::SignalProperties& sigProp);


/**
* \fn DYNALO_EXPORT bool SignalConsumerInitializeConsumer(const sig::SignalProperties& sigProp);
*
* This function is called at the initialization stage to allow the transformer - in consumer mode - to initialize any variables, allocate resources, etc.
* These are called once for each signal type.
* 
* Return true if successful
*/
DYNALO_EXPORT bool SignalTransformerInitializeConsumer(const sig::SignalProperties& sigProp, const sig::SignalConsumerRuntimeConfig& cfg); 

/**
* \fn DYNALO_EXPORT int DYNALO_CALL GetSignalDataTransformer(uint8_t * mem, sig::time_point& time, sig::SignalMemory& sigMem);
*
* GetSignalData is called to allow the producer to write signal data to the memory pointed at by `mem`.
* The return value is the number of bytes written to `mem`, otherwise the return value should be 0.
* A timestamp `time` is provided which may be useful for some timed applications (e.g. read-from-file producers or synthetic signal generators)
* For transformers, a SignalMemory struct is passed into the function to allow access to the relevant finite signal histories 
*
*/

DYNALO_EXPORT int DYNALO_CALL GetSignalDataTransformer(uint8_t* mem, sig::time_point& time, sig::SignalMemory& sigMem);


/**
* \fn void DYNALO_CALL SignalTransformerShutdownConsumer();
* This should be set to do nothing; the Producer shutdown is only called once and should be the default
*
*/

DYNALO_EXPORT void DYNALO_CALL SignalTransformerShutdownConsumer();

/**
* \fn void DYNALO_CALL SignalTransformerShutdownProducer();
* Called when the transformer is being shutdown, used to allow the module writer to deallocate resources, free memory, and otherwise tidy themselves up
*
*/


DYNALO_EXPORT void DYNALO_CALL SignalTransformerShutdownProducer();


/**
* \fn void DYNALO_CALL std::string GetName();
* Returns the transformer's name in string-name/version format
*/


//DYNALO_EXPORT std::string DYNALO_CALL GetName();
DYNALO_EXPORT const char * DYNALO_CALL GetName();

