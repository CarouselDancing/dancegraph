#include "state.h"

#include <chrono>
#include <deque>

#include "formatter.h"


#include "server.h"
#include "net.h"
#include "message.h"

#include <magic_enum/magic_enum.hpp>

using namespace std::chrono;

namespace net
{
	
	// Helper to track packet rates
	struct PacketRateTracker
	{
		static const int BUFFER_SIZE = 100;
		std::array<float, BUFFER_SIZE> latestDeltas;
		sig::time_point latestTime = sig::time_now();
		int head=0;

		void Add(const sig::time_point& t)
		{
			auto usecs = duration_cast<microseconds>(t - latestTime).count();
			latestDeltas[head] = usecs;
			head = (head + 1) % latestDeltas.size();
			latestTime = t;
		}

		float Average() const
		{
			auto avg = 0.000001f * std::accumulate(latestDeltas.begin(), latestDeltas.end(), 0.0f) / latestDeltas.size();
			return 1 / avg;
		}
	};

	struct MovingAverage
	{
		double accumulator = 0;
		int windowSize = 32;
		std::deque<double> samples;

		void Add(double sample)
		{
			accumulator += sample;
			samples.push_back(sample);
			if (samples.size() > windowSize)
			{
				accumulator -= samples.front();
				samples.pop_front();
			}
		}
		
		float Average() const { return (float)(accumulator / samples.size()); }
	};

	struct PacketLossTracker
	{
		MovingAverage lossAverage;
		std::bitset<64> received; // This stores the last 64 elements: 2 small windows
		int currentWin32Index = 0; // the starting small window index

		void Add(uint32_t packetId)
		{
			auto win32Index = packetId / 32;
			if (win32Index < currentWin32Index) // This might happen if client reconnects and restarts from 0 packets
			{
				currentWin32Index = win32Index;
				lossAverage = {};
				received = {};
			}
			while (win32Index > (currentWin32Index + 1))
			{
				auto raw = received.to_ullong();
				auto firstWindowBits = raw & 0xffffffff;
				auto loss = (32 - std::bitset<32>(firstWindowBits).count()) / 32.0;
				lossAverage.Add(loss);
				received = raw >> 32;
				++currentWin32Index;
			}
			received.set((win32Index - currentWin32Index) * 32 + (packetId % 32), true);
		}

		// Return between 0 and 1
		float Average() const
		{
			return lossAverage.Average();
		}
	};

	struct AllPacketTrackerData
	{
		PacketRateTracker rateTracker;
		PacketLossTracker lossTracker;
	};

	// Key is (client, signal type, signal idx)
	using packet_tracker_db = std::unordered_map<uint64_t, AllPacketTrackerData>;

	void RegisterPacketToTracker(const sig::SignalMetadata sigMeta, const sig::time_point& timeNow, const char * label)
	{
		if (sigMeta.userIdx < 0)
			return;
#if 1
		int key = sigMeta.userIdx | ((int)sigMeta.sigType << 8) | (sigMeta.sigIdx << 16);
		static packet_tracker_db db;
		static uint64_t counter = 0;
	
		auto it = db.find(key);
		if (it == db.end())
			it = db.emplace(key, AllPacketTrackerData()).first;
		it->second.rateTracker.Add(timeNow);
		it->second.lossTracker.Add(sigMeta.packetId);
		// Always measure but report every 128 iterations
		if (((++counter) & 127) == 0)
		{
			auto avg = it->second.rateTracker.Average();
			spdlog::debug("PacketRateTracker {}: Client {} SigType {} SigIdx {} : {} packets/sec", label, sigMeta.userIdx, magic_enum::enum_name<SignalType>((SignalType)sigMeta.sigType), sigMeta.sigIdx, avg);
			avg = it->second.lossTracker.Average();
			spdlog::debug("PacketLossTracker {}: Client {} SigType {} SigIdx {} : {} ", label, sigMeta.userIdx, magic_enum::enum_name<SignalType>((SignalType)sigMeta.sigType), sigMeta.sigIdx, avg*100);
		}
#endif
	}

	std::string SignalData::to_string() const
	{
		return is_valid() ? fmt::format("{} bytes, at {}", data.size(), sig::time_point_as_string(timestamp)) : "(null)";
	}

	void SignalData::from_network(const uint8_t* data_in, int size, const sig::SignalProperties& props)
	{
		data.resize(props.stateFormMaxSize);
		int stateSize = props.networkToState(data_in, size, data.data(), data.size(), nullptr);
		data.resize(stateSize);
	}

	std::string ClientInfo::to_string() const
	{
		static char ip[64];
		enet_address_get_host_ip(&address, ip, sizeof(ip));
		auto s = fmt::format("Client {} @{}:{}\n", name.c_str(), ip, address.port);
		for (const auto& sig : signals)
			s += fmt::format("\t{}\n", sig.to_string());
		return s;
	}

	std::string EnvState::to_string() const
	{
		std::string s;
		for (const auto& sig : signals)
			s += fmt::format("\t{}\n", sig.to_string());
		return s;
	}

	void EnvState::reset_user_state(int i) {
		userStates[i] = EnvUserState{};
	}


	void WorldState::on_connection_info(const sig::SignalMetadata& header, const msg::ConnectionInfo& ci, int numUserSignals)
	{
		spdlog::info("Registering client {} with name {}\n", header.userIdx, ci.name.data());
		if (clients.size() <= header.userIdx)
			clients.resize(header.userIdx + 1);
		auto& client = clients[header.userIdx];
		client.signals.resize(numUserSignals);
		if (ci.address.port == 0)
		{
			spdlog::info("Clearing client {}\n", header.userIdx);
			client = {};
		}
		else
		{
			client.address = ci.address;
			client.name = std::string(ci.name.data());
			client.time_offset_ms_ntp = ci.time_offset_ms_ntp;
		}
	}

	std::string WorldState::to_string() const
	{	
		auto s = fmt::format("WorldState: {} registered clients:\n", clients.size());
		for (const auto& client : clients)
			s += client.to_string();
		s += fmt::format("Environment: {}\n", environment.to_string());
		return s;		
	}


}