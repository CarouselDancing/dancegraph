#pragma once

#include <cstdint>
#include <core/net/config_master.h>
#define DGRAPH_API __declspec(dllexport) 

namespace sig
{
	struct SignalMetadata;
}


namespace dll
{

	extern "C" {

		// Provide a function callback to C++, so that when we call this, we get a message in the Debug.Log in Unity
		DGRAPH_API void InitBindings(void(*debugLog)(const char * s));

		// initialize enet and dancegraph master config
		DGRAPH_API bool Initialize();
		
		// Start a connection with the server
		DGRAPH_API bool Connect(const char* name, const char * serverIp, int serverPort, int localPort, const char* scene_name, const char* user_role);
		DGRAPH_API bool ConnectJson(const char* jsonString);

		// Applicable to async tick mode
		DGRAPH_API void SetNativeTickMs(int ms);

		//DGRAPH_API bool Connect(const char* jsonPreset);

		// Initialize plugin/resource
		DGRAPH_API void InitializePlugin();
		
		// Cleanup plugin/resources
		DGRAPH_API void ShutDownPlugin();

		// Initialize plugin/resource
		DGRAPH_API void ConnectAsClient();
		DGRAPH_API void ConnectAsListener();
		DGRAPH_API void Disconnect();

		// TODO: All (other) control messages

		// Network tick, polling server messages
		DGRAPH_API void Tick();
		
		// When native client/listener gets a message, call this function with the message data
		DGRAPH_API void RegisterSignalCallback(void(*onSignalData)(const uint8_t* stream, int numBytes, const sig::SignalMetadata& sigMeta));

		//DGRAPH_API void RegisterConfigPreset(char* filename, char* preset);

		DGRAPH_API void NativeDebugLog(const char* string);

		// Trigger a producer-like signal, e.g. an environment interaction, pause/resume music, collision with actor
		DGRAPH_API void SendEnvSignal(const uint8_t * stream, int numBytes, int sigIdx);

		// EnvAdapter code
		DGRAPH_API bool ReadLocalEnvData();
		DGRAPH_API int GetEnvSignal(char ip[]);
		DGRAPH_API void SendSignal(char* stream, int numBytes);

		// TEMPORARY! Set a zed replay filename. IF this is set, then we'll add a producer override upon connection
		DGRAPH_API void SetZedReplay(const char* replayFilename);

		// TEMPORARY! Return the local ID
		DGRAPH_API int GetUserIdx();

		DGRAPH_API void SetLogLevel(int ll);
		DGRAPH_API void SetLogToFile(const char * logFile);

		// GetLastError was taken
		DGRAPH_API void GetLastDanceGraphError(char *, unsigned int);


	}
}