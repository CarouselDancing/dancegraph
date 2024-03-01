
#define DYNALO_EXPORT_SYMBOLS
#include <dynalo/symbol_helper.hpp>


#include <spdlog/spdlog.h>

#include <nlohmann/json.hpp>

#include "sig/signal_common.h"

#include "impulse_signal_config.h"

#include "modules/impulse/impulse_signal_common.h"


// Get the signal properties, to know what data to expect; also set the data size
DYNALO_EXPORT void DYNALO_CALL GetSignalProperties(sig::SignalProperties& sigProp)
{
	
	spdlog::set_default_logger(sigProp.logger);
	ImpulseSignalConfig cfg = nlohmann::json::parse(sigProp.jsonConfig);
	int dataSize = cfg.datasize;
	spdlog::info(  "Impulse Server {} bytes @ {}ms sleep={}ms", dataSize, cfg.interval_ms, cfg.sleep_ms);
	sigProp.keepState = true;
	sigProp.set_all_sizes(dataSize); // configurable data size
}

