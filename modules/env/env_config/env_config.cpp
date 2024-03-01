#include <spdlog/sinks/basic_file_sink.h>

#include "env_config.h"
#include "../env_common.h"
// Config for environment signals. 



// Get the signal properties, to know what data to expect
DYNALO_EXPORT void DYNALO_CALL GetSignalProperties(sig::SignalProperties& sigProp)
{

	sigProp.set_all_sizes(ENV_SIGNAL_MAX_SIZE);
	sigProp.isReliable = true;
	sigProp.isReflexive = true;
	sigProp.isPassthru = true;
	sigProp.keepState = false; // state is kept anyway with the hardcoded structs

	sigProp.toString = [](std::string& text, const uint8_t* mem, int size, sig::SignalStage stage) {

		text = fmt::format("Env Signal (format TBD), size = {}", size);

	};
}
