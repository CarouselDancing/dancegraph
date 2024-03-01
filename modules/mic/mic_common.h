#pragma once

#define MAX_PACKETS 10000
#define MAX_PACKET_SIZE 100000
#define PACKET_SIZE  6000
struct MicPacketData
{
	int numSamples = 0;
	unsigned long long time = 0;
	int numChannels = 0;
	int sampleRate = 0;
	int packetID = 0;
	float data[PACKET_SIZE];
};