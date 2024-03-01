#pragma once

#define DYNALO_EXPORT_SYMBOLS
#include <dynalo/symbol_helper.hpp>

#include <sig/signal_common.h>

// Get the signal properties, to know what data to expect
DYNALO_EXPORT void DYNALO_CALL GetSignalProperties(sig::SignalProperties& sigProp);