#pragma once


#define DYNALO_EXPORT_SYMBOLS

#include <dynalo/symbol_helper.hpp>

#include <sig/signal_common.h>

#include <modules/mic/mic_common.h>

// Get the signal properties, to know what data to expect
DYNALO_EXPORT void DYNALO_CALL GetSignalProperties(sig::SignalProperties& sigProp);

// Get the signal in a text form, so that we can print it
DYNALO_EXPORT void DYNALO_CALL GetSignalAsText(std::string& text, const uint8_t* mem, int size, sig::SignalStage stage);