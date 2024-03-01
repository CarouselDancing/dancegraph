#pragma once

#include <string>
#include <memory>

#include <nlohmann/json.hpp>

#include <ipc/ringbuffer.h>

#include <core/net/config_master.h>
#include <core/net/config_runtime.h>

namespace dll {
	// Class inside the DanceNet plugin for receiving ZED signals via IPC and sending them across the Unity C# boundary
	// Essentially, this is a wrapper round the IPC reader that joins the data being read via IPC to the Unity Data structures in C#

	// Environment messages are reliable, so we should have a reliable IPC mechanism set up, rather than the DNRingbuffer(Writ|Read)er



	struct BufferProperties {
		std::string name;
		int numEntries;

		void populate_from_json(nlohmann::json overrides) {
			auto& master_cfg = net::cfg::Root::instance();
			try {
				nlohmann::json allopts = master_cfg.env_signals.at("env").at("opts");
				name = allopts.at("ipcOutBufferName");
			}
			catch (nlohmann::json::exception e) {
				name = "Dancegraph_Env_Out";
			}
			try {
				nlohmann::json allopts = master_cfg.env_signals.at("env").at("opts");
				numEntries = allopts.at("ipcBufferEntries");
			}
			catch (nlohmann::json::exception e) {
				numEntries = 15;
			}

		}

	};


	

	void env_initialize();
	void env_shutdown();

	class EnvAdapter {

	public:
		bool read_ipc();
		//bool write_ipc();

		~EnvAdapter();
		EnvAdapter(std::string fname, std::string tname, int size, int numEntries);

		int get_env_msg(char[]);
		bool send_env_msg(char[], int size);

		int lastReadSize;

	protected:
		ipc::DNRingbufferReader ipcReader;
		ipc::DNRingbufferWriter ipcWriter;
	};

}
