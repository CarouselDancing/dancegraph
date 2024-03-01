#ifndef _IPC_LINUX_H_
#define _IPC_LINUX_H_

// Posix Semaphores. 
#include <semaphore.h> 

#include <string>

namespace ipc {

	struct SharedMemoryVariables {
		std::string memname; // Name of the shared memory; typically found in /dev/shm/<memname>
		int fd; // File descriptor
		void * membuffer;
		int memsize;	
	};

	struct SharedSemaphoreVariables {
		sem_t * psem; // the underlying posix semaphore
		std::string name; // Name of the shared memory; typically found in /dev/shm/sem.<memname>
	};

   }

#endif // _IPC_LINUX_H_
