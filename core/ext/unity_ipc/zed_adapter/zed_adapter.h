#pragma once

#include <memory>

#include <modules/zed/zed_common.h>
#include <ipc/ringbuffer.h>

namespace dll {
	// Class inside the DanceNet plugin for receiving ZED signals via IPC and sending them across the Unity C# boundary
	// Essentially, this is a wrapper round the IPC reader that joins the data being read via IPC to the Unity Data structures in C#

	
	void zed_initialize();
	void zed_shutdown();
	class ZedAdapter {

	public:

		bool read_ipc();
		int get_numbodies();
		//long int get_elapsed();
		//double get_elapsed();

		unsigned long long get_elapsed();
		int get_dataSize();

		void get_roottransform(float[]);
		void get_bodyIDs(int[]);
		void get_bodyData(float[]);

		unsigned long long get_timestamp(bool meta = false); // Time since epoch in milliseconds
		
		int get_packetID();
		int get_userID();
		
		zed::Zed4SignalType sigType;

		~ZedAdapter();
		ZedAdapter(std::string name, int size, int numEntries);

	protected:
		ipc::DNRingbufferReader ipcReader;
		zed::BufferProperties bProps;
	};

}