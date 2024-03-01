#pragma once

#define DYNALO_EXPORT_SYMBOLS
#include <dynalo/symbol_helper.hpp>
#include <nlohmann/json.hpp>

#include <sig/signal_common.h>
#include "modules/impulse/impulse_signal_common.h"


#define NLOHMANN_JSON_FROM_OPTIONAL(v1) if(nlohmann_json_j.find(#v1) != nlohmann_json_j.end()) nlohmann_json_j.at(#v1).get_to(nlohmann_json_t.v1);
#define JSON_FROM(Type, ...) inline void from_json(const nlohmann::json & nlohmann_json_j, Type & nlohmann_json_t) { NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_FROM_OPTIONAL, __VA_ARGS__)); }

JSON_FROM(ImpulseSignalConfig, datasize, interval_ms, sleep_ms);

// Get the signal properties, to know what data to expect; also some properties need to be set, primarily the data sizes (typically with sigProp.set_all_sizes)
DYNALO_EXPORT void DYNALO_CALL GetSignalProperties(sig::SignalProperties& sigProp);