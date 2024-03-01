#include "mic_signal_config.h"

#include <array>

#include <spdlog/fmt/fmt.h>

#include <nlohmann/json.hpp>

#include <core/common/utility.h>

using json = nlohmann::json;

// Get the signal properties, to know what data to expect
DYNALO_EXPORT void DYNALO_CALL GetSignalProperties(sig::SignalProperties& sigProp)
{
	sigProp.keepState = true;
	int packetSize = sizeof(MicPacketData);
	auto jText = dancenet::readTextFile("mic-signal-config.json");
	if (!jText.empty())
	{
		auto j = json::parse(jText);
		if (j.contains("packetSize"))
			j.at("packetSize").get_to(packetSize);
	}
	sigProp.set_all_sizes(packetSize);

	sigProp.toString = [](std::string& text, const uint8_t* mem, int size, sig::SignalStage stage) {
		// print the first 4 characters
		text = fmt::format("{} {} {} {}... ({})", mem[0], mem[1], mem[2], mem[3], size);
	};
}