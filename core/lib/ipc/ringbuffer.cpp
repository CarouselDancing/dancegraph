#include <iostream>
#include <map>


#include "ringbuffer.h"

namespace ipc {

	const std::string RB_ErrorString(RBError e) {
		const std::map<RBError, std::string> RBEnumStrings {
			{RBError::SUCCESS,std::string("RBError: Success")},
			{RBError::WRITE_FAILURE,std::string("RBError: Write Failure")},
			{RBError::WRITE_SIZE_ERROR, std::string("RBError: Write Size Error")},
			{RBError::READ_BUFFER_END, std::string("RBError: Read Buffer End")},
			{RBError::READ_BUFFER_BUSY,std::string("RBError: Read Buffer Busy")},
			{RBError::READ_BUFFER_OOB, std::string("RBError: Attempted Read out of buffer bounds")},
			{RBError::READ_BUFFER_NO_INIT, std::string("RBError: No valid initial write location")},
			{RBError::READ_FAILURE, std::string("RBError: Read Failure")}
		};

		auto ans = RBEnumStrings.find(e);
		if (ans == RBEnumStrings.end())
			return std::string("RB Error: Impossible Error");
		else
			return ans->second;
	}


	DNRingbuffer::DNRingbuffer(std::string name, int entry_size, int num_entries, bool createMem)
		: buffer_name(name),
		sem(name.append("_sem")),
		cBuf(name, (entry_size + sizeof(buffer_entry)) * num_entries + sizeof(int) * (RW_PAGE_SIZE), createMem),
		buffer_entry_size(sizeof(buffer_entry) + entry_size),
		buffer_payload_size(entry_size),
		num_entries(num_entries)	
	{

		buffer_size = num_entries * buffer_entry_size;

		spdlog::info(  "Entry size={}", entry_size);
		spdlog::info(  "sizeof(buffer_entry) ={}", sizeof(buffer_entry));
		spdlog::info(  "num_entries={}", num_entries);

		total_buffer_bytes = RW_PAGE_SIZE * sizeof(int) + buffer_size * buffer_entry_size;

		max_buffer_write = (num_entries - 1) * buffer_entry_size;

		read_page = buffer_entry_size * num_entries;
		write_page = read_page + READ_PAGE_SIZE * sizeof(int);
		spdlog::info("read_page offset : {}, write_page offset: {}", read_page, write_page);
		spdlog::info("buffer_size: {}, Buf entry size: {}", buffer_size, buffer_entry_size);

	}


	DNRingbufferReader::DNRingbufferReader(std::string name, int entrysize, int num_entries) : DNRingbuffer(name, entrysize, num_entries, true) {
		set_read_page(-1, -1); // Setting the first read value to -1 signifies nothing being read
		read_offset = 0;
		initialized_write = false;


		spdlog::info(  "Ringbuffer Reader constructed");
	}


	DNRingbufferWriter::DNRingbufferWriter(std::string name, int entrysize, int num_entries) : DNRingbuffer(name, entrysize, num_entries, true) {

		write_offset = 0;
		this->set_status(write_offset, BW_BUFFER_END);
		this->sem.set();
	}


	void inline DNRingbufferReader::set_read_page(int offset, int size) {
		spdlog::info(  "Reader: set_read_page {}, {}", offset, size);

		this->cBuf.write(this->read_page, offset);
		this->cBuf.write(this->read_page + sizeof(int), size);
		

	}

	void inline DNRingbufferWriter::set_write_page(int offset, int size) {
		spdlog::info(  "Writer: set_write_page {}, {}", offset, size);

		this->cBuf.write(this->write_page, offset);
		this->cBuf.write(this->write_page + sizeof(int), buffer_entry_size);

	}


	bool inline DNRingbufferWriter::get_read_page() {
		// Copy into the struct

		rpage.offset = this->cBuf.read_int(this->read_page);
		rpage.size = this->cBuf.read_int(this->read_page + sizeof(int));
		return true;
	}


	bool inline DNRingbufferReader::get_write_page() {
		// Copy into the struct

		wpage.offset = this->cBuf.read_int(this->write_page);
		wpage.size = this->cBuf.read_int(this->write_page + sizeof(int));

		return true;
	}


	RBError DNRingbufferWriter::write(void* from, int write_size) {

		int write_payload_size;
		if (write_size < 0) 
			write_payload_size = buffer_payload_size;
		else
			write_payload_size = write_size;

		int write_entry_size = write_payload_size + sizeof(buffer_entry);

		if (write_entry_size > DNRingbuffer::max_buffer_write) {
			return RBError::WRITE_SIZE_ERROR;
		}

		// Checks that the semaphore isn't up (i.e. being written to by the read process)
		// (sigh) blocks, if so. The other obvious way is to use atomic writes by the read process,
		// which may also block the write process

		// TODO: Try it both ways and test the performance implications

		spdlog::info(  "Blocking semaphore wait");
		this->sem.wait(); // Block if Reader is writing to the read page indicator

		int read_offset = this->cBuf.read_int(this->read_page);
		int read_size = this->cBuf.read_int(this->read_page + sizeof(int));

		if (read_offset == write_offset) {
			spdlog::info(  "Writer: Skipping overwrite ({}) of the read_offset at {}", write_offset, read_offset);
		}

		// If the writer is overtaking the reader, skip over it; 
		// Also uninitialized read_size is less than 0, so we know to ignore that 

		int buffer_status = this->get_status(write_offset);
		if ((buffer_status != BW_BUFFER_END) && (buffer_status != BW_READY)) {
			spdlog::info(  "Warning: Unusual ringbuffer overwrite value: {}", buffer_status);
		}

		int old_write_offset = write_offset;

		this->set_status(write_offset, BW_WRITING);

		set_write_page(write_offset, 1);
		this->sem.set();

		//spdlog::info(  "Writer: Writing size {} to rbuf offset {}", write_entry_size, write_offset);

		// The reader is now blocked by the BW_WRITING value

		// Write the size 
		//this->set_size(write_offset, buffer_entry_size);
		this->set_size(write_offset, write_entry_size);

		if (this->cBuf.copy_to(BLOCK_HEADER_SIZE * sizeof(int) + write_offset * buffer_entry_size,
			(void*)from,
			write_payload_size) != IPC_Error::WRITE_SUCCESS) {

			// Cleanup and quit
			this->set_status(old_write_offset, BW_BUFFER_END);

			return RBError::WRITE_FAILURE;
		}

		this->set_status(old_write_offset, BW_READY);

		this->set_status(this->buffer_add(write_offset, 1), BW_BUFFER_END);

		write_offset = this->buffer_add(write_offset, 1);

		return RBError::SUCCESS;
	}


	RBError DNRingbufferReader::read(void* memto) {

		// Busy and end of buffer signals have the same behaviour here, but maybe the
		// calling function has different ideas as to what to do next, so the signals are different
		// (e.g. if we're hittting the buffer end a lot, maybe there could be more time between read calls)
		//spdlog::info(  "Ringbuffer read from offset {}", read_offset);
		if (!initialized_write) {
			// Wait until the writer has given the go ahead for the first read
			// This maybe should be a non-blocking read with an error message
			this->sem.wait();

			// Look up the write page to get the first write
			get_write_page();
			read_offset = wpage.offset;
			this->sem.set();

			if (read_offset < 0) {
				spdlog::info(  "Reader: Initiate Read Fail: {} < {}", read_offset, 0);
				return RBError::READ_BUFFER_NO_INIT;

			}
			else if (read_offset > buffer_entry_size * this->buffer_size) {
				spdlog::info(  "Reader: Initiate Read Fail: {} > {} * {}", read_offset, buffer_entry_size, this->buffer_size);
				return RBError::READ_BUFFER_NO_INIT;
			}


			if (wpage.size < 0 || wpage.size > this->max_buffer_write) {
				spdlog::info(  "Reader: Initiate Read Fail");
				return RBError::READ_BUFFER_NO_INIT;
			}

			initialized_write = true;
			spdlog::info(  "Reader: Initiate read at offset {}", read_offset);
		}


		//int header_status = this->cBuf.read_int(this->mem_offset(read_offset));
		int header_status = this->get_status(read_offset);

		if (header_status == BW_BUFFER_END)
			return RBError::READ_BUFFER_END;

		if (header_status == BW_WRITING)
			return RBError::READ_BUFFER_BUSY;

		if (header_status != BW_READY)
			return RBError::READ_FAILURE;

		read_size = this->get_size(read_offset);
		spdlog::info("Setting new read size: {}", read_size);

		int new_read_offset = this->buffer_add(read_offset, 1);

		//spdlog::info(  "Old Read Offset: {}, size: {}, new offset: {}", read_offset, read_size, new_read_offset);

		if (read_size < 0 || read_size > this->max_buffer_write) {
			return RBError::READ_BUFFER_OOB;
		}

		// Set the semaphore (blocks the write page briefly)
		// TODO: possibly using atomic write will be quicker; test and see
		// If this process dies while the semaphore is up, it's bad.
		this->sem.wait();
		this->set_read_page(read_offset, buffer_entry_size);
		this->sem.set();
		auto data_offset = read_offset * buffer_entry_size + offsetof(buffer_entry, payload);

		if (this->cBuf.copy_from(data_offset, memto, read_size) == IPC_Error::READ_FAILURE) {
			return RBError::READ_FAILURE;
		}

		read_offset = new_read_offset;
		this->sem.wait();
		// Signal to the writer that we're not reading anything anymore
		this->set_read_page(-1, -1);
		this->sem.set();
		
		return RBError::SUCCESS;
	}


	int DNRingbufferReader::get_last_read_size() {
		return read_size;
	}

}
