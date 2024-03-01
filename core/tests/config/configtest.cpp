

#include <string>
#include <iostream>
#include <vector>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ranges.h>

#include <core/net/config_master.h>


int main(int argc, char** argv) {
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

	auto zedopts = master_cfg.user_signals.at("zed").at("v1.0").opts;
	auto zedglobalopts = master_cfg.user_signals.at("zed").at("v1.0").globalopts;
	zedopts.merge_patch(zedglobalopts);

	spdlog::info("Test options for zed/v1.0: {}", zedopts.dump(4));

}

