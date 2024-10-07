#ifndef _IPC_WINDOWS_H_
#define _IPC_WINDOWS_H_

#include <string>
#include <windows.h>

namespace ipc {

#define MAX_SEM_COUNT 1


	struct SharedMemoryVariables {
		std::string memname;
		HANDLE memHandle;
		int memsize;
		PVOID membuf;

	};


	struct SharedSemaphoreVariables {
		std::string name;
		HANDLE pSem;
	};

}

#endif // _IPC_WINDOWS_H