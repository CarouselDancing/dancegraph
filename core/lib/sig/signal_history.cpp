

#include "signal_history.h"
#include <sig/signal_common.h>
#include <core/net/config_master.h>
#include <algorithm>

using namespace sig;



// Sets the new signal entry
void SignalHistory::add(const uint8_t* entry, int signal_size, const sig::SignalMetadata& metaData) {

	std::shared_ptr<char[]> q_entry = std::shared_ptr<char[]>(new char[max_signal_size]);

	if (signal_size <= 0)
		memcpy((void*)q_entry.get(), (void*)entry, max_signal_size);
	else
		memcpy((void*)q_entry.get(), (void*)entry, signal_size);


	std::shared_ptr<sig::SignalMetadata> m_entry = std::make_shared<sig::SignalMetadata>();
	*m_entry = metaData;

	//spdlog::debug("Adding {} bytes for packet {}", signal_size, metaData.packetId);

	/*
	std::stringstream ss;
	ss = std::stringstream("");
	if (signal_size > 16) {
		for (int i = 0; i < signal_size; i++) {
			//ss << *(q_entry.get() + i) << ", ";
			ss << (int)*(entry + i) << ", ";

		}
		spdlog::debug("Bytes queued from packet {} is {}", metaData.packetId, ss.str());
	}
	*/


	if (signalList.size() < max_queue_size) {
		signalList.push_back(q_entry);
		metaList.push_back(m_entry);
	}
	else if (_lockedForRead) {
		signalList.push_back(q_entry);
		metaList.push_back(m_entry);
	}
	else {
		signalList.pop_front();
		metaList.pop_front();

		signalList.push_back(q_entry);
		metaList.push_back(m_entry);
	}
}



void SignalHistory::lock() {
	_lockedForRead = true;
}

void SignalHistory::unlock() {
	// Now we prune back down to the maxSize entries
	for (int i = max_queue_size; i < signalList.size(); i++) {
		signalList.pop_front();
		metaList.pop_front();
	}

	_lockedForRead = false;
}


std::shared_ptr<sig::SignalMetadata> SignalHistory::getmetadata(int idx) {
	auto it_m = metaList.begin();
	for (int i = 0; i < idx && it_m != metaList.end(); i++, it_m++);
	if (it_m == metaList.end())
		return nullptr;
	return *it_m;
}

std::shared_ptr<char[]> SignalHistory::getdata(int idx) {
	auto it_d = signalList.begin();
	for (int i = 0; i < idx && it_d != signalList.end(); i++, it_d++) {
	}
	if (it_d == signalList.end()) {
		return nullptr;
	}
	return *it_d;
}


void SignalHistory::initialize(int queue_size, size_t signal_size) {

	_lockedForRead = false;

	max_queue_size = queue_size;
	max_signal_size = signal_size;

	signalList = std::list<std::shared_ptr<char[]>>();
	metaList = std::list<std::shared_ptr<sig::SignalMetadata>>();

}

// Size of the available queue elements (i.e. ignoring the extra ones allowed via locking)
int SignalHistory::queue_size() {
	return (signalList.size() > max_queue_size) ? max_queue_size : signalList.size();

}

// Just spam the history to spdlog
void SignalHistory::dump_history(std::string prefix) {
	std::stringstream ss{ "" };

	for (int i = 0; i < signalList.size(); i++) {
		ss << getmetadata(i)->packetId << " ";
	}
	spdlog::debug("Sig Queue [{}]: size: {}, packets: {}", prefix, signalList.size(), ss.str());
}



void sig::SignalMemory::registerSignal(std::string signalName, net::SignalType sigType, int sigIdx, int signalSize, int queueSize) {
	spdlog::debug("Adding signal {} ({}/{}) to transformer memory", signalName, (int)sigType, sigIdx);
	switch (sigType) {
	case net::SignalType::Environment:
		if (sigIdx >= signal_memories_env.size())
			signal_memories_env.resize(sigIdx + 1);
		signal_memories_env[sigIdx] = sig::SignalHistory{};
		signal_memories_env[sigIdx].initialize(queueSize, signalSize);

		break;
	case net::SignalType::Client:
		if (sigIdx >= signal_memories_user.size())
			signal_memories_user.resize(sigIdx + 1);
		signal_memories_user[sigIdx] = sig::SignalHistory{};
		signal_memories_user[sigIdx].initialize(queueSize, signalSize);
		break;
	default:
		break;
		// We're not touching any Control signals as of yet
	}

	signalMapping[signalName] = std::pair<net::SignalType, int>(sigType, sigIdx);

	spdlog::debug("Registering signal history {} {}/{}", signalName, (int)sigType, sigIdx);

}


//sig::SignalHistory NULL_HISTORY = sig::SignalHistory{};



sig::SignalHistory* sig::SignalMemory::get_history(std::string name) {
	auto& sigpair = signalMapping[name];
	switch (sigpair.first) {
	case net::SignalType::Environment:

		return &signal_memories_env[sigpair.second];
		break;
	case net::SignalType::Client:
		return &signal_memories_user[sigpair.second];
		break;
	default:
		spdlog::error("Signal history for unsupported signal type: {}", (int)sigpair.first);
		return nullptr;
		break;
	}
}

