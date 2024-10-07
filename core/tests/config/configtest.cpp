#include <string>
#include <iostream>
#include <vector>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ranges.h>

#include <core/net/config_master.h>
#include <core/net/config_runtime.h>
#include <argparse.hpp>

#include <nlohmann/json.hpp>


struct SettingsFile {
	net::cfg::Client preset;
	bool autoConnect;
	int logLevel;
	std::string logToFileStem;
	bool simpleSkeletonSmoothing;
	nlohmann::json avatar;
};


NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SettingsFile, preset, autoConnect, logLevel, logToFileStem, simpleSkeletonSmoothing, avatar);


void runtime_cfg_check(std::string rtfile) {

	SettingsFile sfile;
	
	auto text = dancenet::readTextFile(rtfile);


	try {
		sfile = nlohmann::json::parse(text);
	}
	catch (nlohmann::json::exception e) {
		std::cerr << "Error parsing " << rtfile << ": " << e.what() << std::endl;
		exit(1);
	}
	
	spdlog::info("Found {} transformers", sfile. preset.transformers.size());
	for (auto& t : sfile.preset.transformers) {
		spdlog::info("Found transformer ", t.name);
	}

}


int main(int argc, char** argv) {

	argparse::ArgumentParser program("configtest");

	program.add_argument("--runtime").help("runtime cfg");
	

	try {
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error& err) {

		std::cerr << "Argument parsing error: " << err.what() << std::endl;
		std::exit(1);
	}


	std::string rtfile = program.get<std::string>("--runtime");
	
	if (rtfile.size() > 0) {
		runtime_cfg_check(rtfile);
		exit(0);
	}

	bool loader = net::cfg::Root().load();
	if (!loader) {
		spdlog::error("Config failed to load");
		exit(0);
	}
	auto & master_cfg = net::cfg::Root().instance();

	spdlog::info("Master config user signals: ");
	for (auto& us : master_cfg.user_signals) {
		spdlog::info("\t{}", us.first);
		for (auto& usd : us.second) {
			spdlog::info("\t\t{}", usd.first);
			spdlog::info("\t\t\tLocal");
			spdlog::info("\t\t\t{}", usd.second.opts.dump(4));
			spdlog::info("\t\t\tGlobal");
			spdlog::info("\t\t\t{}", usd.second.globalopts.dump(4));

		}

	}

	auto transformers = master_cfg.transformers;

	spdlog::info("Found {} transformers: ", transformers.size());

	for (auto & t : transformers) {
		spdlog::info("T: {}", t.first);
	}

	auto zedopts = master_cfg.user_signals.at("zed").at("v1.0").opts;
	auto zedglobalopts = master_cfg.user_signals.at("zed").at("v1.0").globalopts;
	zedopts.merge_patch(zedglobalopts);

	spdlog::info("Test options for zed/v1.0: {}", zedopts.dump(4));

}

