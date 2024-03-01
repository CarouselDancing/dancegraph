

#include <iostream>
#include <string>
#include <iomanip>

#include <thread>
#include <chrono>

#include <fstream>
#include <sstream>

#ifdef __linux__
#include <unistd.h>

#elif _WIN32
#include <windows.h>
#include <processthreadsapi.h>
#include <system_error>
#endif 

#include <argparse.hpp>

#include <stdlib.h> // Sigh. Malloc

#include <assert.h>



#include <spdlog/sinks/basic_file_sink.h>
//#include <core/common/common.h>
#include <ipc/ringbuffer.h>

#include <chrono>

// Ringbuffer Timing Test

// Sends the payload at a given interval and reads it as fast as possible, and prints times

using namespace ipc;


struct Payload {
	std::chrono::system_clock::time_point acquisitionTime;
	

	void fill() {
		acquisitionTime = std::chrono::system_clock::now();
	}

};


std::string stripname(const char * file) {
#ifdef _WIN32
	return std::string((strrchr(file, '\\') ? strrchr(file, '\\') + 1 : file));
#else
	return std::string(strrchr(file, '/') ? strrchr(file, '/') + 1 : file);
#endif 
}

void gotmem(const char * prefix, int * rmem, int count) {
	std::stringstream ss = std::stringstream();
	ss << prefix << " ";
	for (int i = 0; i < count; i++) {
		ss << rmem[i] << " ";
	}
	ss << "\n";

	spdlog::info(ss.str());
}



void test_writer(std::string name, int entrycount, int numwrites, int writepause, int padding) {
	// entrycount == number of entries in the buffer
	// writesize == size of buffer
	// numwrites == number of times we're going to write something
	// writepause == length of time between writes

	//LOG << "Creating writer with entry size: " << entrycount << ", and entrysize: " << sizeof(Payload) << "\n";

	spdlog::info("Creating writer with entry size : {}, and entry count : {}\n", padding + sizeof(Payload),entrycount);
	DNRingbufferWriter writer = DNRingbufferWriter(name, sizeof(Payload) + padding, entrycount);

	Payload payload;

	RBError rerr;
	
	for (int i = 0; i < numwrites; i++) {
		payload.fill();

		rerr = writer.write((void *)&payload, sizeof(Payload));		
		if (rerr != RBError::SUCCESS) {
			std::cout << "Error writing to shared memory\n";
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(writepause));

	}
	
}

void test_reader(std::string name, int entrycount, int numreads, int readpause, int padding) {

	//char* data = new char[sizeof(Payload)];

	char* data = new char[sizeof(Payload) + padding];

	Payload* readp = (Payload *) data;

	DNRingbufferReader reader = DNRingbufferReader(name, sizeof(Payload) + padding, entrycount);

	//spdlog::info("Creating reader with entry size : {}, and entry count : {}\n", sizeof(Payload), entrycount);
	int readcounter = 0;
	
	printf("Creating reader with entry size : %d, and entry count : %d\n", (int)sizeof(Payload), entrycount);

	//spdlog::info("Packet has size {}\n", sizeof(Payload));
	RBError rerr;
	int deadcounter = 0;

	std::chrono::time_point initial_time = std::chrono::system_clock::now();
	int timelimit = readpause * (10 + numreads) + 1000; // Time in milliseconds until we give up
	int elapsed = 0;

	while ((readcounter < numreads) && (elapsed < timelimit)) {
		
		rerr = reader.read(data);
		
		if (rerr == RBError::SUCCESS) {
			std::chrono::time_point currentTime = std::chrono::system_clock::now();
			long long duration;			
			try {
				duration = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - readp->acquisitionTime).count();				
			}
			catch (std::exception e) {
				std::cout << "Duration fail: " << e.what() << "\n";

			}
							
			//std::cout << "Duration: " << duration << "\n";
			int readbytes = reader.get_last_read_size();
			
			std::cout << "Read " << readcounter << " (" << readbytes << "): Time difference: " << duration << "\n";
			
			readcounter += 1;
			//elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - initial_time).count();
			
		}
	}

	delete[] data;
}




int main(int argc, char** argv)
{


	argparse::ArgumentParser program("buffertiming");

	program.add_argument("--padding").help("padding value")
		.nargs(1)
		.scan<'i', int>()
		.default_value(0);

	program.add_argument("numwrites").help("numwrites")
		.scan<'i', int>();


	program.add_argument("interval").help("interval")
		.scan<'i', int>();

	program.add_argument("--TEST_WRITER")
		.default_value(false)
		.implicit_value(true);

	program.add_argument("--TEST_READER")
		.default_value(false)
		.implicit_value(true);

	try {
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error& err) {
		std::cout << "Invalid Arguments\n";
		std::exit(1);
	}


	int numwrites = program.get<int>("numwrites");
	int write_interval = program.get<int>("interval");

	int padbytes = program.get<int>("--padding");

	bool isReader = program.get<bool>("--TEST_READER");
	bool isWriter = program.get<bool>("--TEST_WRITER");


	
	std::string testname = "IPC_Latency_Test";
	int buffersize = 20;

	if ((numwrites <= 0) || (write_interval <= 0) && (isReader && isWriter)) {
		std::cout << "Args: " << stripname(argv[0]) << "--padding <padding> numwrites write_interval(in ms)\n";
		return 0;
	}

#ifdef __linux__
	if (fork() == 0) {
		LOG_initialize("Log_Buffertest_write.txt", LogType::LOG_SOURCETIME);
		writeTestToLog(testname, buffersize, writesize, numwrites, writepause, readpause);
		test_writer(testname, buffersize, writesize, numwrites, writepause);
	}
	else {
		LOG_initialize("Log_Buffertest_read.txt", LogType::LOG_SOURCETIME);
		writeTestToLog(testname, buffersize, writesize, numwrites, writepause, readpause);
		test_reader(testname, buffersize, writesize, readpause);
	}

#elif _WIN32
	// Windows don't fork so we'll just invoke this program with separate arguments for a similar effect
	

	if (isWriter) {
		std::cout << "Running writer\n";
		spdlog::set_default_logger(spdlog::basic_logger_mt("basic_logger", "Log_Buffertest_write.txt"));
		test_writer(testname, buffersize, numwrites, write_interval, padbytes);

	}
	else if (isReader) {
		std::cout << "Running Reader\n";
		spdlog::set_default_logger(spdlog::basic_logger_mt("basic_logger", "Log_Buffertest_read.txt"));
		test_reader(testname, buffersize, numwrites, write_interval, padbytes);
	}
	else {
		
		spdlog::set_default_logger(spdlog::basic_logger_mt("basic_logger", "Log_Buffertest_control.txt"));		
		std::stringstream ssr = std::stringstream();
		std::stringstream ssw = std::stringstream();


		for (int i = 0; i < argc; i++) {
			ssr << argv[i] << " ";
			ssw << argv[i] << " ";
		}

		ssr << "--TEST_READER";
		ssw << "--TEST_WRITER";

		PROCESS_INFORMATION procinfor = PROCESS_INFORMATION();
		PROCESS_INFORMATION procinfow = PROCESS_INFORMATION();
		STARTUPINFO startupinfor = STARTUPINFO();
		STARTUPINFO startupinfow = STARTUPINFO();

		ZeroMemory(&startupinfor, sizeof(STARTUPINFO));
		ZeroMemory(&startupinfow, sizeof(STARTUPINFO));
		startupinfor.cb = sizeof(STARTUPINFO);
		startupinfow.cb = sizeof(STARTUPINFO);

		ZeroMemory(&procinfor, sizeof(PROCESS_INFORMATION));
		ZeroMemory(&procinfow, sizeof(PROCESS_INFORMATION));

		
		spdlog::info("Opening Read Process : {} \n", ssr.str());
		BOOL cprocr = CreateProcessA(nullptr, (LPSTR)ssr.str().c_str(), nullptr, nullptr, TRUE, NORMAL_PRIORITY_CLASS, nullptr, nullptr, &startupinfor, &procinfor);
		if (cprocr == 0) {			
			spdlog::warn("Fail to create reader process: {}\n", std::system_category().message(GetLastError()));
		}
		
		spdlog::info("Opening Write Process : {}\n", ssw.str());
		BOOL cprocw = CreateProcessA(nullptr, (LPSTR)ssw.str().c_str(), nullptr, nullptr, TRUE, NORMAL_PRIORITY_CLASS, nullptr, nullptr, &startupinfow, &procinfow);

		if (cprocw == 0) {
			spdlog::warn("Fail to create writer process: {}\n", std::system_category().message(GetLastError()));			
		}

		WaitForSingleObject(procinfow.hProcess, INFINITE);
		WaitForSingleObject(procinfor.hProcess, INFINITE);

		CloseHandle(procinfow.hProcess);
		CloseHandle(procinfow.hThread);
		CloseHandle(procinfor.hProcess);
		CloseHandle(procinfor.hThread);


	}


#endif // _linux_ / _WIN32

}
