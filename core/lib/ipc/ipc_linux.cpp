#ifdef __linux__

#include <string>
#include <iostream>

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>

#include <spdlog/spdlog.h>

#include "ipc.h"
#include "ipc_linux.h"
#include "common.h"

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

		svars.memsize = size;

		// Non-exclusive writes
		if (create) {
			svars.fd = shm_open(name.c_str(), O_CREAT|O_RDWR, 0600);
		}
		else {
			svars.fd = shm_open(name.c_str(), O_RDWR, 0600);
		}
		if (svars.fd < 0) {
			perror("shm_open()");
			return IPC_Error::INIT_FAILURE;
		}
	
		ftruncate(svars.fd, svars.memsize);
	
		svars.membuffer = mmap(0, svars.memsize, PROT_WRITE| PROT_READ, MAP_SHARED, svars.fd, 0);
		if (svars.membuffer == MAP_FAILED) {
			spdlog::info(  "Write map failure: {}", strerror(errno));
			// We can extract errno to find out why the map fails
			return IPC_Error::INIT_FAILURE;
		}
		close(svars.fd);
		return IPC_Error::INIT_SUCCESS;
	}

	IPC_Error SharedMemory::copy_from(int offset, void * memto, int n) {

		if ((n < 0) || (offset + n > svars.memsize)) {
			return IPC_Error::READ_OOB;
		}

		// These castings as memory offsets don't strike me as good style
		void * memwritefrom = (char *) svars.membuffer + offset;
		void * memval = memcpy(memto, memwritefrom, n);

		int * ct = (int *)memto;
		spdlog::info(  "IPC COPYFROM {} bytes from offset {} to {}", n, offset, memto);
	
		return IPC_Error::READ_SUCCESS;
	}

	IPC_Error SharedMemory::copy_to(int offset, void * memfrom, int n) {
		// Lets not scribble all over memory we don't have access to
		if ((n < 0) || (offset + n > svars.memsize)) {
			return IPC_Error::WRITE_OOB;
		}
		
		void * memwriteto = (char *) svars.membuffer + offset;
		void * memval = memcpy(memwriteto, memfrom, n);

		int * ct = (int *)memfrom;
		spdlog::info(  "IPC COPYTO {} bytes to offset {} ( {}, {}, {})", n, offset, ct[0], ct[1], ct[2]);
	
		return IPC_Error::WRITE_SUCCESS;	
	}

	void SharedMemory::uninitialize_memory() {
		spdlog::info(  "Unmapping the shared memory");
		munmap(svars.membuffer, svars.memsize);

		spdlog::info(  "Unlinking the shared memory");
		shm_unlink(svars.memname.c_str());

	}

	void * SharedMemory::memory_ptr(int offset) {
		return (void *) ((char *)svars.membuffer + offset * sizeof(char));
	}


	SharedMemory::~SharedMemory() {
		spdlog::info(  "Calling the SharedMemory destructor");
		uninitialize_memory();
	}

	DNSemaphore::DNSemaphore(std::string name) {
		svars.name = name;

		spdlog::info(  "Opening semaphore {}", svars.name);

		svars.psem = sem_open(svars.name.c_str(), O_CREAT, 0606, 1);
	
		if (svars.psem == SEM_FAILED) {
			// Should probably throw an exception here
			spdlog::info(  "Error: sem_open: {}", strerror(errno));
			spdlog::info(  "sem_open arguments: {}, {}, " {}  "", svars.name.c_str(), O_CREAT, 0606);
		}
	}

	DNSemaphore::~DNSemaphore() {
		if (svars.psem != SEM_FAILED) {
			sem_close(svars.psem);
			sem_unlink(svars.name.c_str());
		}
	}

	bool DNSemaphore::read() {
		int v;
		int err = sem_getvalue(svars.psem, &v);
		if (err < 0) {
			spdlog::info(  "Error: sem_getvalue: {}", strerror(errno));
		}
		return (v != 0);
	}

	void DNSemaphore::set() {

		// Posix semaphore is a counter rather than a boolean hence this faff

		int v;
		int err = sem_getvalue(svars.psem, &v);
		if (err < 0) {
			spdlog::info(  "Error: sem_getvalue: {}", strerror(errno));
		}

		if (v == 0) {
			sem_post(svars.psem);
		}
	}

	void DNSemaphore::unset() {
		int err = sem_trywait(svars.psem); // non-blocking semaphore decrement
	}

	void DNSemaphore::wait() {
		int sval;
		sem_getvalue(svars.psem, &sval);
		spdlog::info(  "Waiting on semaphore with val: {}", sval);
		int err = sem_wait(svars.psem); // waits until the semaphore is zero

	}

}
#endif // __linux__



