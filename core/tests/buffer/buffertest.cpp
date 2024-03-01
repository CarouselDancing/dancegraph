

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

#include <stdlib.h> // Sigh. Malloc
#include <assert.h>

#include <spdlog/sinks/basic_file_sink.h>

#include <ipc/ringbuffer.h>
#include "csv.h"


// Crude ringbuffer test

// Currently reads BufferTestValues.csv for a table of tests to run (which are just reads and writes to the buffer at various rates of sequences of prime multiples of various sizes

// The command line argument is just the name of the test, as per the table

// Sadly, because the tests are unreliable, the 'test' methodology is to read the list of numbers spammed by the output of the reader and check whether it's what you expect; if your reader is too slow you expect that some of these writes will be missing, so writing an 'assert' to check this automatically will be a little annoying

using namespace ipc;


int primes[] = {2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67};
int primecount = sizeof(primes) / sizeof(int);



typedef struct Packet_ {
	int multiplier;
	int	count;
	int payload[0]; // This gets expanded with some icky mallocking
} Packet;


bool check_memory(Packet * p) {

	// Can't have a negative count
	if (p->count < 0) {
			spdlog::info(  "Check_Memory: count failure: {}", p->count);
			return false;
	}
	
	// Multiplier must be one of our set of primes
	bool ptest = false;
	for (auto prime : primes) {
		if (prime == p->multiplier)
			ptest = true;
	}
	
	// The multiplier isn't in the set of allowed values
	if (!ptest) {
		spdlog::info(  "Check_Memory: multiplier value is not allowed");
		return false;
	}

	// Now lets match them up

	for (int i = 0; i < p->count; i++) {
		if (i * p->multiplier != p->payload[i]) {
			spdlog::info(  "Check_Memory: Payload mismatch at offset {}", i);
			return false;
		}
	}
	return true;
}


void gotmem(const char * prefix, int * rmem, int count) {
	std::stringstream ss = std::stringstream();
	ss << prefix << " ";
	for (int i = 0; i < count; i++) {
		ss << rmem[i] << " ";
	}
	ss << "\n";
	spdlog::info("{}", ss.str());
}


void fillpacket(Packet * v) {
	for (int i = 0; i < v->count; i++) {
		v->payload[i] = i * v->multiplier;
	}

}


void test_writer(std::string name, int entrycount, int writesize, int numwrites, int writepause) {
	// entrycount == number of entries in the buffer
	// writesize == size of buffer
	// numwrites == number of times we're going to write something
	// writepause == length of time between writes

	spdlog::info(  "Creating writer with entry size: {}, and entrysize: {}", entrycount, sizeof(Packet) + sizeof(int) * writesize);
	DNRingbufferWriter writer = DNRingbufferWriter(name, sizeof(Packet) + sizeof(int) * writesize, entrycount);

	// Ugh at the variable sized struct hack
	Packet* p = (Packet*)malloc(sizeof(Packet) + writesize * sizeof(int));
	if (p == nullptr) {
		spdlog::info(  "Error: Can't allocate memory for test packet");
		exit(0);	
	}

	p->count = writesize;
	
	RBError rerr;
	
	for (int i = 0; i < numwrites; i++) {

		p->multiplier = primes[i % primecount];
		fillpacket(p);

		rerr = writer.write((void *)p);
		gotmem("Wrote", (int*)p->payload, 10);
		spdlog::info(  "Write #{}, Size: {}, Val: {}, Err: {}", i, writesize, p->multiplier, RB_ErrorString(rerr));
		std::this_thread::sleep_for(std::chrono::milliseconds(writepause));

	}
	std::this_thread::sleep_for(std::chrono::milliseconds(10000));

	free(p);
}

void test_reader(std::string name, int buffersize, int writesize, int readpause) {

	Packet* readmem = (Packet*)malloc(sizeof(Packet) + writesize * sizeof(int));

	DNRingbufferReader reader = DNRingbufferReader(name, sizeof(Packet) + sizeof(int) * writesize, buffersize );

	
	if (readmem == nullptr) {
		spdlog::info(  "Error: Can't allocate memory for test packet");
		exit(0);
	}

	spdlog::info(  "Packet has size {}", sizeof (Packet) + writesize * sizeof(int));
	
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	
	RBError rerr;
	for (int i = 0; i < 30; i++) {
	
		rerr = reader.read((Packet*) readmem);

		spdlog::info(  "Read Process Received: {}", RB_ErrorString(rerr));
		if (rerr == RBError::SUCCESS) {
			Packet * p = (Packet *) readmem;

			gotmem("Got", readmem->payload, 10);
			assert(check_memory(p));
			spdlog::info("Successful read: {} bytes with multiplier {}", readmem->count, readmem->multiplier);
			std::cout << fmt::format("Successful read:  bytes with multiplier {}\n", readmem->count, readmem->multiplier);

		}
		
		std::this_thread::sleep_for(std::chrono::milliseconds(readpause));
		
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(10000));

	free(readmem);
	
}

void writeTestToLog(std::string testname, int buffersize,int writesize, int numwrites, int writepause, int readpause) {

	spdlog::info(  "Test: {}, bufsize: {}, wsize: {}, numwrite: {}, wpause: {}, rpause: {}", testname, buffersize, writesize, numwrites, writepause, readpause);
}



int main(int argc, char** argv) {


	if (argc < 2) {
		spdlog::info(  "Needs a testname argument");
		return 0;
	}

	std::ifstream csvfile;

	csvfile.open("BufferTestValues.csv");
	auto parsedcsv = readCSV(csvfile);

	for (auto row = parsedcsv.begin(); row != parsedcsv.end(); row++) {

		if ((*row)[0] == std::string(argv[1])) {
			
			std::stringstream converter;
			
			int buffersize, numwrites, writepause, readpause, writesize;

			converter << (*row)[1]; converter >> buffersize; converter.clear();

			converter << (*row)[2]; converter >> writesize; converter.clear();

			converter << (*row)[3]; converter >> numwrites; converter.clear();
			converter << (*row)[4]; converter >> writepause; converter.clear();
			converter << (*row)[5]; converter >> readpause; converter.clear();

			std::string testname = std::string("ringbuffertest_").append((*row)[0]);

			
#ifdef __linux__
			if (fork() == 0) {
				spdlog::basic_logger_mt("basic_logger", "Log_Buffertest_write.txt");
				writeTestToLog(testname, buffersize, writesize, numwrites, writepause, readpause);
				test_writer(testname, buffersize, writesize, numwrites, writepause);
			}
			else {
				spdlog::basic_logger_mt("basic_logger", "Log_Buffertest_read.txt");
				writeTestToLog(testname, buffersize, writesize, numwrites, writepause, readpause);
				test_reader(testname, buffersize, writesize, readpause);
			}

#elif _WIN32
			// Windows don't fork so we'll just invoke this program with separate arguments for a similar effect

			if (argc > 2 && !strcmp(argv[2], "_TEST_WRITER")) {
				
				spdlog::set_default_logger(spdlog::basic_logger_mt("basic_logger", "Log_Buffertest_write.txt"));
				writeTestToLog(testname, buffersize, writesize, numwrites, writepause, readpause);
				test_writer(testname, buffersize, writesize, numwrites, writepause);
				
			}
			else if(argc > 2 && !strcmp(argv[2], "_TEST_READER")) {
				
				spdlog::set_default_logger(spdlog::basic_logger_mt("basic_logger", "Log_Buffertest_read.txt"));
				writeTestToLog(testname, buffersize, writesize, numwrites, writepause, readpause);
				test_reader(testname, buffersize, writesize, readpause);
			}
			else {
				spdlog::info(  "Got testname {}", testname);

				spdlog::set_default_logger(spdlog::basic_logger_mt("basic_logger", "Log_Buffertest_control.txt"));
				std::stringstream ssr = std::stringstream();
				std::stringstream ssw = std::stringstream();

				ssr << argv[0] << " " << argv[1] << " _TEST_READER";
				ssw << argv[0] << " " << argv[1] << " _TEST_WRITER";

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
				
				spdlog::info(  "Opening Read Process : {}", ssr.str());
			
				BOOL cprocr = CreateProcessA(nullptr, (LPSTR) ssr.str().c_str(), nullptr, nullptr, TRUE, NORMAL_PRIORITY_CLASS, nullptr, nullptr, &startupinfor, &procinfor);
				if (cprocr == 0) {
					spdlog::info(  "Fail to create reader process:{}", std::system_category().message(GetLastError()));
				}

				spdlog::info(  "Opening Write Process : {}", ssw.str());

				BOOL cprocw = CreateProcessA(nullptr, (LPSTR) ssw.str().c_str(), nullptr, nullptr, TRUE, NORMAL_PRIORITY_CLASS, nullptr, nullptr, &startupinfow, &procinfow);
			
				if (cprocw == 0) {
					spdlog::info(  "Fail to create writer process:{}", std::system_category().message(GetLastError()));
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
	}
}
