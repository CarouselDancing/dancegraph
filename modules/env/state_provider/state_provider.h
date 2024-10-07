#pragma once

#define DYNALO_EXPORT_SYMBOLS
#include <dynalo/symbol_helper.hpp>

#include <sig/signal_common.h>
#include <env_common.h>

using namespace std;

using json = nlohmann::json;


namespace dll {

  
    class UserStateProvider {
		EnvUserState userState;
		sig::time_point start_time;
		bool firstTime = true;
    public:
		
		
		float timeDelay = 0.0;
        bool sent = false;
        void populate_from_json(json opts);
        int send_user_data(uint8_t * mem, sig::time_point & time);

    };

		// Keeps a preferred initial user Environment State and spits it out on demand



	// Initialize the signal producer (alloc resources/caches, etc)
	DYNALO_EXPORT bool DYNALO_CALL SignalProducerInitialize(const sig::SignalProperties& sigProp);

	// Get signal data and the time of generation. Return number of written bytes
	DYNALO_EXPORT int DYNALO_CALL GetSignalData(uint8_t* mem, sig::time_point& time);

	// Shutdown the signal producer (free resources/caches, etc)
	DYNALO_EXPORT void DYNALO_CALL SignalProducerShutdown();

}