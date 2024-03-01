#pragma once

#define DYNALO_EXPORT_SYMBOLS
#include <dynalo/symbol_helper.hpp>

#include <vector>

#include <sig/signal_common.h>
#include <chrono>

//#include "zed_common.h"

using namespace std;

namespace dll {

	struct UndumperFrame {
		uint32_t size;
		uint8_t * data;

		~UndumperFrame() {
			if (data)
				delete[] data;
		}
	};


	class Undumper {

	public:

		bool initialize(sig::SignalProperties & sigProp);

		Undumper(std::string fname);
		~Undumper();

		bool read_data_from_file_v01(ifstream& df);
		bool read_data_from_file_v02(ifstream& df);
		uint8_t* get_current_frame_v01();

		UndumperFrame * get_current_frame_v02();

		int frame_size;
		int data_size;
		std::string save_version;


	protected:
		

		sig::time_point start_time;
		long long playback_start_time;
		std::chrono::time_point<std::chrono::system_clock> playback_start_time_v2;
		
		int last_played = -1;

		ifstream dataFile;
		string filename;		
		
		//std::vector<sig::SignalHeader *> headers;
		std::vector<uint8_t *> frames;
		std::vector<UndumperFrame *> frames_v2;
	};

}
// Initialize the signal producer (alloc resources/caches, etc)
DYNALO_EXPORT bool DYNALO_CALL SignalProducerInitialize(sig::SignalProperties& sigProp);

// Get signal data and the time of generation. Return number of written bytes
DYNALO_EXPORT int DYNALO_CALL GetSignalData(uint8_t* mem, sig::time_point& time);

// Shutdown the signal producer (free resources/caches, etc)
DYNALO_EXPORT void DYNALO_CALL SignalProducerShutdown();