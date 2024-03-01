#pragma once

//#include "common/config.h"

#include <string>
#include <vector>

#include <memory>

#include "spdlog/formatter.h"

#include <core/net/listener.h>

class Listener
{
public:
	void Init(int argc, char** argv);
	void OnGui();
	void Start();

	// Members

	net::ListenerConfig listenerConfig;
	std::vector<std::string> signalList;
	std::string app_config_filename = "./listener-gui.json";
	/*
	std::string server_ip = "192.168.0.1";
	int server_port = 7777;
	int local_port = 7777;
	std::string listener_config;
	*/

	std::string listener_config_name = "listener_zed";
private:
	bool init = false;
	bool is_listener_running = false;
	net::ListenerState listenerState;
};
