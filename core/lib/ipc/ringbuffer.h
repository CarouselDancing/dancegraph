#pragma once

#include <memory.h>

#include <vector>
#include <atomic>
#include <array>

#include <spdlog/spdlog.h>

#include <ipc/ipc.h>


// Single Client Ringbuffer implementation

// The basic idea is a single-reader, single-writer non-locking (or minimally locking) circular buffer with arbitrary write lengths

// Underlying this is the assumption that the reader only wants the very latest information and that old information can be safely dropped;
// this means that it may be unsuitable for information channels of the 'dump a large block of info at the start and then update it with deltas' ilk,
// such as keyframed video formats, etc

// The reader should be happy to spin (or find something else to do) in the absence of material to read
// If the writer overtakes the reader, then the write just skips ahead and writes at the end of the material being read;
// this will cause skipping of reads if the reader is too slow

// Because there must ALWAYS be a space to write to, there should be a limit to the maximum write,
// and there must be always be at least that maximum-write-size data available for writing at all times.

// Multiple readers might be supportable with this idea (but multiple readers may fragment the incoming data)

// There are three main communication channels here:

// 1:  The main memory that is written to by the writer, and read by the reader; each block has a small header detailing it's status and length; when blocks are written, a 'tail' value is appended which is essentially a 'null' header, signifying no more data; the 'tail' is overwritten by the header at each new write.

// | header_status 1 | data_size 1 | ... ... ... data 1 ... ... ... | header_status 2 | data_size 2 | ... ... data 2 ... ... | tail |

// 2:  A boolean semaphore set and unset by the writer whenever it is considering writing to a new block; this is to signify to the reader that it shouldn't start reading a new block while the semaphore is set, to avoid race conditions

// 3:  A small piece of memory (2 ints) that delineates the start and size of the memory being read, so that the writer doesn't overwrite it; it's written (atomically) by the reader, and read by the writer. This is currently handled by the same semaphore as case 3) below, since the write to this memory should never happen when the writer is changing write pages.
// One possible edge-case issue is where the reader process dies entirely after setting the semaphore, thus blocking the writer

// 4. A small piece of memory (2 ints) denoting the last memory write, written to the buffer by the *writer*; this is so that the reader can find a proper place to start reading when it initially connects. Writes to these occur under the cover of the semaphore

// There should perhaps be a 'number of unread items' indicator for latency management

namespace ipc {

#define BLOCK_HEADER_SIZE 2
#define READ_PAGE_SIZE 2
#define WRITE_PAGE_SIZE 2

#define RW_PAGE_SIZE (READ_PAGE_SIZE + WRITE_PAGE_SIZE)

	enum BufferWriteStatus {
		BW_BUFFER_END = -1, // end of buffer; also the last byte written
		BW_WRITING = -2, // buffer is currently being written to; do not read
		BW_READY = -3 // ready to read
	};


	enum class RBError {
		SUCCESS = 0,
			WRITE_FAILURE,
			WRITE_SIZE_ERROR, // bigger than max_buffer_write or less than 0
			READ_BUFFER_END, // Hit the end of the buffer
			READ_BUFFER_BUSY, // Current read-page is being written
			READ_BUFFER_OOB, // Reader is attempting an out of bounds read
			READ_BUFFER_NO_INIT, // Can't find an initial write page
			READ_FAILURE	
			};

	struct ReadPage{
		int offset;
		int size;
	};

	const std::string RB_ErrorString(RBError);

	// Payload has to be the last entry for the variable length struct hack
	struct buffer_entry_ {
		int write_status;
		int write_size;
		char payload[0];
	};

	using buffer_entry = struct buffer_entry_;

	
	class DNRingbuffer { // Base class for both reader and writer

	protected:
		std::string buffer_name; // Name used for both shared memory and semaphore

		unsigned int total_buffer_bytes; // Total memory allocated including the two read->write ints, measured in bytes
		unsigned int buffer_size; // Size of the buffer measured in units of sizeof(struct buffer_entry<T>) bytes
		
		unsigned int num_entries;

		size_t buffer_entry_size; // Size of the buffer entries
		size_t buffer_payload_size; // Size of the actual payload of the buffer entries

		// (maybe) The top 8 bytes are two 32-bit integers that are reserved for writing *by the reader*, and should
		// be pointed to by the readpage pointer
		SharedMemory cBuf;

		// Semaphore that blocks the read process from switching to a different block until the writer has finished writing
		//'new block' housekeeping
		DNSemaphore sem;

		// points to a minimal (int offset, int size) pair that's atomically updated by the reader, and denotes the page being read
		// Updating this will likely have very very minimal blocking of the write process
		// The size includes the header bytes

		int read_page;

		// points to a minimal (int offset, int size) pair that's atomically updated by the writer, and denotes the last
		// page written to;

		int write_page;

		inline unsigned int buffer_add(unsigned int offset, unsigned int n) {
			// Could use an operator overload but meh
			return (offset + n) % num_entries;
		}

		inline bool check_bound_wrap(unsigned int offset, unsigned int itemcount) {
			return ((offset + itemcount) >= num_entries);
		}

		// Converts read_offset or write_offset to the offset that the SharedMemory class wants
		inline int mem_offset(int offset) {
			return buffer_entry_size * offset;
		}

		// Sets the write status for the buffer entry at a given offset
		// Only the writer will use this
		inline RBError set_status(int offset, int bufstatus) {
			this->cBuf.write(offset * buffer_entry_size + offsetof(buffer_entry, write_status), bufstatus);
			return RBError::SUCCESS;

		}

		// Sets the write size for the buffer entry at a given offset
		// Only the writer will typically use this
		inline RBError set_size(int offset, int size) {
			this->cBuf.write(offset * buffer_entry_size + offsetof(buffer_entry, write_size), size);
			return RBError::SUCCESS;
		}

		// Gets the write status
		inline int get_status(int offset) {
			return this->cBuf.read_int(offset * buffer_entry_size + offsetof(buffer_entry, write_status));
		}

		inline int get_size(int offset) {
			spdlog::info(  "Offset to read_int: {}*{} + {}", offset, buffer_entry_size, offsetof(buffer_entry, write_size));
			return this->cBuf.read_int(offset * buffer_entry_size + offsetof(buffer_entry, write_size));
		}


	
	public:
		// Size of the biggest allowable memory write
		// To avoid blocking, a contiguous block with this amount of memory needs to be writeable at all times (i.e. nothing reading it)
		// Public because the process doing the reading needs to know how much memory to allocate
		unsigned int max_buffer_write;
	
		// Constructor with common code for the reader and writer
		DNRingbuffer(std::string name, int entry_size, int num_entries, bool isRead);
	
	};

// Ringbuffer Reader class

class DNRingbufferReader : public DNRingbuffer {

		int read_offset; // Points to the start of 2-int header of the currently read data
		int read_size; // Size of the data currently being read

		bool initialized_write; // This reader has found an apparently valid write page

		// Sets the read page; should only be called when the reader has set the semaphore to block
		// the writer from reading

		// Another way of doing this is std::atomic, but it will still block the writer either way
		void inline set_read_page(int, int);
		bool inline get_write_page();
		struct ReadPage wpage; // It's the last page write

		RBError read_entry(void *);

	public:
		DNRingbufferReader(std::string name, int entry_size, int num_entries);
	
		// memto should be allocated at least as big as max_buffer_write
		int get_last_read_size(); 

		// Reads into a pre-allocated memory chunk of size at least max_buffer_write * sizeof(T)
		// The value stored in read_size (if it's there) is the size of bytes being read
		RBError read(void * memto);

	};

// Ringbuffer Writer class
	class DNRingbufferWriter : public DNRingbuffer {
	// Struct to copy the read page into
		struct ReadPage rpage;

	private:
	// Depending on how this is implemented, we may signal whether the read was successful
		bool inline get_read_page();
		int write_offset; // The offset of the start of the memory block, as an increment to a T* pointer
		void inline set_write_page(int, int);

	public:
		DNRingbufferWriter(std::string name, int entry_size, int num_entries);

		RBError write(void*, int size = -1);

	};
}
