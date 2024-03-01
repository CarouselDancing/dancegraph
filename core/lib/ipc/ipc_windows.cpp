#ifdef _WIN32

#include <windows.h>

#include "ipc.h"
#include "ipc_windows.h"

#include <sstream>
#include <iomanip>

#include <spdlog/spdlog.h>

namespace ipc {

	SharedMemory::SharedMemory(std::string name, int size, bool doCreate) {
		svars.memname = name;
		svars.memsize = size;

		spdlog::info(  "Opening shared memory {} with size {}", svars.memname, svars.memsize);

		IPC_Error rval = initialize_memory(svars.memname, svars.memsize, doCreate);
		if (rval != IPC_Error::INIT_SUCCESS)
			std::cerr << "Shmem memory init failure\n";
	}

	IPC_Error SharedMemory::initialize_memory(std::string name, int size, bool create) {
		if (create) {
			svars.memHandle = CreateFileMapping(INVALID_HANDLE_VALUE,
												nullptr,
												PAGE_READWRITE,
												0,
												svars.memsize,
												svars.memname.c_str());

			if (svars.memHandle == nullptr) {
				spdlog::info(  "Memory Initialization failure:{}", GetLastError());
				return IPC_Error::INIT_FAILURE;
			}
		}

		svars.membuf = (PVOID)MapViewOfFile(svars.memHandle,
											FILE_MAP_ALL_ACCESS,
											0,
											0,
											svars.memsize);

		if (svars.membuf == nullptr) {
			spdlog::info(  "Memory Initialization file mapping error:{}", GetLastError());
			CloseHandle(svars.memHandle);
			return IPC_Error::INIT_FAILURE;
		}
		spdlog::info(  "Memory Initialization at {} with size {}", svars.membuf, svars.memsize);

		return IPC_Error::INIT_SUCCESS;
	}

	IPC_Error SharedMemory::copy_from(int offset, void* memto, int n) {
		char * readptr = (char *) svars.membuf + offset;
		//spdlog::info(  "Copy {} bytes from offset {}", n * sizeof(TCHAR), offset);
		CopyMemory(memto, (PVOID) readptr, n * sizeof(TCHAR));
		
		return IPC_Error::READ_SUCCESS;
	}

	IPC_Error SharedMemory::copy_to(int offset, void* memfrom, int n) {
		char* writeptr = (char*)svars.membuf + offset;
		//spdlog::info(  "Copy {} bytes into offset {}", n * sizeof(TCHAR), offset);
		CopyMemory((PVOID)writeptr, memfrom, n * sizeof(TCHAR));
	

		return IPC_Error::WRITE_SUCCESS;
	}

	void SharedMemory::uninitialize_memory() {
		CloseHandle(svars.memHandle);
	}

	void* SharedMemory::memory_ptr(int offset) {
		void * mp = ((char*)svars.membuf + offset * sizeof(char));
		return mp;
	}

	SharedMemory::~SharedMemory() {
		uninitialize_memory();
	}

	DNSemaphore::DNSemaphore(std::string name) {
		// This semaphore isn't inherited by child processes, as per the first argument to CreateSemaphore

		svars.name = name;
		spdlog::info(  "Opening Windows semaphore {}", svars.name);
		svars.pSem = CreateSemaphore(NULL, MAX_SEM_COUNT, MAX_SEM_COUNT, svars.name.c_str());

		if (svars.pSem == nullptr) {
			spdlog::info(  "CreateSemaphore Error");
		}
	}

	DNSemaphore::~DNSemaphore() {
		CloseHandle(svars.pSem);
	}

	bool DNSemaphore::read() {
		spdlog::info(  "Windows semaphore reads unsupported at this time");
		return false;
	}

	void DNSemaphore::set() {
		bool rv = ReleaseSemaphore(svars.pSem, 1, nullptr);
		if (rv == 0) {
			spdlog::info(  "Semaphore release fail, value: {}", rv);
		}
	}

	void DNSemaphore::unset() {
		DWORD dw = WaitForSingleObject(svars.pSem, 0);
	}

	void DNSemaphore::wait() {
		DWORD dw = WaitForSingleObject(svars.pSem, INFINITE);
	}

}
#endif // _WIN32
