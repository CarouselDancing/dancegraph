// Attempt at a generic filedump replay utility

#include <fstream>
#include <iostream>
#include <memory>

#include <string>
#include <cstring>

//#include <vector>
#include <nlohmann/json.hpp>

#include <spdlog/spdlog.h>

#include <spdlog/formatter.h>

#include <core/net/config_master.h>

#include <core/net/config_runtime.h>

//#include <net/utility.h>

#include <sig/signal_config.h>
#include "undumper.h"

#include "core/net/config_master.h"

// 640 Megabytes should be enough for anyone
#define MAX_JSON_HEADER_SIZE 655360000

using namespace std;
using json = nlohmann::json;

#define DATA_ERROR_CORRECTION 8

// Frame marker is 'DGSV' in ascii
#define DGSAV_FRAME_MARKER 1448298308

namespace dll {
	std::unique_ptr<Undumper> g_Undumper;

	bool Undumper::read_data_from_file_v01(ifstream& dataFile) {

		bool init_flag = true;

		while (dataFile) {

			//sig::SignalHeader* newHeader = new sig::SignalHeader;
			//dataFile.read((char *)newHeader, sizeof(sig::SignalHeader));
			uint8_t* newFrame = new uint8_t[frame_size + DATA_ERROR_CORRECTION];
			dataFile.read((char*)newFrame, frame_size + DATA_ERROR_CORRECTION);

			if (dataFile) {
				//headers.push_back(newHeader);
				frames.push_back(newFrame);
			}
			else {
				//delete newHeader;
				delete[] newFrame;
			}

			//sig::SignalHeader * sh = (sig::SignalHeader *) newFrame;

		}

		
		sig::SignalHeader* sh = (sig::SignalHeader*)frames[0];
		playback_start_time = sh->timestamp;


		return true;
	}

	bool Undumper::read_data_from_file_v02(ifstream& dataFile) {
		bool init_flag = true;

		uint32_t framemarker;

		uint32_t cur_frame_size;


		//new uint8_t[frame_size + DATA_ERROR_CORRECTION];


		while (dataFile) {
			
			UndumperFrame* framep = new UndumperFrame;
			//framep->data = new uint8_t[frame_size + DATA_ERROR_CORRECTION];
			framep->data = new uint8_t[frame_size];

			dataFile.read((char*)&framemarker, 4);
			if (framemarker != DGSAV_FRAME_MARKER) {
				spdlog::error("Undumper: Frame marker not found at position {} - {}", (uint32_t) dataFile.tellg(), framemarker);
			}

			dataFile.read((char*)&cur_frame_size, 4);			
			//dataFile.read((char*)&cur_frame_size, 4);
			
			dataFile.read((char*)(framep->data), cur_frame_size);
			framep->size = cur_frame_size;

			if (dataFile)
				frames_v2.push_back(framep);

			/*
			sig::SignalMetadata* ffp = (sig::SignalMetadata*)frames_v2[frames_v2.size() - 1]->data;
			long long int tsi = std::chrono::duration_cast<std::chrono::microseconds> (ffp->acquisitionTime.time_since_epoch()).count();
			spdlog::info("Initial frame metadata: {},{},{},{}, * {}", ffp->userIdx, ffp->packetId, ffp->sigIdx, ffp->sigType, tsi);
			*/



			//sig::SignalHeader* sh = (sig::SignalHeader*)framep->data;
		}

		sig::SignalMetadata* sm = (sig::SignalMetadata*)frames_v2[0]->data;
		playback_start_time_v2 = sm->acquisitionTime;

		sig::SignalMetadata* sm2 = (sig::SignalMetadata*)frames_v2[1]->data;

		long long int tsi1 = std::chrono::duration_cast<std::chrono::milliseconds> (sm->acquisitionTime.time_since_epoch()).count();
		long long int tsi2 = std::chrono::duration_cast<std::chrono::milliseconds> (sm2->acquisitionTime.time_since_epoch()).count();

		spdlog::info("Initial frame metadata: {},{},{},{}, * {}", sm->userIdx, sm->packetId, sm->sigIdx, sm->sigType, tsi1);
		spdlog::info("Second frame metadata: {},{},{},{}, * {}", sm2->userIdx, sm2->packetId, sm2->sigIdx, sm2->sigType, tsi2);

		spdlog::info("Read {} frames", frames_v2.size());
		return true;

	}


	bool Undumper::initialize(sig::SignalProperties& sigProp) {
		ifstream dataFile;


		dataFile = ifstream(filename, ios::in | ios::binary);

		if (!dataFile) {
			spdlog::info("Can't open input file {}", filename);
			return false;
		}

		char magicnum[5];

		dataFile.read(magicnum, 5);

		if (strncmp(magicnum, "DGSAV", 5)) {
			spdlog::info("Wrong file header - not a sav file");
		}

		int optSize;

		dataFile.read((char*)&optSize, 4);

		if (optSize < 0 || optSize > MAX_JSON_HEADER_SIZE) {
			spdlog::info("Bad option header size: {}", optSize);
		}

		//std::unique_ptr<char> optString = std::unique_ptr<char>(new char[optSize]);

		char* optString = new char[optSize + 1];

		dataFile.read(optString, optSize);
		std::cout << "Header bytes: " << std::hex << optSize << "\n";

		optString[optSize] = '\0';
		json newJS, spJS;
		

		std::string producer_type;
		try {
			newJS = json::parse(optString);
			//cfgSrv.merge_options(newJS);
			spdlog::info("File options: {}", newJS.dump(4));
			spJS = json::parse(sigProp.jsonConfig);
			spdlog::info("Overriding producer:{}", spJS.dump(4));

			for (json::iterator j = newJS.begin(); j != newJS.end(); j++) {
				spJS["producer_override"] = j.key();
				producer_type = j.key();
			}
			sigProp.jsonConfig = spJS.dump(4);
			spdlog::info("JSon Config: {}", sigProp.jsonConfig);
		}
		catch (json::exception e) {
			spdlog::error("Undumper configuration issue:{}\n{}\n{}", e.what(), newJS.dump(4), spJS.dump(4));
			spdlog::error("Problem candidate file: {}", filename);
			spdlog::error("Problem candidate 1: {}", optString);
			spdlog::error("Problem candidate 2: {}", sigProp.jsonConfig);
		}

		try {
			newJS.at("save-version").get_to(save_version);
		}
		catch (json::exception e) {
			spdlog::error("No undump save version, defaulting to 0.1");
			save_version = "0.1";
		}

		// Now we load the dll config for the saved producer type to get the Signal Properties

		auto& master_cfg = net::cfg::Root::instance();
		master_cfg.load();

		std::string newdll;
		spdlog::info("Producer type is {}", producer_type);
		try {
			
			std::vector<std::string> signal_parts = dancenet::string_split(producer_type, '/');

			// Default to 'v1.0' if there's no version number
			if (signal_parts.size() == 1) {
				signal_parts.push_back("v1.0");
			}
			spdlog::info("Signal Parts 0 = {}/{}", signal_parts[0], signal_parts[1]);

			spdlog::info("Master Config User signal size is {}", master_cfg.user_signals.size());
			for (auto& f : master_cfg.user_signals) {
				spdlog::info("Signal name: {}", f.first);
			}
			//const auto signal_module = master_cfg.user_signals.at(producer_type).at(signal_parts[1]);;
			const auto signal_module = master_cfg.user_signals.at(signal_parts[0]).at(signal_parts[1]);;
			

			// Load the config library
			newdll = master_cfg.make_library_filename(signal_module.config.dll);
			spdlog::info("Config filename {} is: {}", signal_module.config.dll, newdll);
		}
		catch (std::exception e) {
			spdlog::info("Problem splitting producer {}: {}", producer_type, e.what());
		}

		sig::SignalLibraryConfig confception;
		spdlog::info("Loading savefile config dll {}", newdll);
		confception.init(newdll.c_str());
		confception.fnGetSignalProperties(sigProp);

		frame_size = sigProp.consumerFormMaxSize;
		data_size = frame_size - sizeof(sig::SignalMetadata); // Was sig::SignalHeader

		std::cout << "Frame size is 0x" << std::hex << frame_size << ", data_size=0x" << data_size << ", offset=0x" << dataFile.tellg() << "\n";

		delete[] optString;

		/*		catch (std::exception e) {
					spdlog::info("Error: Failed to initialize file open: {}", e.what());
					return false;
				}
		*/
		spdlog::info("Reading version {} save\n", save_version);

		if (save_version == std::string("0.1")) {

			g_Undumper->read_data_from_file_v01(dataFile);
		}
		else if (save_version == std::string("0.2")) {
			g_Undumper->read_data_from_file_v02(dataFile);
		}
		else {
			spdlog::critical("Error, failed to find undumper version\n");
			dataFile.close();
			return false;
		}


		dataFile.close();



		start_time = std::chrono::system_clock::now();
		return true;
	}
	

	Undumper::Undumper(std::string fname) : filename(fname) {


	}

	Undumper::~Undumper() {
		for (auto i : frames) {
			delete[] i;
		}
	}

	uint8_t* Undumper::get_current_frame_v01() {
		auto now_time = chrono::system_clock::now();
		long long elapsed_time = chrono::duration_cast<chrono::milliseconds>(now_time - start_time).count();


		vector<uint8_t*>::iterator i;
		int cur_frame = 0;
		for (i = frames.begin(); i != frames.end(); i++) {

			sig::SignalHeader* hp = (sig::SignalHeader*)(*i);

			long long playback_time = (hp->timestamp - playback_start_time);
			if (playback_time >= elapsed_time) {

				if (cur_frame > last_played) {
					last_played = cur_frame;					
					return (*i);
				}
				else {
					return nullptr;
				}
			}
			cur_frame += 1;
		}

		spdlog::info("No frame found at {}", elapsed_time);

		return nullptr;

	}



	UndumperFrame* Undumper::get_current_frame_v02() {
		auto now_time = chrono::system_clock::now();
		long long elapsed_time = chrono::duration_cast<chrono::milliseconds>(now_time - start_time).count();


		std::chrono::time_point tn = chrono::system_clock::now();

		vector<UndumperFrame*>::iterator i;
		int cur_frame = 0;
		for (i = frames_v2.begin(); i != frames_v2.end(); i++) {
			//(i != frames.end()) && (((*i)->timestamp - playback_start_time) > elapsed_time));
				//i++) {

			sig::SignalMetadata* hp = (sig::SignalMetadata*)((*i)->data);

			long long playback_time = std::chrono::duration_cast<std::chrono::milliseconds> (hp->acquisitionTime - playback_start_time_v2).count();

			spdlog::trace("Undump: Playback: {}, Elapsed: {}", playback_time, elapsed_time);
			if (playback_time >= elapsed_time) {
				
				if (cur_frame > last_played) {
					last_played = cur_frame;
					return (*i);
				}
				else {
					
					return nullptr;					
				}
			}
			cur_frame += 1;
		}
		return nullptr;

	}
}
DYNALO_EXPORT bool DYNALO_CALL SignalProducerInitialize(sig::SignalProperties& sigProp) {
	spdlog::set_default_logger(sigProp.logger);
	std::string infile;
	try {
		spdlog::info("SigProp undumper opts: {} {}", sigProp.signalType, sigProp.jsonConfig);
		json pJs = json::parse(sigProp.jsonConfig);
		//pJs.at("producer").at("playback_file").get_to(infile);
		//spdlog::info(  "Opening playback file {}", infile);
		
		infile = pJs.at(sigProp.signalType).at("playback_file");

	}
	catch (json::exception e) {
		spdlog::info(  "Can't open playback file: {}", infile);		
		return false;
	}

	dll::g_Undumper = std::unique_ptr<dll::Undumper>(new dll::Undumper(infile));

	return dll::g_Undumper->initialize(sigProp);	

}

DYNALO_EXPORT int DYNALO_CALL GetSignalData(uint8_t* mem, sig::time_point & time) {

	if (dll::g_Undumper->save_version == "0.1") {
		uint8_t* frame = dll::g_Undumper->get_current_frame_v01();

		// No data today. Try again later.
		if (frame == nullptr)
			return -1;
		
		memcpy((void*)mem, frame + sizeof(sig::SignalHeader), dll::g_Undumper->data_size);

		return dll::g_Undumper->data_size;
	}
	else if (dll::g_Undumper->save_version == "0.2") {
		dll::UndumperFrame* frame = dll::g_Undumper->get_current_frame_v02();
		if (frame == nullptr)
			return -1;
		memcpy((void*)mem, frame->data + sizeof(sig::SignalMetadata), frame->size);
		spdlog::trace("Sending frame size {} upwards", frame->size - sizeof(sig::SignalMetadata));
		return frame->size - sizeof(sig::SignalMetadata);
	}
	return -1;
}
	



DYNALO_EXPORT void DYNALO_CALL SignalProducerShutdown() {	
}

