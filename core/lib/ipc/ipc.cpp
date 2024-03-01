
#include <map>
#include <string>

#include <spdlog/spdlog.h>

#include "ipc.h"

namespace ipc {
	const std::string IPC_Errorstring(IPC_Error e) {
		const std::map<IPC_Error, std::string> IPCEnumStrings {
			{IPC_Error::INIT_SUCCESS, std::string( "IPCError: Initialization Success")},
			{IPC_Error::INIT_FAILURE, std::string( "IPC_Error: Initialization Failure")},
			{IPC_Error::READ_SUCCESS, std::string("IPC_Error: Read Success")},
			{IPC_Error::READ_OOB, std::string( "IPC Error: Read Out of Bounds")},
			{IPC_Error::READ_FAILURE, std::string("IPC Error: Read Failure")},
			{IPC_Error::WRITE_SUCCESS, std::string("IPC Error: Write Success")},
			{IPC_Error::WRITE_OOB, std::string("IPC Error: Write Out of Bounds")},
			{IPC_Error::WRITE_FAILURE, std::string( "IPC Error: Write Failure")}
		};
		
		auto ans = IPCEnumStrings.find(e);
		if (ans == IPCEnumStrings.end())
			return std::string("IPC Error: Impossible Error");
		else
			return ans->second;
	};

	int SharedMemory::read_int(int offset) {
		int* ip = (int*)memory_ptr(offset);

		spdlog::trace("Read, offset {}, pointer {}, value int {}\n", offset, fmt::ptr(ip), *ip);
		return *ip;
	}

	void SharedMemory::write(int offset, int val) {
		spdlog::trace("Write int {} at offset {}\n", val, offset);
		int* ip = (int*)memory_ptr(offset);
		ip[0] = val;
	}

}
