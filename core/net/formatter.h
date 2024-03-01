#pragma once

#include <spdlog/fmt/fmt.h>

#include "state.h"

// Code for being able to print specific types with fmt::format

template <>
struct fmt::formatter<ENetAddress> : fmt::formatter<std::string> {
	auto format(const ENetAddress& a, format_context& ctx) {
		char buf[64];
		enet_address_get_host_ip(&a, buf, sizeof(buf));
		return formatter<std::string>::format(fmt::format("{}:{}", buf, a.port), ctx);
	}
};

template <>
struct fmt::formatter<net::ClientInfo> : fmt::formatter<std::string> {
	auto format(const net::ClientInfo& w, format_context& ctx) {
		return formatter<std::string>::format(w.to_string(), ctx);
	}
};

template <>
struct fmt::formatter<net::WorldState> : fmt::formatter<std::string> {
	auto format(const net::WorldState& w, format_context& ctx) {
		return formatter<std::string>::format(w.to_string(), ctx);
	}
};