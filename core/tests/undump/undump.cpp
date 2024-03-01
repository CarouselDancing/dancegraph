#define ENET_IMPLEMENTATION

#include <argparse.hpp>

#include <core/net/net.h>
#include <core/net/client.h>

#include <core/net/config_master.h>
#include <core/net/config_runtime.h>


// Undumper that can handle the case of a saved signal from multiple networked sources
// We'll spoof multiple users

using namespace net;
constexpr char DANCEGRAPH_PRESETS_FILENAME[] = "dancegraph_rt_presets.json";
namespace fs = std::filesystem;


void usagemessage(char* cmdname) {

#ifdef _WIN32
	char* lname = (strrchr(cmdname, '\\') ? strrchr(cmdname, '\\') + 1 : cmdname);
#else 
	char* lname = (strrchr(cmdname), '/') ? strrchr(cmdname, '/') + 1 : cmdname);

#endif 

	spdlog::info("Usage: {} --server <ip> <port> input_track runtime_preset", lname);

}


void dump_presets(net::cfg::RuntimePresetsDb & pdb) {

	std::cerr << "Available client presets:" << std::endl;
	for (auto p : pdb.client) {
		std::cerr << "\t" << p.first << std::endl;
	}
}

#if 0
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
#endif


int main(int argc, char** argv) {

	net::cfg::RuntimePresetsDb presets;

	fs::path resources_path = fs::path(net::cfg::DanceGraphAppDataPath());
	auto text = dancenet::readTextFile((resources_path / DANCEGRAPH_PRESETS_FILENAME).string());


	try {
		presets = nlohmann::json::parse(text);
	}
	catch (nlohmann::json::exception e) {
		std::cerr << "Error parsing " << resources_path << "/" << DANCEGRAPH_PRESETS_FILENAME << ": " << e.what() << std::endl;
		exit(1);
	}


	if (text.empty())
	{
		spdlog::error("presets file is empty or doesn't exist");
		return -1;
	}

	argparse::ArgumentParser program("undump.exe");
	program.add_argument("input_track")
		.help("Input file");

	program.add_argument("--server").help("server_ip server_port")
		.nargs(2)
		.required()
		.default_value(std::vector<std::string>());

	program.add_argument("preset")
		.help("Runtime preset name");
		

	try {
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error& err) {

		std::cerr << "Argument parsing error: " << err.what() << std::endl;
		usagemessage(argv[0]);
		std::exit(1);
	}

	std::string rt_preset = program.get<std::string>("preset");
	std::string input_track = program.get<std::string>("input_track");

	std::string preset_name = program.get<std::string>("preset");
	try {
		presets.client.at(preset_name);
	}
	catch (std::exception e) {
		dump_presets(presets);
		spdlog::error("Cannot find preset {}, quitting", preset_name);
		
		exit(0);
	}
	
	net::ClientState client;

	auto configp = presets.client.find(preset_name);
	if (configp == presets.client.end()) {
		spdlog::error("Error finding client preset {}", preset_name);
		dump_presets(presets);
		exit(-1);
	}

	auto config = configp->second;
	spdlog::info("Config preset {} with scene {}", preset_name, config.scene);

	std::vector<std::string> server_addr = program.get<std::vector<std::string>>("--server");

	if (server_addr.size() == 2) {
		int server_port = std::stoi(server_addr[1]);
		spdlog::info("Altering server to {},{}", server_addr[0], server_port);

		config.server_address.ip = server_addr[0];
		config.server_address.port = server_port;
	}

	spdlog::info("Attempting to connect to server {},{} and preset {}, with file {}", config.server_address.ip, config.server_address.port, preset_name, input_track);


	std::vector<uint8_t> mem_prod, mem_state, mem_net;

	std::vector<std::vector<uint8_t>>mem_cons;


	std::vector<uint8_t> mem_state2 = mem_state;
	sig::time_point time_pt;
	int counter = 0;

	bool run = true;

	std::string confdll;
	//std::string proddll;
	std::vector<std::string> consdlls;

	// We need the undumper dll

	std::string proddll = "generic/undumper/v1.0";
	
/*

	if (!cfg.get_dll_to(consname, confdll)) {
		usagemessage(argv[0]);
		spdlog::info("Can't find config dll: {}\n", consname.c_str());
		exit(-1);
	}

	if (!cfg.get_dll_to(prodname, proddll)) {
		usagemessage(argv[0]);
		spdlog::info("Can't find producer dll: {}\n", prodname.c_str());
		exit(-1);
	}

	conf.init(confdll.c_str());
	prod.init(proddll.c_str());

	SignalProperties props;
	props.logger = spdlog::default_logger();
	conf.fnGetSignalProperties(props);
	json jcfg = {
		{
			"generic",
			{
				{ "opts",
					{ { "playback_file" , program.get<std::string>("input_file") } }
				}
			}
		}
	};
	mem_prod.resize(props.producerFormMaxSize);
	mem_state.resize(props.stateFormMaxSize);
	mem_net.resize(props.networkFormMaxSize);

	props.jsonConfig = jcfg.dump(4);

	*/
	sig::SignalLibraryConfig sigconf;
	sig::SignalLibraryProducer prod;

	sig::SignalMetadata sigMeta;

	prod.init(proddll.c_str());



	while (run)
	{
		auto numRead = prod.fnGetSignalData(mem_prod.data(), sigMeta.acquisitionTime);

		if (numRead > 0)
		{
			int outputSize;
			if (props.keepState)
			{
				outputSize = props.producerToState(mem_prod.data(), numRead, mem_state.data(), mem_state.size(), nullptr);

				outputSize = props.stateToNetwork(mem_state.data(), outputSize, mem_net.data(), mem_net.size(), nullptr);
				outputSize = props.networkToState(mem_net.data(), outputSize, mem_state2.data(), mem_state2.size(), nullptr);

				for (int i = 0; i < consumer_count; i++) {
					outputSize = props.stateToConsumer(mem_state2.data(), outputSize, mem_cons[i].data(), mem_cons[i].size(), nullptr);
				}
			}
			else
			{
				outputSize = props.producerToNetwork(mem_prod.data(), numRead, mem_net.data(), mem_net.size(), nullptr);

				for (int i = 0; i < consumer_count; i++) {
					outputSize = props.networkToConsumer(mem_net.data(), mem_net.size(), mem_cons[i].data(), mem_cons[i].size(), nullptr);
				}
			}

			for (int i = 0; i < consumer_count; i++) {
				consumers[i].fnProcessSignalData(mem_cons[i].data(), outputSize, sigMeta);

			}
			sigMeta.packetId++;
		}

		if (isEscapePressed())
			run = false;
	}

	prod.fnSignalProducerShutdown();
	for (auto& c : consumers) {
		c.fnSignalConsumerShutdown();
	}

	return 0;
}



/*
#include <argparse.hpp>
#include <iostream>
#include <vector>

#include <spdlog/spdlog.h>

#include <sig/signal_producer.h>
#include <sig/signal_consumer.h>
#include <sig/signal_config.h>
	
#include <stdio.h>
#include <string.h>

#include <core/net/config_master.h>
#include <core/net/config_runtime.h>

#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <windows.h>
#endif // _WIN32

using namespace sig;
using json = nlohmann::json;

#ifdef _WIN32
bool isEscapePressed() {
	return ((GetAsyncKeyState(VK_ESCAPE) & 0x7fff) == 1);
}
#endif //WIN32


void fatal_error(const char* msg)
{
	spdlog::info(msg);
	exit(0);
}

void usagemessage(char* cmdname) {

#ifdef _WIN32
	char* lname = (strrchr(cmdname, '\\') ? strrchr(cmdname, '\\') + 1 : cmdname);
#else 
	char* lname = (strrchr(cmdname), '/') ? strrchr(cmdname, '/') + 1 : cmdname);

#endif 

	spdlog::info("Usage: {} save_file consumer1 consumer2 ...", lname);

}



int main(int argc, char** argv) {

	
	//std::string prodname = "generic/undumper";
	//std::string consname = "generic/config";
	
	argparse::ArgumentParser program("undump.exe");

	program.add_argument("input_file")
		.help("Input file");

	program.add_argument("consumerdlls")
		.help("Consumers")
		.remaining();

	try {
		program.parse_args(argc, argv);
	}
	catch (std::exception e) {
		usagemessage(argv[0]);
		exit(0);
	}

	//config::ConfigServer cfg;
	//cfg.set_opt(CONFIG_MODULE::GENERIC, "playback_file", program.get<std::string>("input_file"));

	SignalLibraryConfig conf;
	SignalLibraryProducer prod;

	std::vector<std::string> consnames;
	try {
		consnames = program.get<std::vector<std::string>>("consumerdlls");
	}
	catch(std::exception e) {
		usagemessage(argv[0]);
		exit(0);
	}

	std::string confdll;
	std::string proddll;
	std::vector<std::string> consdlls;

	if (!cfg.get_dll_to(consname, confdll)) {
		usagemessage(argv[0]);
		spdlog::info("Can't find config dll: {}\n", consname.c_str());
		exit(-1);
	}

	if (!cfg.get_dll_to(prodname, proddll)) {
		usagemessage(argv[0]);
		spdlog::info("Can't find producer dll: {}\n", prodname.c_str());
		exit(-1);
	}

	conf.init(confdll.c_str());
	prod.init(proddll.c_str());

	SignalProperties props;
	props.logger = spdlog::default_logger();
	conf.fnGetSignalProperties(props);
	json jcfg = { 
		{
			"generic",
			{
				{ "opts",
					{ { "playback_file" , program.get<std::string>("input_file") } }
				}
			}
		}
	};
	props.jsonConfig = jcfg.dump(4);	

	if (!prod.fnSignalProducerInitialize(props))
		fatal_error("Error initializing producer\n");

	sig::SignalConsumerRuntimeConfig rtcfg;
	rtcfg.client_index = SIGNAL_LOCAL_CLIENTID;

	std::string override;
	try {
		json pc = json::parse(props.jsonConfig);
		pc.at("producer_type").get_to(override);
	}
	catch (json::exception e) {
		spdlog::info(  "Failed override:{}", props.jsonConfig);
		override = config::parse_dllname(prodname).first;
	}

	spdlog::info(  "Producer type: {}", override);
	spdlog::info(  "Config: {}", props.jsonConfig);
	rtcfg.producer_type = override;
	rtcfg.producer_name = config::parse_dllname(prodname).second;
	
	// We don't want to change the metadata
	rtcfg.metadata_override = true;

	json verbopt = cfg.get_all_options(override);

	spdlog::info(  "All options:\n{}", verbopt.dump(4));

	int consumer_count = consnames.size();
	
	std::vector<SignalLibraryConsumer> consumers;

	for (int i = 0; i < consnames.size(); i++) {
		std::string condll;
		if (cfg.get_dll_to(consnames[i], condll)) {

			spdlog::info(  "Consumer {} : {}", consnames[i], condll);

			consumers.push_back(SignalLibraryConsumer());
			consumers[i].init(condll.c_str());
			if (!consumers[i].fnSignalConsumerInitialize(props, rtcfg)) {
				fatal_error("Error initializing consumer");
			}
		}
		else {
			spdlog::info("Can't find consumer dll: {}\n", consnames[i].c_str());
			usagemessage(argv[0]);
			exit(-1);
		}
	}

	std::vector<uint8_t> mem_prod, mem_state, mem_net;

	std::vector<std::vector<uint8_t>>mem_cons;

	mem_prod.resize(props.producerFormMaxSize);
	mem_state.resize(props.stateFormMaxSize);
	mem_net.resize(props.networkFormMaxSize);

	spdlog::info(  "Prod size: {}", props.producerFormMaxSize);
	spdlog::info(  "State size: {}", props.stateFormMaxSize);
	spdlog::info(  "Net size: {}", props.networkFormMaxSize);

	for (int i = 0; i < consumer_count; i++) {

		spdlog::info(  "Initializing consumer {} to {}", i, props.consumerFormMaxSize);
		mem_cons.push_back(std::vector<uint8_t>(props.consumerFormMaxSize));

	}

	std::vector<uint8_t> mem_state2 = mem_state;
	sig::time_point time_pt;
	int counter = 0;

	bool run = true;
	SignalMetadata sigMeta;
	while (run)
	{
		auto numRead = prod.fnGetSignalData(mem_prod.data(), sigMeta.acquisitionTime);

		if (numRead > 0)
		{
			int outputSize;
			if (props.keepState)
			{
				outputSize = props.producerToState(mem_prod.data(), numRead, mem_state.data(), mem_state.size(), nullptr);
				
				outputSize = props.stateToNetwork(mem_state.data(), outputSize, mem_net.data(), mem_net.size(), nullptr);
				outputSize = props.networkToState(mem_net.data(), outputSize, mem_state2.data(), mem_state2.size(), nullptr);

				for (int i = 0; i < consumer_count; i++) {
					outputSize = props.stateToConsumer(mem_state2.data(), outputSize, mem_cons[i].data(), mem_cons[i].size(), nullptr);
				}
			}
			else
			{
				outputSize = props.producerToNetwork(mem_prod.data(), numRead, mem_net.data(), mem_net.size(), nullptr);

				for (int i = 0; i < consumer_count; i++) {
					outputSize = props.networkToConsumer(mem_net.data(), mem_net.size(), mem_cons[i].data(), mem_cons[i].size(), nullptr);
				}
			}

			for (int i = 0; i < consumer_count; i++) {
				consumers[i].fnProcessSignalData(mem_cons[i].data(), outputSize, sigMeta);
				
			}
			sigMeta.packetId++;
		}

		if (isEscapePressed())
			run = false;
	}

	prod.fnSignalProducerShutdown();
	for (auto& c : consumers) {
		c.fnSignalConsumerShutdown();
	}

	return 0;

}
*/