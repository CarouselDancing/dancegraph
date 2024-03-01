#include "consumer_dump2file.h"

#include <fstream>
#include <string>

#include <sstream>

#include <iomanip>
#include <ctime>

#include <spdlog/spdlog.h>

#define DG_SAVE_VERSION_STRING "0.2"


/* Current file format
*
*  Initial Header
*--------------------------------------------------
*  Bytes   |   Type    |   What
* -------------------------------------------------
*    5     |  char *   |   Constant string "DGSAV"
*    4     |  int      |   Size of json header (jSize)
*   jSize  |  char *   |   Json object in string form with config options; top level "save-version" should contain current version string
*
* 
* Per frame signal data
* -------------------------------------------------
*    4     |  char *   |   Constant string "DGSV"
*    4     |  int      |   Size of signal data including metadata (dataSize)
* 
* The signal and metadata all just come in and are dumped into the file as-is
* 
* Per frame signal metadata 
* -------------------------------------------------
*    4     |  int      |   UserID
*    4     |  int      |   Alignment padding
*    8     |  double   |   Timestamp (milliseconds since epoch)
*    4     |  int      |   packetID
* 
* Actual signal Data follows, the size being dataSize - sizeof(signal metadata)
* 
*/


namespace sig
{
	constexpr char framemarker[] = "DGSV";

	struct ConsumerDump2File
	{
		std::ofstream fs;
		sig::SignalProperties props;		

		bool init(const sig::SignalProperties & sigProp, std::string header1)
		{
			spdlog::info("Initializing dump2file consumer\n");
			
			auto t = time(nullptr);
			auto lt = *localtime(&t);
			std::stringstream ss;
			ss.str(std::string());

			ss << "ConsumerRecord_" << std::put_time(&lt, "%Y%m%d_%H%M%S") << ".dgs";

			fs = std::ofstream(ss.str().c_str(), std::ios::out | std::ios::binary);

			const char* magic = "DGSAV";

			fs.write(magic, 5);
			int h1l = header1.length();
			fs.write((const char*)&h1l, sizeof(int));
			fs.write(header1.c_str(), h1l);

			return fs.good();

		}

		void shutdown()
		{
			spdlog::info("Shutting down dump2file consumer\n");
			// close file
			if (fs.good())
				fs.close();
		}

		void proc(const uint8_t* mem, int size, const sig::SignalMetadata& sigMeta)
		{
			// Write the frame marker
			fs.write(framemarker, 4); // We're not including the null terminator
			int fullsize = size + sizeof(sig::SignalMetadata);
			fs.write((const char *) & fullsize, 4); // Write the signal size
			// memcopy the signal, including metadata
			fs.write((const char*) &sigMeta, sizeof(sig::SignalMetadata));
			fs.write((const char *)mem, size);
		}
	};
}

sig::ConsumerDump2File g_ConsumerDump2File;

// Initialize the signal consumer (alloc resources/caches, etc)
DYNALO_EXPORT bool DYNALO_CALL SignalConsumerInitialize(const sig::SignalProperties& sigProp, sig::SignalConsumerRuntimeConfig& rtcfg)
{
	std::string signal_type = sigProp.signalType;

	spdlog::info("Properties for dump2file consumer and signal: {} {}", signal_type, sigProp.jsonConfig);

	
	nlohmann::json optJs = nlohmann::json({});
	//optJs.at(signal_type).merge_patch(nlohmann::json::parse(sigProp.jsonConfig));
	optJs.merge_patch(nlohmann::json::parse(sigProp.jsonConfig));
	optJs["producer_name"] = rtcfg.producer_name;
	optJs["signal_type"] = rtcfg.producer_type;
	optJs["save-version"] = DG_SAVE_VERSION_STRING;
	spdlog::info("Initializing dump with options: {}", optJs.dump(4));

	// Get all the relevant options

	return g_ConsumerDump2File.init(sigProp, optJs.dump(4));
}

// Process the signal data (mem: pointer to data, size: number of bytes, time: time that signal was acquired)
DYNALO_EXPORT void DYNALO_CALL ProcessSignalData(const uint8_t* mem, int size, const sig::SignalMetadata& sigMeta)
{
	g_ConsumerDump2File.proc(mem, size, sigMeta);
}

// Shutdown the signal consumer (free resources/caches, etc)
DYNALO_EXPORT void DYNALO_CALL SignalConsumerShutdown()
{
	g_ConsumerDump2File.shutdown();
}
