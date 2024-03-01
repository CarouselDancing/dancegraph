// Zedcam producer for spitting out a completely zeroed set of body orientations, in order to investigate what the default position of the zedcam is

#include <fstream>
#include <sstream>
#include <iostream>
#include <memory>
#include <cstring>

#include <math.h>

#include <chrono>

#include "test_producer.h"

#include <spdlog/spdlog.h>

#include <modules/env/env_common.h>



using namespace std;


namespace dll {


	chrono::time_point<chrono::system_clock> start_time;
	std::chrono::system_clock::time_point previous_tick;
	long long tick_time;

	EnvTestState testStateTemplate = EnvTestState();
	EnvTestState * testState;
	
	// Initialize the signal producer (alloc resources/caches, etc)
	DYNALO_EXPORT bool DYNALO_CALL SignalProducerInitialize(const sig::SignalProperties& sigProp)
	{		

		/*
		config::ConfigServer cfgServer = config::ConfigServer();
		try {			
			cfgServer.get_opt_to(config::ENV, "testFreqMicroSecs", tick_time);
		}
		catch(json::exception e) {
			tick_time = 100000; // 10 fps
		}
		
		testStateTemplate.mBody.payload = 0;
		*/

		try {
			auto sigopts = nlohmann::json::parse(sigProp.jsonConfig).at("env/1.0");

			tick_time = sigopts.at("testFreqMicroSecs");
		}
		catch (nlohmann::json::exception e) {
			tick_time = 100000; // 10 fps
		}

		testStateTemplate.mBody.payload = 0;

		return true;
	}

	// Get signal data and the time of generation. Return number of written bytes
	DYNALO_EXPORT int DYNALO_CALL GetSignalData(uint8_t* mem, sig::time_point& time)
	{
		static int framecount = 0;
		static int prevfps = 0;

		auto now_time = std::chrono::system_clock::now();
		long long current_elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now_time - previous_tick).count();
		
		if (current_elapsed >= tick_time) {
			previous_tick = now_time;
			// Spam IPC with the test packet

			EnvTestState * testState = (EnvTestState*)mem;

			memcpy(testState, &testStateTemplate, sizeof(EnvTestState));

			std::stringstream ss;
			ss << std::hex;

			for (int i = 0; i < sizeof(EnvTestState); i++) {
				ss << (int)mem[i] << " ";
			}
			
			spdlog::info("{}: Producing {} bytes of test with payload {}", current_elapsed, sizeof(EnvTestState), testState->mBody.payload);
			spdlog::info("{}: Vals: {}", current_elapsed, ss.str());
			testStateTemplate.mBody.payload = testStateTemplate.mBody.payload + 7;
			return sizeof(EnvTestState);
		}
		else {
			//spdlog::info("{}: No tick at t={}", current_elapsed, tick_time);
			return -1;
		}
	}

	// Shutdown the signal producer (free resources/caches, etc)
	DYNALO_EXPORT void DYNALO_CALL SignalProducerShutdown()
	{
	}
}