#pragma once

#include <cstdint>

#include <modules/zed/zed_common.h>


#define DGRAPH_API __declspec(dllexport) 
 
namespace dll
{
	extern "C" {

		// Provide a function callback to C++, so that when we call this, we get a message in the Debug.Log in Unity
		DGRAPH_API void InitBindings(void(*debugLog)(const char * s));
		// Initialize networking (call once)
		DGRAPH_API void Initialize();
		
		// Start a connection with the server
		DGRAPH_API void Connect(const char* name, const char * serverIp, int serverPort, int localPort);
		
		// Send some HMD data to the server
		DGRAPH_API void SignalCallback_Hmd(float* xform);

		// Stub - supposedly send some ZED data to the server
		DGRAPH_API void SignalCallback_Zed(float * zedData);
		
		// Fetch data from the attached local Zedcams given a ZedBodies instance
		//DGRAPH_API bool GetLocalZedData(zed::UnityZedData & data);

		DGRAPH_API bool ReadLocalZedData();
		DGRAPH_API int GetLocalZedBodyCount();
		DGRAPH_API long long GetLocalZedElapsed(); 

		DGRAPH_API void GetLocalZedRootTransform(float []);
		DGRAPH_API void GetLocalZedBoneData(float []);
		DGRAPH_API void GetLocalZedBodyIDs(int []);
		
		DGRAPH_API int GetUserID();
		DGRAPH_API int GetPacketID();
		DGRAPH_API long long GetPacketTimestamp();
		DGRAPH_API int GetDataSize();
		// Uninitialize
		DGRAPH_API void ShutDown();

		// Network "tick" which reads any pending messages from the server. Unity still has no way to access these
		DGRAPH_API void Tick();


		// Environment Signals
		DGRAPH_API bool ReadLocalEnvData();
		DGRAPH_API int GetEnvSignal(char ip[]);

		// Trigger a producer-like signal, typically env
		DGRAPH_API void SendSignal(char* stream, int numBytes);

		// Generic API
		
		// When native client/listener gets a message, call this function with the message data
		DGRAPH_API void RegisterSignalCallback(void(*onSignalData)(const uint8_t * stream), int numBytes);


	}
}