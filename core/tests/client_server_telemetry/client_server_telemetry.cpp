#define ENET_IMPLEMENTATION

#include <inttypes.h>
#include <argparse.hpp>
#include <iostream>
#include <vector>
#include <format>

#include <spdlog/spdlog.h>

#include <core/common/utility.h>
#include <core/net/net.h>
#include <core/net/config_runtime.h>
#include <core/net/server.h>
#include <modules/env/env_common.h>
#include <core/ext/unity_inline/dancegraph.h>

#include <sig/signal_common.h>

constexpr int kClientTimeAliveSec = 20;
constexpr int kServerAliveSec = 20;
constexpr int kTelemetryStartAtSec = 15;

int main(int argc, char** argv) {

	/*
	*	args:
	*		client_idx (or -1 for server)
	*		adapter_tick_ms
	*		dancegraph_tick_ms
	*/
	
	const int client_idx = std::atoi(argv[1]);
	const bool is_server = client_idx == -1;
	const int adapter_tick_ms = std::atoi(argv[2]);
	const int dancegraph_tick_ms = std::atoi(argv[3]);

	bool is_decoupled = false;
#ifdef THREADED_DECOUPLING
	is_decoupled = true;
#endif

	//auto tp = std::chrono::utc_clock::now();
	//std::cout << sig::time_point_as_string(tp) << endl;

	std::string logStem = std::filesystem::path(argv[0]).stem().string();
	for (int i = 1; i < argc; ++i)
		logStem += fmt::format("_{}", argv[i]);
	dll::SetLogToFile(fmt::format("{}/log_{}.txt", net::cfg::DanceGraphAppDataPath(), logStem).c_str());

	const int leftDefault = 1000;
	bool ok = false;

	using namespace nlohmann;
	net::cfg::RuntimePresetsDb presetDb;
	try {
		presetDb = json::parse(dancenet::readTextFile(net::cfg::DanceGraphAppDataPath() + "/dancegraph_rt_presets.json"));
	}
	catch (json::exception e) {
		spdlog::critical("JSON presets error: {}", e.what());
		return -1;
	}

	dll::SetLogLevel(spdlog::level::warn); // warn
	if (is_server)
	{
		if (!net::cfg::Root::load())
			return -1;
		if (!net::initialize())
			return -1;

		net::ServerState server;
		net::cfg::Server cfg;
		cfg.address.ip = "192.168.0.15";
		cfg.address.port = 7777;
		cfg.scene = "impulse_test";
		auto ok = server.initialize(cfg);
		if(!ok)
			return -1;
		int telemetry_state = 0; // 0: not started, 1: sent request
		bool telemetry_started = false;
		bool telemetry_in_progress = false;
		const auto time_start = sig::time_now();
		int elapsed_sec = 0;
		while ( (elapsed_sec = std::chrono::duration_cast<std::chrono::seconds>( sig::time_now() - time_start).count()) < kServerAliveSec)
		{
			if (telemetry_state == 0 && elapsed_sec >= kTelemetryStartAtSec)
			{
				auto server_start_time = server.start_time();
				server.request_telemetry([server_start_time,is_decoupled, adapter_tick_ms, dancegraph_tick_ms](const net::TelemetryData& tdLatest) {
					static FILE* fp = nullptr;
					if (fp == nullptr)
					{
						const auto filename = std::format("e:/dancegraph/telemetry_tests-decoupled={0}-adapter_tick={1}-dancegraph_tick={2}.csv", is_decoupled, adapter_tick_ms, dancegraph_tick_ms);
						fp = fopen(filename.c_str(), "a");
						fprintf(fp, "SigType, SigIdx, UserIdx, PacketId, TelemetryCapturePoint, TimePoint, EpochReps\n");
					}

					if (fp != nullptr)
						for (const auto& [key, value] : tdLatest)
							fprintf(fp, "%d, %d, %d, %d, %s, %s, %" PRId64 "\n", key.sigType, key.sigIdx, key.userIdx, key.packetId, TelemetryCapturePointString(key.telemetryCapturePoint).data(), sig::time_point_as_string(value).c_str(), (value - server_start_time).count());

					});
			}


			server.tick();
		}
		server.deinitialize();
	}
	else
	{
		auto preset = presetDb.client["babis_impulse_test"];
		if (preset.address.ip.empty())
			preset.address.ip = net::get_local_ip_address();
		preset.username += fmt::format(" {}", client_idx);
		preset.address.port += client_idx * 10;
		json j = preset;
		dll::SetNativeTickMs(dancegraph_tick_ms);
		ok = dll::ConnectJson(j.dump().c_str());
		if(!ok)
			return -1;
		
		const auto time_start = sig::time_now();
		while (std::chrono::duration_cast<std::chrono::seconds>(sig::time_now() - time_start).count() < kClientTimeAliveSec)
		{
			dll::Tick();
			std::this_thread::sleep_for(std::chrono::milliseconds(adapter_tick_ms));
		}
		dll::Disconnect();
	}
	return 0;
}