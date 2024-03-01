#include <thread>
#include <mutex>
#include <spdlog/sinks/basic_file_sink.h>
#include <modules/mic/wav_writer.h>
#include "reader.h"

using namespace ipc;
using namespace std::chrono;

#define DllExport __declspec (dllexport)

bool isEscapePressed() {
	return ((GetAsyncKeyState(VK_ESCAPE) & 0x7fff) == 1);
}

#define MAX_BUFFER 1000000

float* wavSamples;
int numWavSamples = 250000, wavIndex = 0;
bool wavWritten = false;
int prevTime = 0;
int main() // Functionality for testing reading from IPC
{
	spdlog::set_default_logger(spdlog::basic_logger_mt("basic_logger", "Log_Buffertest_read.txt"));
	DNRingbufferReader* bufferReader = new DNRingbufferReader("Mic", sizeof(MicPacketData), 3);
	MicPacketData* packetData = (MicPacketData*)malloc(sizeof(MicPacketData));
	wavSamples = new float[numWavSamples * 2];
	bool run = true;
	while (run)
	{
		RBError rerr = bufferReader->read((void*)packetData);
		spdlog::info(  "Reading - Size: {}, Err : {}", PACKET_SIZE, RB_ErrorString(rerr));
		if (rerr == RBError::SUCCESS && packetData->time != prevTime)
		{
			prevTime = packetData->time;
			if (!wavWritten)
			{
				for (int i = 0; i < packetData->numSamples; i++)
				{
					wavSamples[wavIndex] = packetData->data[i];
					wavIndex++;
					if (wavIndex == numWavSamples * 2)
					{
						break;
					}
				}

				if (wavIndex == numWavSamples * 2)
				{
					if (WAV_Writer::Write(numWavSamples, packetData->sampleRate, packetData->numChannels, 32,
						3, wavSamples, "./ReaderWav.wav") < 0)
					{
						spdlog::info("Failed wav write in reader\n");
						//outFile << "Failed wav write in reader\n";
					}
					else
					{
						spdlog::info("Wav written successfully in reader\n");
						//outFile << "Wav written successfully in reader\n";
					}
					wavWritten = true;
				}
			}
		}
		if (isEscapePressed()) {
			run = false;
		}
	}
	free(packetData);
	delete bufferReader;
	return 1;
}

//#define PACKET_SIZE 10000

DNRingbufferReader* bufferReader;
std::thread readThread;
	
float* filledPacket;
std::ofstream outFile;

bool reading = false;
//int channels = -1, sampleRate = -1;
int lastId = -1;
int packetSize;
typedef void(__stdcall* READCALLBACK)(int);
static READCALLBACK ReadCallback;
bool packetFilled = false;

const int readPackets = 1;
std::vector<MicPacketData> packetsBuffer;
// Reads data from IPC to be pulled into Unity
void ReadIPC()
{
	reading = true;
	wavSamples = new float[numWavSamples * 2];
	MicPacketData* packetData = new MicPacketData[readPackets];

	while (reading) 
	{
		RBError rerr = bufferReader->read((void*)packetData);
		spdlog::info(  "Reading - Size: {}, Err : {}", sizeof(MicPacketData), RB_ErrorString(rerr));

		if (rerr == RBError::SUCCESS && packetData[0].packetID != lastId)
		{
			lastId = packetData[0].packetID;
 
			outFile << duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - packetData[0].time << std::endl;
			for (int i = 0; i < readPackets; i++)
			{
				packetsBuffer.push_back(packetData[i]);
			}
		}
	}

	delete[] packetData;
	//free(packetData);
}

	// Thread for reading data from IPC
	std::thread ReadThread()
	{
		return std::thread([=] { ReadIPC(); });
	}
	

extern "C" // Native plugin functionality
{


	DllExport float* GetPacket(bool* packetAvailable, int* numChannels, int* sampleRate, unsigned long long* packetTime)
	{
		if (packetsBuffer.size() > 0)
		{
			*packetAvailable = true;
			*numChannels = packetsBuffer[0].numChannels;
			*sampleRate = packetsBuffer[0].sampleRate;
			*packetTime = packetsBuffer[0].time;
			memcpy(filledPacket, packetsBuffer[0].data, PACKET_SIZE * sizeof(float));
			packetsBuffer.erase(packetsBuffer.begin());

		}
		else
		{
			*packetAvailable = false;
			*numChannels = -1;
			*sampleRate = -1;
			*packetTime = -1;
		}

		return filledPacket;
	}

 //DllExport float* GetSamples(int* numChannels, int* outSampleRate)
	//{
	//	*numChannels = channels;
	//	*outSampleRate = sampleRate;

	//}

	DllExport void StartThread()
	{
		// Start reading thread
		readThread = ReadThread();
	}

	// Initialize IPC reader functionality
	DllExport int Init()
	{
		outFile.open("ReaderOutput.txt");
		// Initialize IPC reader
		spdlog::basic_logger_mt("basic_logger", "Log_mic_read.txt");
		bufferReader = new DNRingbufferReader("Mic", sizeof(MicPacketData) * readPackets, 3);
		filledPacket = new float[PACKET_SIZE];
		return PACKET_SIZE;
	}

	// Shutdown thread and delete dynamic memory
	DllExport void Shutdown()
	{
		reading = false; // Stop reading from IPC
		readThread.join(); // Wait until read thread has closed
		outFile.close();

		// Delete dynamic memory
		delete bufferReader;
		//collectiveBuffer.~vector();
		delete[] filledPacket;

		if (packetsBuffer.size() > 0)
		{
			packetsBuffer.clear();
		}
	}
}