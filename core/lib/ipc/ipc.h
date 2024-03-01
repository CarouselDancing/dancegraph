#ifndef _IPC_H_
#define _IPC_H_

#include <string>
#include <iostream>

#ifdef __linux__
#include "ipc_linux.h"

#elif _WIN32
#include "ipc_windows.h"
#else
// Else any other OS or fallback IPC options

#endif

namespace ipc {

// Shared memory IPC class; uses integer offsets and C-style void pointers. 
// TODO: Perhaps read and write specific known data structures (tracking data, haptics, etc) once we know what they are

    enum class IPC_Error {
		INIT_SUCCESS = 0,
			INIT_FAILURE,
			READ_SUCCESS,
			READ_OOB, // Out of Bounds; read/write was attempted outside the buffer
			READ_FAILURE,
			WRITE_SUCCESS,
			WRITE_OOB,
			WRITE_FAILURE
			};

// Turns the error message into a printable string
	const std::string IPC_Errorstring(IPC_Error);


	// TODO: Consider having the classes holding unique_ptrs to the SharedMemoryVariables and SharedSemaphoreVariables structs,
	// rather than holding the actual structs themselves
	// This should decrease build time at the cost of an indirection (which is a standard use-case for this pattern as per the
	// C++ guidelines on the pimpl idiom)

	// Since we're using the pattern for OS-independent code, not compile time reduction, and build times are not an issue (yet),
	// I'll leave as-is for now, but the option to switch is there, if we need it.
	
	class SharedMemory {
		
	private:

		// Hides the private variables behind a struct so that the class definition is platform independent
		struct SharedMemoryVariables svars;

		// Called by the constructor.
		// Init memory for writing
		IPC_Error initialize_memory(std::string, int, bool);
		// The second argument is the size of the memory

	
	public:

		// First argument is the name
		// The second argument denotes whether the memory should be created if necessary
		SharedMemory(std::string, int, bool);
	
		// Write n bytes from shared memory to a void pointer in unshared memory
		IPC_Error copy_from(int offset, void * copy_to, int n);
	
		// Write n bytes from a void pointer to unshared memory to shared memory
		IPC_Error copy_to(int offset, void * memfrom, int n);

		// offset is an integer offset in shared memory to read from/write to
		// (i.e. the shared mem starts at (char *) shmem_base_ptr + offset)
	
		// Shut 'er down
		void uninitialize_memory();

		// Destructor, for shutting down the memory
		~SharedMemory();
	
		// I don't like exposing a pointer to the buffer memory, but it should make the writes from the reader more
		// efficient (thereby blocking the write process less)
		void * memory_ptr(int);
	
		// Reading and writing single ints to a given offset is a very common use case so we have it here
		// We need to read and write ints no matter what data is being sent
		// Overload with other types if necessary

		int read_int(int offset);
		void write(int offset, int val);


/* template<class T> */
		/* 	inline void write(int offset, T val) { */
		/* 	T *ip = (T *) memory_ptr(offset); */
		/* 	ip[0] = val; */
		/* } */
	
	};

// Simple On/Off Semaphore encapsulation class
// Both Windows and Posix support named semaphores

// Note that the C++20' standard libs has semaphore support in a C++ idiomatic way, so maybe we can switch to that,
// compiler support permitting
	
	class DNSemaphore {
	private:
		struct SharedSemaphoreVariables svars;
	public:
		DNSemaphore(std::string name);

		bool read(); // Return the value of the semaphore. Windows doesn't have a non-intrusive version of this function so we should perhaps not have it
		void set(); // Set the value to 1
		void unset(); // Set the value to 0; does not block
		void wait(); // Set the value to 0, blocking the calling thread until the semaphore IS zero

		~DNSemaphore();
	
	};

}
#endif // _IPC_H_
