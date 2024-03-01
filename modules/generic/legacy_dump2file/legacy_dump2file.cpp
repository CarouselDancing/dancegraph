#include "legacy_dump2file.h"

#include <fstream>
#include <string>

#include <sstream>

#include <iomanip>
#include <ctime>

#include <spdlog/spdlog.h>

#include "common/config.h"

namespace sig
{
	struct ConsumerDump2File
	{
		sig::SignalProperties props;
		std::ofstream fs;

		bool init(const sig::SignalProperties& sigProp)
		{
			spdlog::info("Initializing consumer\n");
			props = sigProp;
			// TODO: read json for filename. open file for async write


			auto t = time(nullptr);
			auto lt = *localtime(&t);


			std::stringstream ss;
			ss << "ConsumerRecord_" << std::put_time(&lt, "%Y%m%d_%H%M%S") << ".timestamp";

			std::ofstream tsfs;

			tsfs = std::ofstream(ss.str().c_str(), std::ios::out);

			auto now = sig::time_now();

			auto tpsinceepoch = std::chrono::system_clock::now().time_since_epoch();
			auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(tpsinceepoch).count();
			tsfs << "{" << "\n";
			tsfs << "\"Current time\"" << ", \"" << time_point_as_string(now) << "\",\n";
			tsfs << "\"Time since epoch\"" << ", \"" << millis << "\"\n";
			tsfs << "}\n";
			tsfs.close();

			ss.str(std::string());

			ss << "ConsumerRecord_" << std::put_time(&lt, "%Y%m%d_%H%M%S") << ".dat";

			fs = std::ofstream(ss.str().c_str(), std::ios::out | std::ios::binary);

			return fs.good();

		}

		void shutdown()
		{
			spdlog::info("Shutting down consumer\n");
			// close file
			if (fs.good())
				fs.close();
		}

		void proc(const uint8_t* mem, int size, const sig::SignalMetadata& sigMeta)
		{
			// memcpy the signal, include time
			fs.write((const char *)mem, size);
		}
	};
}

sig::ConsumerDump2File g_ConsumerDump2File;

// Initialize the signal consumer (alloc resources/caches, etc)
DYNALO_EXPORT bool DYNALO_CALL SignalConsumerInitialize(const sig::SignalProperties& sigProp, const sig::SignalConsumerRuntimeConfig& cfg)
{

	config::ConfigServer globalConfig = config::ConfigServer();
	json opts = globalConfig.get_all_options(cfg.producer_type);

	spdlog::info( "{}", opts.dump(4));

	return g_ConsumerDump2File.init(sigProp);
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