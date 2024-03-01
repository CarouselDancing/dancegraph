#include "consumer.h"

#include <cstdio>
#include <iostream>
#include <sstream>

#include <spdlog/sinks/basic_file_sink.h>

#include <modules/mic/mic_common.h>
#include <modules/mic/wav_writer.h>

using namespace ipc;
using namespace std::chrono;

namespace sig
{
	struct MicConsumer
	{
		float* wavSamples;
		int numWavSamples = 250000, wavIndex = 0;
		bool wavWritten;

		DNRingbufferWriter* bufferWriter;
		std::ofstream outFile;
		MicPacketData* packetsBuffer;
		int maxPackets = 1, packetIndex = 0, prevId = -1;
		bool init(const sig::SignalProperties& sigProp)
		{
			outFile.open("ConsumerOutput.txt");
			spdlog::info("Initializing consumer\n");
			spdlog::set_default_logger(spdlog::basic_logger_mt("basic_logger", "Log_Buffertest_write.txt"));
			packetsBuffer = new MicPacketData[maxPackets];
			bufferWriter = new DNRingbufferWriter("Mic", sizeof(MicPacketData) * maxPackets, 3);
			wavSamples = new float[numWavSamples * 2];
			return true;
		}

		void shutdown()
		{
			delete[] packetsBuffer;
			outFile.close();
			spdlog::info("Shutting down consumer\n");
			delete bufferWriter;
		}

		void proc(const uint8_t* mem, int size, const sig::SignalMetadata& sigMeta)
		{
			//SignalHeader* header = (SignalHeader*)mem;
			SignalMetadata* header = (SignalMetadata*)mem;

			MicPacketData* packetData = (MicPacketData*)(mem + sizeof(SignalMetadata));
			// MicPacketData* packetData = (MicPacketData*)(mem + sizeof(SignalHeader));
			//printf("Data size: %d\n", packetData->numSamples);
			//packetData->time = header->timestamp;
			//for (int i = 0; i < PACKET_SIZE; i++)
			//{
			//	outFile << i << " " << packetData->data[i] << std::endl;
			//}

			unsigned long long ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
			int latency = ms - packetData->time;


			//printf("Start time: %llu\n", header->timestamp);
			//printf("This time: %llu\n", ms);
			spdlog::info("Latency: {}\n", latency);
			if (packetData->packetID != prevId)
			{
				//outFile << packetData->packetID << std::endl;

				prevId = packetData->packetID;
				packetsBuffer[packetIndex] = *packetData;
				packetIndex++;
				if (packetIndex == maxPackets)
				{
					RBError rerr = bufferWriter->write((void*)packetsBuffer);
					spdlog::info(  "Writing - Size: {}, Err : {}", size, RB_ErrorString(rerr));
					packetIndex = 0;
				}
			}

			//printf("Num packets: %d\n", packetData->numSamples);
			//printf("Last cons sample: %f\n", packetData->data[packetData->numSamples - 1]);
			//if (!wavWritten)
			//{

			//	for (int i = 0; i < PACKET_SIZE; i++)
			//	{
			//		wavSamples[wavIndex] = packetData->data[i];
			//		wavIndex++;
			//		if (wavIndex == numWavSamples * 2)
			//		{
			//			break;
			//		}
			//	}

			//	if (wavIndex == numWavSamples * 2)
			//	{
			//		if (WAV_Writer::Write(numWavSamples, packetData->sampleRate, packetData->numChannels, 32,
			//			3, wavSamples, "./ConsumerWav.wav") < 0)
			//		{
			//			printf("Failed wav write in reader\n");
			//			//outFile << "Failed wav write in reader\n";
			//		}
			//		else
			//		{
			//			printf("Wav written successfully in reader\n");
			//			//outFile << "Wav written successfully in reader\n";
			//		}
			//		wavWritten = true;
			//	}
			//}


		}
	};
}

sig::MicConsumer micConsumer;

// Initialize the signal consumer (alloc resources/caches, etc)
DYNALO_EXPORT bool DYNALO_CALL SignalConsumerInitialize(const sig::SignalProperties& sigProp, const sig::SignalConsumerRuntimeConfig& cfg)
{
	return micConsumer.init(sigProp);
}

// Process the signal data (mem: pointer to data, size: number of bytes, time: time that signal was acquired)
DYNALO_EXPORT void DYNALO_CALL ProcessSignalData(const uint8_t* mem, int size, const sig::SignalMetadata& sigMeta)
{
	micConsumer.proc(mem, size, sigMeta);
}

// Shutdown the signal consumer (free resources/caches, etc)
DYNALO_EXPORT void DYNALO_CALL SignalConsumerShutdown()
{
	micConsumer.shutdown();
}