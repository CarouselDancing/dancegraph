#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <cstdio>
#include <iostream>
#include <vector>
#include <deque>

#include <modules/mic/mic_common.h>


#include <modules/mic/mic_common.h>
#include <deque>

#pragma once
class Capture
{
public:
	Capture();
	~Capture();
	IMMDevice* SelectDevice(int deviceIndex, bool defaultDevice);
	HRESULT Init(int deviceIndex, bool defaultDevice);
	HRESULT Start(std::vector<BYTE>& audioBytes);
	void Stop();
	HRESULT Shutdown();
	WAVEFORMATEX* GetFormat();
	void Clear() 
	{ 
		clearData = true; 
	}
	bool capturing;
	UINT32 bytesAvailable;
	bool clearing = false;
private:
	WAVEFORMATEX* format;
	IAudioClient* pAudioClient;
	IAudioCaptureClient* pCaptureClient;
	bool clearData;
	int toClear = 0;
};