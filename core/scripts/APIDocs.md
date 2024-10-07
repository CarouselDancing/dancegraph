

<h2> The Producer/Consumer model </h2>

<h3> 1. Overview </h3>

In DanceGraph, signal input and output is decoupled from each other and separated from the main dancegraph client for the purposes of modularity and code reuse. These modules are implemented as shared library DLLs.

There are currently three types of DLL for use with the system - producers, consumers and config DLLs. A fourth type - transformers, which both produce and consume signals - is also part of the intended design, but are as yet unimplemented.

Producers are the signal generation modules. Typically a producer module will take information from a hardware source, such as a camera, or microphone or haptic device. Other signal sources - such as generating synthetic testing signals, or loading recorded signal data from a file - would also be implemented using producers.

Consumers are the modules which dispatch signal data. For example, the data can be passed off to a game engine via IPC, or saved to a file. Sending data upstream to the network is generally implicitly performed by the Dancegraph client and isn't implemented as a consumer. Consumers should preferably be written in a signal-agnostic fashion, though this may not always be possible. A single signal may have multiple consumers attached so that the same tracking signal could be, say, sent to a game engine AND saved to a file AND have a dump printed to stdout simultaneously.

Config modules are modules which set various signal options using the sig::SignalProperties class, and can pass some generic information to the producer/consumers.

Transformers are a forthcoming set of modules intended to morph incoming signals; these will be implemented in a future update.


<h3> 2. Examples </h3>

A barebones producer/consumer/signal template implementation is to be found in the ./sig directory

Other simple examples of the use of the API would include the IPC class in signal_libraries/generic/consumer_ipc and the zed signal test producer in signal_libraries/zed/zed_null

<h3> 3. The SignalProperties class </h3>

The sig::SignalProperties class is a class passed into the Config module to be populated, and then subsequently passed into the producers and consumers.

Defined in `net\config.h`
<h4>sig::SignalProperties properties</h4>


<ul>
<li>`bool isReliable = false;`</li>

A flag to indicate whether the signal should be sent over the network using reliable packets (i.e. with TCP-style acknowledgements). This significantly affects latency.

<li>`bool keepState = true;`</li>

A flag to indicate whether signal state is kept while reading/broadcasting or consuming

<li>`bool isPassthru = true;`</li>

A flag to indicate this is a passthru signal and should not be transformed anywhere in the pipeline

<li>`std::string jsonConfig = "{}";`</li>
This is a string of arbitrary JSON for passing options to be fed to the producers, consumers and transformers - this may be to indicate, say, buffer names for IPC transfer or similar.
</ul>

<h4> sig::SignalProperties methods</h4>


<ul>
<li>`void set_all_sizes(int datasize)`</li>
 helper function that should be the preferred method of populating the above to the correct size.
</ul>

<h3> 4. SignalConsumerRuntimeConfig Class </h3>


A class for passing some parameters to consumers at runtime
Defined in `sig/signal_common.h`
<h4> SignalConsumerRuntimeConfig properties </h4>
<ul>
<li>`std::string producer_type;`</li>

Name of the producer

<li>`int client_index`</li>

Index number of the client; set to -1 for a local user

<li>`bool metadata_override = true;`</li>

Set if you want to signal to the consumer to not alter the signal metadata (e.g. for file replay)
</ul>
<h4> SignalConsumerRuntimeConfig methods </h4>

<ul>
<li>`nlohmann::json toJson();`</li>

Serializes the methods into a JSON bundle
</ul>

<h3> 5. The SignalMetadata struct </h3>

Defined in `sig/signal_common.h`

<h4> SignalMetaData properties </h4>

<ul>
<li>`time_point acquisitionTime;`</li>
	Timestamp of the signal as acquired by the dancegraph plugin

<li>`uint32_t packetId = 0;`</li>
	Index of the received packet

<li>`int16_t userIdx = -1;`</li>
	Client index of the node sending the packet

<li>`uint8_t sigIdx = -1;`</li>
	Index of the signal as per the scene configuration

<li>`uint8_t sigType = 0;`</li>
	Signal Type control, client or environment

</ul>

<h3> 6. Config DLL </h3>

	#define DYNALO_EXPORT_SYMBOLS
    #include <dynalo/symbol_helper.hpp>
    #include <dynalo/dynalo.hpp>
    
    #include "signal_common.h"

<h4> Required headers</h4>

<h4> DLL Functions </h4>

<ul>
<li>`DYNALO_EXPORT GetSignalProperties(sig::SignalProperties & sigProp);`</li>

This function is called by the DanceGraph application in order to populate the signal's properties, and so that the jsonConfig class string can be used to pass data onwards to the producer and consumer DLLS
</ul>

<h3> 7. Producer API </h3>

<h4> Required headers </h4>

	#define DYNALO_EXPORT_SYMBOLS
    #include <dynalo/symbol_helper.hpp>
    #include <dynalo/dynalo.hpp>
    
    #include "signal_common.h"

<h4> DLL Functions </h4>

<ul>
<li>`DYNALO_EXPORT bool DYNALO_CALL SignalProducerInitialize(const sig::SignalProperties& sigProp);`</li>

This function is called at the initialization stage to allow the producer to initialize any variables, allocate resources, caches, etc. 
Return true if successful.

<li>`DYNALO_EXPORT int DYNALO_CALL GetSignalData(uint8_t* mem, sig::time_point& time);` </li>

GetSignalData is called to allow the producer to write signal data to the memory pointed at by `mem`. The return value is the number of bytes written to `mem`, otherwise the return value should be 0. A timestamp `time` is provided which may be useful for some timed applications (e.g. read-from-file producers or synthetic signal generators)


<li>`DYNALO_EXPORT void DYNALO_CALL SignalProducerShutdown();`</li>

Called when the producer is being shutdown, used to allow the module writer to deallocate resources, free memory, and otherwise tidy themselves up
</ul>

<h3> 8. Consumer API </h3>
<h4> Required headers </h4>

	#define DYNALO_EXPORT_SYMBOLS
    #include <dynalo/symbol_helper.hpp>
    #include <dynalo/dynalo.hpp>
    
    #include "signal_common.h"

<h4> DLL Functions </h4>

<ul>
<li>`DYNALO_EXPORT bool DYNALO_CALL SignalConsumerInitialize(const sig::SignalProperties& sigProp, const sig::SignalConsumerRuntimeConfig& cfg);`</li>
	This function is called on initialization with a previously populated `sig::SignalProperties` instance and allows the module to initialize any variables, allocate resources, caches, etc.
	A `sig::SignalConsumerRuntimeConfig` instance is provided that contains additional signal information (see class properties, above)

<li>`DYNALO_EXPORT void DYNALO_CALL ProcessSignalData(const uint8_t* mem, int size, const sig::SignalMetadata& sigMeta);`</li>
	Called whenever there is an applicable signal ready to be processed by the consumer.
	The signal consists of `size` bytes pointed at by `mem`. Extra information on the origin and type of the signal is provided by the `sigMeta` SignalMetadata instance.
	

<li>`DYNALO_EXPORT void DYNALO_CALL SignalConsumerShutdown();`</li>
Called upon shutdown the signal consumer in order to free resources/caches, etc

</ul>


