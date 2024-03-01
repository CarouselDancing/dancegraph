#include "stats.h"

#include <sig/signal_common.h>

#include "channel.h"

namespace net
{
	void ThroughputStats::MergeFrom(const ThroughputStats& other)
	{
		bytesIn += other.bytesIn;
		bytesOut += other.bytesOut;
	}

	void ServerStats::ResetThroughput()
	{
		throughputTotal = {};
		std::fill(throughputPerClientSignal.begin(), throughputPerClientSignal.end(), ThroughputStats());
		std::fill(throughputPerEnvSignal.begin(), throughputPerEnvSignal.end(), ThroughputStats());
		std::fill(throughputPerClient.begin(), throughputPerClient.end(), ThroughputStats());
		std::fill(throughputPerListener.begin(), throughputPerListener.end(), ThroughputStats());
		throughputPerClientSignal.resize(0);
		throughputPerEnvSignal.resize(0);
		throughputPerClient.resize(0);
		throughputPerListener.resize(0);
	}

	void UpdateThroughputVector(std::vector<ThroughputStats>& vec, int idx, uint64_t bytes, bool in)
	{
		if (idx >= vec.size())
			vec.resize(idx + 1);
		vec[idx].Add(bytes, in);
	}

	void ServerStats::AddClientSignalThroughputInfo(int idx, uint64_t bytes, bool in)
	{
		UpdateThroughputVector(throughputPerClientSignal, idx, bytes, in);
	}

	void ServerStats::AddEnvSignalThroughputInfo(int idx, uint64_t bytes, bool in)
	{
		UpdateThroughputVector(throughputPerEnvSignal, idx, bytes, in);
	}

	void ServerStats::AddClientThroughputInfo(int idx, uint64_t bytes, bool in)
	{
		UpdateThroughputVector(throughputPerClient, idx, bytes, in);
	}

	void ServerStats::AddListenerThroughputInfo(int idx, uint64_t bytes, bool in)
	{
		UpdateThroughputVector(throughputPerListener, idx, bytes, in);
	}

	void ServerStats::OnPacket(const sig::SignalMetadata& header, size_t totalSize, bool in)
	{
		throughputTotal.Add(totalSize, in);
		// if we have no user info, ignore any further classification
		if (header.userIdx < 0)
			return; 
		int idx = header.userIdx;
		auto sigType = (SignalType)header.sigType;
		auto is_ctrl = sigType == SignalType::Control;
		auto ctrl_type = (ControlSignal)header.sigIdx;
		bool isListener = is_ctrl && (ctrl_type == ControlSignal::NewListenerConnection || ctrl_type == ControlSignal::NewListenerConnectionResponse);
		if (sigType == SignalType::Client)
			AddClientSignalThroughputInfo(header.sigIdx, totalSize, in);
		else if (sigType == SignalType::Environment)
			AddEnvSignalThroughputInfo(header.sigIdx, totalSize, in);
		if (isListener)
			AddListenerThroughputInfo(idx, totalSize, in);
		else
			AddClientThroughputInfo(idx, totalSize, in);
	}

	void ServerStats::OnReceivedPacket(const sig::SignalMetadata& header, size_t totalSize)
	{
		OnPacket(header, totalSize,true);
	}

	void ServerStats::OnSentPacket(const sig::SignalMetadata& header, size_t totalSize)
	{
		OnPacket(header, totalSize, false);
	}
}