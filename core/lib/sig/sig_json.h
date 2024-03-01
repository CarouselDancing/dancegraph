#pragma once

#include <nlohmann/json.hpp>
#include "signal_common.h"

namespace sig
{
    void from_json(const nlohmann::json& j, sig::SignalProperties& p) {
        j.at("jsonConfig").get_to(p.jsonConfig);
        j.at("isReliable").get_to(p.isReliable);
        j.at("keepState").get_to(p.keepState);
        j.at("isPassthru").get_to(p.isPassthru);
        j.at("producerSize").get_to(p.producerSize);
        j.at("stateSize").get_to(p.stateSize);
        j.at("consumerSize").get_to(p.consumerSize);
        j.at("networkSize").get_to(p.networkSize);
    }
}