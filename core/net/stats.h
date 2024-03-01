#pragma once

#include <cstdint>
#include <vector>

namespace sig
{
	struct SignalMetadata;
}

namespace net
{
	struct ThroughputStats
	{
		// total bytes in
		uint64_t bytesIn = 0;
		uint64_t bytesOut = 0;

		void MergeFrom(const ThroughputStats& other);
		void Add(uint64_t value, bool in) { (in ? bytesIn : bytesOut) += value; }
	};

	struct ServerStats
	{
		// Total I/O for a time windows
		ThroughputStats throughputTotal;
		std::vector<ThroughputStats> throughputPerClientSignal;
		std::vector<ThroughputStats> throughputPerEnvSignal;
		std::vector<ThroughputStats> throughputPerClient;
		std::vector<ThroughputStats> throughputPerListener;

		void ResetThroughput();
		void AddClientSignalThroughputInfo(int idx, uint64_t bytes, bool in);
		void AddEnvSignalThroughputInfo(int idx, uint64_t bytes, bool in);
		void AddClientThroughputInfo(int idx, uint64_t bytes, bool in);
		void AddListenerThroughputInfo(int idx, uint64_t bytes, bool in);
		void OnReceivedPacket(const sig::SignalMetadata& header, size_t totalSize);
		void OnSentPacket(const sig::SignalMetadata& header, size_t totalSize);
	private:
		void OnPacket(const sig::SignalMetadata& header, size_t totalSize, bool in);
	};
}