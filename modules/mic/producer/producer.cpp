//#include <modules/mic/producer/producer.h>
#include "producer.h"
#include <modules/mic/wav_writer.h>
using namespace std::chrono;

namespace sig
{
	struct MicProducer
	{
		Capture capture;
		std::thread captureThread;
		std::ofstream outFile;
		std::vector<BYTE> audioBytes;
		WAVEFORMATEX* audioFormat = NULL;
		float* wavSamples;
		int numWavSamples = 250000, wavIndex = 0;
		bool wavWritten;
		std::thread CaptureThread() // Thread for capturing audio
		{
			return std::thread([=] { capture.Start(audioBytes); });
		}

		bool init(const sig::SignalProperties& sigProp)
		{
			spdlog::info("Initializing producer\n");
			bool success = true;
			HRESULT hr = CoInitialize(nullptr);
			outFile.open("ProducerOutput.txt");
			if (FAILED(hr))
			{
				spdlog::info("FAILED TO INIT WASAPI\n");
				return false;
			}

			hr = capture.Init(1, true);
			if (FAILED(hr))
			{
				spdlog::info("FAILED TO INIT CAPTURE\n");
				return false;
			}
			audioFormat = capture.GetFormat();

			captureThread = CaptureThread();

			wavSamples = new float[numWavSamples * audioFormat->nChannels];
			spdlog::info("BPS: {}\n", audioFormat->wBitsPerSample);
			return true;
		}

		void shutdown()
		{
			outFile.close();
			spdlog::info("Shutting down producer\n");

			// Stop capture and wait for thread to end
			capture.Stop();
			captureThread.join();

			HRESULT hr = capture.Shutdown();
			if (FAILED(hr))
			{
				spdlog::info("FAILED TO SHUTDOWN CAPTURE\n");
			}
			audioBytes.clear();
			CoUninitialize();

			delete[] wavSamples;
		}
		int packetId = 0;

		int get_data(uint8_t* mem, sig::time_point& time)
		{
			MicPacketData* packetData = (MicPacketData*)mem;
			packetData->time = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

			int dataSize = 0;
			int sampleSize = sizeof(float);
			int numBytes = audioBytes.size();

			if (numBytes > PACKET_SIZE * sampleSize)
			{

				dataSize = PACKET_SIZE * sampleSize + sizeof(SignalMetadata);
				memcpy(packetData->data, audioBytes.data(), PACKET_SIZE * sampleSize);
				//printf("Num samples: %d\n", audioBytes.size());
				//audioBytes.clear();
				audioBytes.erase(audioBytes.begin(), audioBytes.begin() + PACKET_SIZE * sampleSize);
				packetData->numSamples = PACKET_SIZE;

				packetId++;
				outFile << packetId << std::endl;

				//printf("Last prod sample: %f\n", packetData->data[packetData->numSamples - 1]);
				//if (!wavWritten)
				//{ 

				//	for (int i = 0; i < packetData->numSamples; i++)
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
				//			3, wavSamples, "./ProducerWav.wav") < 0)
				//		{
				//			printf("Failed wav write in producer\n");
				//			//outFile << "Failed wav write in reader\n";
				//		}
				//		else
				//		{
				//			printf("Wav written successfully in producer\n");
				//			//outFile << "Wav written successfully in reader\n";
				//		}
				//		wavWritten = true;
				//	}
				//}
			}
			else
			{
				packetData->numSamples = 0;
			}

			time = time_now();

			packetData->numChannels = audioFormat->nChannels;
			packetData->sampleRate = audioFormat->nSamplesPerSec;
			packetData->packetID = packetId;

			//if (dataSize > 0)
			//{
			//	printf("NumSamples %d\n", packetData->numSamples);
			//	for (int i = 0; i < PACKET_SIZE; i++)
			//	{
			//		outFile << i << " " << packetData->data[i] << std::endl;
			//	}
			//}

			return dataSize;
		}
	};
}

sig::MicProducer micProducer;

// Initialize the signal producer (alloc resources/caches, etc)
DYNALO_EXPORT bool DYNALO_CALL SignalProducerInitialize(const sig::SignalProperties& sigProp)
{
	spdlog::set_default_logger(sigProp.logger);
	return micProducer.init(sigProp);
}

// Get signal data and the time of generation. Return number of written bytes
DYNALO_EXPORT int DYNALO_CALL GetSignalData(uint8_t* mem, sig::time_point& time)
{
	return micProducer.get_data(mem, time);
}

// Shutdown the signal producer (free resources/caches, etc)
DYNALO_EXPORT void DYNALO_CALL SignalProducerShutdown()
{
	micProducer.shutdown();
}