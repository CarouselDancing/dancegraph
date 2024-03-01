#define ENET_IMPLEMENTATION

#include <argparse.hpp>
#include <iostream>
#include <vector>

#include <spdlog/spdlog.h>

#include <core/common/utility.h>
#include <core/net/net.h>
#include <core/net/config_runtime.h>
#include <modules/env/env_common.h>
#include <core/ext/unity_inline/dancegraph.h>

#include <sig/signal_common.h>

int main(int argc, char** argv) {

	//auto tp = std::chrono::utc_clock::now();
	//std::cout << sig::time_point_as_string(tp) << endl;

	std::string logStem = std::filesystem::path(argv[0]).stem().string();
	for (int i = 1; i < argc; ++i)
		logStem += fmt::format("_{}", argv[i]);
	dll::SetLogToFile(fmt::format("{}/log_{}.txt", net::cfg::DanceGraphAppDataPath(), logStem).c_str());

	const int leftDefault = 1000;
	bool ok = false;
	int i = 0;
	if (argc > 2)
		i = std::stoi(argv[2]);

	if (true)
	{
		using namespace nlohmann;
		net::cfg::RuntimePresetsDb presetDb;
		try {
			presetDb = json::parse(dancenet::readTextFile(net::cfg::DanceGraphAppDataPath() + "/dancegraph_rt_presets.json"));
		}
		catch (json::exception e) {
			spdlog::critical("JSON presets error: {}", e.what());
			return -1;
		}
		auto preset = presetDb.client["babis_impulse_test"];
		if (preset.address.ip.empty())
			preset.address.ip = net::get_local_ip_address();
		preset.username += fmt::format(" {}", i);
		preset.address.port += i * 10;
		json j = preset;
		ok = dll::ConnectJson(j.dump().c_str());
	}
	else
	{
		std::vector<std::string> singlefiles = {
		"QuantSingleTrack_143415_01.dgs",
		 "QuantSingleTrack_143411_01.dgs",
		 "QuantSingleTrack_151545_00.dgs",
		 "QuantSingleTrack_151553_01.dgs"
		};


		int i = 0;
		if (argc > 2)
			i = std::stoi(argv[2]);

		const char* scene_name = "zed_testscene";
		const char* user_role = "dancer";
		const int this_port = 7927 + i * 10;
		const int server_port = 7777;
		std::string client_name = fmt::format("test client minimal {0}", i + 1);
		const char* server_ip = "192.168.0.15";
		dll::SetZedReplay((std::string("C:/Users/esrev/Documents/repos/dancenet-plugin-ipc/scripts/multilaunch/") + singlefiles[i]).c_str());
		bool ok = dll::Connect(client_name.c_str(), server_ip, server_port, this_port, scene_name, user_role);

	}
	
	dll::SetLogLevel(spdlog::level::warn); // warn
	
	if (!ok)
		return -1;
	int left = argc > 1 ? std::stoi(argv[1]) : leftDefault;
	bool isPlaying = true;
	while (--left >= 0)
	{
		dll::Tick();
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(60ms);
#if 0
		if ((left % 200) == 0)
		{
			EnvMusicState envMusicState;
			memset(&envMusicState, 0, sizeof(EnvMusicState));
			envMusicState.signalID = EnvMusicStateID;
			envMusicState.mBody.isPlaying = isPlaying;
			isPlaying = !isPlaying;
			const int sigIdx = 0; // This is 0 because in the scene env signals we just specify ONE!
			dll::SendEnvSignal((const uint8_t*)&envMusicState, sizeof(envMusicState), sigIdx);
		}

		if ((left % 250) == 0)
		{
			EnvUserState user;
			memset(&user, 0, sizeof(EnvMusicState));
			user.mBody.userID = dll::GetUserIdx();
			user.mBody.isActive = 1;
			auto name = fmt::format("user whose iters left are {}", left);
			strcpy(user.mBody.userName, name.c_str());
			strcpy(user.mBody.avatarDesc, name.c_str());
			user.signalID = EnvUserStateID;
			const int sigIdx = 0; // This is 0 because in the scene env signals we just specify ONE!
			dll::SendEnvSignal((const uint8_t*)&user, sizeof(user), sigIdx);
		}
#endif
	}
	dll::Disconnect();
	return 0;
}