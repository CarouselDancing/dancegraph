#include <thread>

#define ENET_IMPLEMENTATION
#include <enet/enet.h>

#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>

#include <core/net/net.h>
#include <core/net/server.h>
#include <core/net/client.h>
#include <core/net/listener.h>

#include <core/net/formatter.h>

#include <argparse.hpp>

#include <string>

#include <core/net/config_master.h>
#include <core/net/config_runtime.h>

#include <map>



#ifdef _WIN32
bool isEscapePressed() {
	// Returns true if escape is NOT pressed
	return ((GetAsyncKeyState(VK_ESCAPE) & 0x7fff) != 1);
}
#endif //WIN32


using namespace net;
namespace fs = std::filesystem;


constexpr char DANCEGRAPH_PRESETS_FILENAME[] = "dancegraph_rt_presets.json";


// (off/critical/err/warn/*info/debug/trace
std::map<std::string, spdlog::level::level_enum > logmap = std::map<std::string, spdlog::level::level_enum>{
	{std::string("off"), spdlog::level::off},
	{std::string("critical"), spdlog::level::critical},
	{std::string("err"), spdlog::level::err},
	{std::string("warn"), spdlog::level::warn},
	{std::string("info"), spdlog::level::info},
	{std::string("debug"), spdlog::level::debug},
	{std::string("trace"), spdlog::level::trace}
};


void dump_presets(net::cfg::RuntimePresetsDb & pdb) {
	
	std::cerr << "Available client presets:" << std::endl;
	for (auto p : pdb.client) {
		std::cerr << "\t" << p.first << std::endl;
	}
	std::cerr << "Available server presets:" << std::endl;
	for (auto p : pdb.server) {
		std::cerr << "\t" << p.first << std::endl;
	}

	std::cerr << "Available listener presets:" << std::endl;
	for (auto p : pdb.listener) {
		std::cerr << "\t" << p.first << std::endl;
	}


}


int main(int argc, char** argv) {


	//	config::ConfigServer cfg;
	/*
	std::vector<std::string> confignames = cfg.get_net_configs();
	spdlog::info(  "Available configs are:");
	for (std::string c : confignames) {
		spdlog::info(  "\t{}", c);
	}
	spdlog::info(  "");
	*/

	auto local_ip = net::get_local_ip_address();
	spdlog::info("Local IP address: {}\n", local_ip.c_str());

	argparse::ArgumentParser program("nettest");

	program.add_argument("--mode").help("client/server/listener mode")
		.nargs(1)
		.required()
		.default_value(std::string(""));

	program.add_argument("--server").help("server_ip server_port")
		.nargs(2)
		.required()
		.default_value(std::vector<std::string>());

	program.add_argument("--localport").help("local_port")
		.nargs(1)		
		.default_value(std::string("7777"));
		
	program.add_argument("--loglevel").help("log_level (off/critical/err/warn/*info/debug/trace)")
		.nargs(1)
		.default_value(std::string("info"));


	// ./nettest --replay <replay_file> <config_name>
	// This is just shorthand for ./nettest --option generic playback_file <replay_file> <config_name>
	program.add_argument("--replay").help("signal_type input_file (should also load a config with the generic/undumper producer)")
		.nargs(2);

	program.add_argument("--option").help("signal_type1 option1 value1 signal_type2 option2 value2 ... ")
		.nargs(argparse::nargs_pattern::at_least_one)
		.default_value(std::vector<std::string>());
	
	program.add_argument("preset").help("configuration preset");



	try {
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error& err) {

		std::cerr << "Argument parsing error: " << err.what() << std::endl;
		std::exit(1);
	}


	std::string mode = program.get<std::string>("mode");
	std::string logstring = program.get<std::string>("loglevel");
	const auto preset_name = program.get<std::string>("preset");

	if (!cfg::Root::load())
	{
		spdlog::error("Failed loading dancegraph.json");
		exit(-1);
	}

	try {

		spdlog::level::level_enum ll = logmap[logstring];
		spdlog::set_level(ll);

	}
	catch (std::exception e) {
		spdlog::set_level(spdlog::level::critical);
	}

	std::cerr << "Looking for preset " << preset_name << " in mode " << mode << "\n";
	

	// First the APPDATA
	// First the source resources
	// then the current dir


	
	//fs::path localappdata_path = std::filesystem::path(net::cfg::DanceGraphAppDataPath());
	fs::path resources_path = fs::path(net::cfg::DanceGraphAppDataPath());

	//fs::path resources_path = fs::path(__FILE__).parent_path() / ".." / ".." / "resources";

	if (!fs::is_directory(resources_path))
		resources_path = fs::current_path() / "resources";

	if (!fs::is_directory(resources_path))
	{
		spdlog::error("Cannot file resources path at {}", resources_path.string());
	}
	auto text = dancenet::readTextFile((resources_path / DANCEGRAPH_PRESETS_FILENAME).string());

	if (text.empty())
	{
		spdlog::error("presets file is empty or doesn't exist");
		return -1;
	}
	
	net::cfg::RuntimePresetsDb presets;
	

	try {
		presets = nlohmann::json::parse(text);
	}
	catch (nlohmann::json::exception e) {
		std::cerr << "Error parsing " << resources_path << "/" << DANCEGRAPH_PRESETS_FILENAME << ": " << e.what() << std::endl;
		exit(1);
	}

	if (!initialize())
		exit(-1);

	nlohmann::json extra_producer_options = nlohmann::json({});
	nlohmann::json extra_consumer_options = nlohmann::json({});

	std::vector<std::string> option_info = program.get<std::vector<std::string>>("--option");

	spdlog::info(  "Option Info Size is {}", option_info.size());

	if (option_info.size() == 0) {
		// No options to parse, nothing to see here
	}
	else if (option_info.size() % 3 != 0) {
		std::cerr << "Problem parsing --option arguments; must be in <signaltype> <option name> <option value> triples\n";

		std::exit(1);

	}
	else {
		std::cerr << "Extra program options not currently supported" << std::endl;
		
	}
	
	std::vector<std::string> replay_files = program.get<std::vector<std::string>>("--replay");

	if (replay_files.size() % 2 != 0) {
		std::cerr << "Problem parsing --replay arguments; must be in <signaltype> <replayfile> pairs\n";
		std::exit(1);
	}


	int portoverride;
	try {

		std::string portstr = program.get<std::string>("--localport");
		portoverride = std::stoi(portstr);		
	}
	catch (std::exception e) {
		spdlog::info("Failed local port override: {}", e.what());
	}
	

	if (mode == "client")
	{
		ClientState client;

		auto configp = presets.client.find(preset_name);
		if (configp == presets.client.end()) {
			spdlog::error("Error finding client preset {}", preset_name);			
			dump_presets(presets);
			exit(-1);
		}

		auto config = configp->second;
		spdlog::info("Config preset {} with scene {}", preset_name, config.scene);

		config.address.port = portoverride;

		std::vector<std::string> server_addr = program.get<std::vector<std::string>>("--server");

		if (server_addr.size() == 2) {
			int server_port = std::stoi(server_addr[1]);
			spdlog::info("Altering server to {},{}", server_addr[0], server_port);

			config.server_address.ip = server_addr[0];
			config.server_address.port = server_port;
		}
		

		for (int i = 0; i < replay_files.size(); i += 2) {
			std::string sig = replay_files[i];
			std::string replay_file = replay_files[i + 1];

			for (auto& prod : config.producer_overrides) {
				spdlog::info("Checking {} against {}", prod.first, sig);
				if (sig == prod.first) {					
					spdlog::info("Got {}", prod.second.opts.dump(4));
					spdlog::info("Prev Opts: {}", config.producer_overrides[prod.first].opts.dump(4));
					config.producer_overrides[prod.first].opts[sig]["playback_file"] = replay_file;
				}
			}
		}


		for (auto& prod : config.producer_overrides) {
			spdlog::info("Prod options: {} {}", prod.first, config.producer_overrides[prod.first].opts.dump(4));
		}
		
		client.run(config, isEscapePressed);
		spdlog::info("Quitting normally");
	}
	else if (mode == "server")
	{
		//try {
			ServerState server;
			auto configp = presets.server.find(preset_name);
			if (configp == presets.server.end()) {
				std::cerr << "Error finding server preset " << preset_name << std::endl;
				dump_presets(presets);
				exit(-1);
			}

			auto config = configp->second;
			
			server.run(config, isEscapePressed);
			spdlog::info("Quitting normally");

}
	else if (mode == "listener")

	{
		ListenerState listener;
		//net::cfg::Listener listencfg = net::cfg::Listener();

		auto configp = presets.listener.find(preset_name);
		if (configp == presets.listener.end()) {
			spdlog::error("Error finding client preset {}", preset_name);
			dump_presets(presets);
			exit(-1);
		}

		auto config = configp->second;
		spdlog::info("Config preset {} with scene {}", preset_name, config.scene);

		config.address.port = portoverride;

		std::vector<std::string> server_addr = program.get<std::vector<std::string>>("--server");
		if (server_addr.size() == 2) {
			int server_port = std::stoi(server_addr[1]);
			spdlog::info("Altering server to {},{}", server_addr[0], server_port);

			config.server_address.ip = server_addr[0];
			config.server_address.port = server_port;
		}

		listener.run(config, isEscapePressed);
		spdlog::info("Quitting normally");
	}
	else {
		spdlog::info("Unknown networking mode: {}", mode);
	}
}
