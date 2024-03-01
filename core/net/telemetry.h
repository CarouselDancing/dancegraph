#pragma once

#include <magic_enum/magic_enum.hpp>

#include <sig/signal_common.h>

namespace net
{
	// Telemetry is packet-oriented, recording how the packet travels from one client to others via the server

	// In the server, store:
	//		For each client
	//			for each signal type
	//				for each signal index
	//					record ALL PACKETS

	// "Key" should be sigtype/sigidx/clientIdx/packetId

	// Maybe separate receipt at remote clients, and keep it in a map: < (key_t, remote_receipient_id), time_point>


	// This describes the different stages of a packet's data
	enum class TelemetryCapturePoint : uint32_t
	{
		DataProduction=0,						// data gets produced -- this is stored in signal metadata
		SendingToServer,						// data is about to be sent to the server
		ReceivingAtServer,						// data was just received at the server
		ReceivingAtClient0Native,				// data was just received at remote client 0 (add client index for a different client)
		ReceivingAtClient0Framework = 0xffff, 	// data was just received at remote client 0's framework app, e.g. Unity (add client index for a different client)
	};

	static std::string TelemetryCapturePointString(TelemetryCapturePoint value)
	{
		auto uvalue = static_cast<uint32_t>(value);
		if (value <= TelemetryCapturePoint::ReceivingAtServer)
			return magic_enum::enum_name<net::TelemetryCapturePoint>(value).data();
		else if(value < TelemetryCapturePoint::ReceivingAtClient0Framework)
			return std::format("ReceivingAtClientNative {}", uvalue - static_cast<uint32_t>(TelemetryCapturePoint::ReceivingAtClient0Native));
		else
			return std::format("ReceivingAtClientFramework {}", uvalue - static_cast<uint32_t>(TelemetryCapturePoint::ReceivingAtClient0Framework));
	}

	template <class T>
	inline void hash_combine(std::size_t& s, const T& v)
	{
		std::hash<T> h;
		s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
	}

	struct TelemetryKey
	{
		uint8_t sigType=0;
		uint8_t sigIdx=0;
		int16_t userIdx=0;
		uint32_t packetId=0;
		TelemetryCapturePoint telemetryCapturePoint= TelemetryCapturePoint::DataProduction;

		bool operator == (const TelemetryKey& other) const { return memcmp(this, &other, sizeof(TelemetryKey)) == 0; }
	};

	using TelemetryData = std::unordered_map<TelemetryKey, sig::time_point>;
}

namespace std {
	template <>
	struct hash<net::TelemetryKey> {
		auto operator()(const net::TelemetryKey& tkey) const -> size_t {
			static_assert(sizeof(net::TelemetryKey) == 12, "Unexpected size of net::TelemetryKey");
			size_t h = std::hash<uint32_t>()(*(const uint32_t*)this);
			net::hash_combine(h, tkey.packetId);
			net::hash_combine(h, tkey.telemetryCapturePoint);
			return h;
		}
	};
}  // namespace std