/** @file signal_config.h

File containing a template config library
*/



#ifndef __SIGNAL_CONFIG_H__
#define __SIGNAL_CONFIG_H__

#include <memory>

#define DYNALO_EXPORT_SYMBOLS
#include <dynalo/symbol_helper.hpp>
#include <dynalo/dynalo.hpp>

#include "signal_common.h"



namespace sig
{
	/**
	* \struct SignalLibraryConfig
	* Example implementation 
	* Wrapper around the a config library, and the API we're interested in
	*
	*/ 
	struct SignalLibraryConfig
	{
		SignalLibraryConfig() = default;
		SignalLibraryConfig(const char* sharedLibraryPath) { init(sharedLibraryPath); }
		
		bool init(const char* sharedLibraryPath);
		bool is_initialized() const { return library != nullptr; }

		std::function<void(SignalProperties&)> fnGetSignalProperties;
		std::unique_ptr<dynalo::library> library;
	};
}


/**
* \fn DYNALO_EXPORT void DYNALO_CALL GetSignalProperties(sig::SignalProperties& sigProp);
* 
* This function is called by the DanceGraph application in order to populate the sig::SignalProperties struct,
* and so that the jsonConfig class string can be used to pass data onwards to the producer and consumer DLLS
*/

DYNALO_EXPORT void DYNALO_CALL GetSignalProperties(sig::SignalProperties& sigProp);

#endif // __SIGNAL_CONFIG_H__