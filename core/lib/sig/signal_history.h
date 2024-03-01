#pragma once

#include <list>

#include <sig/signal_common.h>

#include <spdlog/spdlog.h>

// A generic class for storing and retrieving a signal history of a maximum size

namespace sig {
	class SignalHistory {

		// The producer end of the transformer is reading the data, so don't clear old entries
		bool _lockedForRead;
		bool _dataSinceLock;

		int max_queue_size;
		

	public:

		// We only want to access the last maxSize elements

		std::list<std::shared_ptr<char[]>> signalList;
		std::list<std::shared_ptr<sig::SignalMetadata>> metaList;

		void initialize(int queue_size, size_t signal_size);

		size_t max_signal_size;

		// Sets the new signal entry
		void add(const uint8_t * entry, int size, const sig::SignalMetadata& metaData);

		void lock();
		void unlock();

		// Has there been new data since the last lock?
		bool new_data_arrived();

		int queue_size();

		void dump_history(std::string prefix);


		// Returns a pointer to the 'nth' element in the history; nullptr if there are no entries to return
		// If this is being called from another thread, then the call should be preceded by 'lock()', and 'unlock()' when you're done

		std::shared_ptr<sig::SignalMetadata> getmetadata(int n);
		std::shared_ptr<char[]> getdata(int n);

	};

	struct SignalMemory {
	
		// The signal Indices overlap so this gets round that
		std::vector<sig::SignalHistory> signal_memories_user;
		std::vector<sig::SignalHistory> signal_memories_env;

		std::unordered_map<std::string, std::pair<net::SignalType, int>> signalMapping;

		int get_history_size(std::string signalName);

		void registerSignal(std::string signalName, net::SignalType sigType, int sigIdx, int signalSize, int queueSize);

		sig::SignalHistory * get_history(std::string name);

		void initialize() {
			spdlog::debug("SignalMemory Container initialized");
			signal_memories_user = std::vector<sig::SignalHistory>{};
			signal_memories_env = std::vector<sig::SignalHistory>{};
		}
	};
}


