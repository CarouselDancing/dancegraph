
#define DLL_MODE

#define ENET_IMPLEMENTATION

#include "dancegraph.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <core/net/net.h>
#include <core/net/client.h>
#include <core/net/message.h>
#include <core/net/formatter.h>

#include <magic_enum/magic_enum.hpp>

#include <core/net/config.h>
#include <core/net/config_master.h>

//#include <modules/zed/zed_common.h>

#include "zed_adapter.h"
#include "env_adapter.h"

net::ClientState g_ClientState;


namespace dll
{
	extern std::unique_ptr<ZedAdapter> zedAdapterp;
	extern std::unique_ptr<EnvAdapter> envAdapterp;

	net::cfg::Root master_cfg;

	extern "C" {

		DGRAPH_API void InitBindings(void(*debugLog)(const char* s))
		{
			set_log_callback(debugLog);
		}

		DGRAPH_API void Initialize()
		{				
			spdlog::set_default_logger(spdlog::basic_logger_mt("basic_logger", "ZED_Adapter_Log.txt"));
			
			spdlog::info("Calling the Dancegraph Plugin Initializer");
			
			dll::zed_initialize();
			dll::env_initialize();
			auto success = net::initialize();
		}
		
		DGRAPH_API void Connect(const char* name, const char * serverIp, int serverPort, int thisPort)
		{

			/*
			auto thisIp = net::get_local_ip_address();

			net::cfg::Client cfg;
			

			net::ClientConfig cfg;
			cfg.common.address = { thisIp, thisPort };
			cfg.common.name = name;
			cfg.common.server_address = { serverIp, serverPort };
			// TODO: add more parameters! like user role, etc

			
			g_ClientState.initialize(cfg);
			*/
		}
		

		
		DGRAPH_API void SignalCallback_Hmd(float* xform)
		{

			//auto hmd = *(net::msg::Hmd*)xform;
			//g_ClientState.send_and_store_signal(hmd);
		}

		DGRAPH_API void SignalCallback_Zed(float* zedData)
		{
			
			//auto hmd = *(net::msg::Hmd*)xform;
			//g_ClientState.send_and_store_signal(hmd);
			
		}



		DGRAPH_API void Tick()
		{
			g_ClientState.tick();
		}

		
		// TODO: Make this generic pluggable-innable (more dlls?)
	/*
		DGRAPH_API bool GetLocalZedData(zed::UnityZedData & bodyData) {
			return true;
		}
		*/

		DGRAPH_API void Shutdown() {
			zed_shutdown();
		}

		DGRAPH_API bool ReadDataHeader() {
			return false;
		}

		DGRAPH_API bool ReadLocalZedData() {
			bool b = zedAdapterp->read_ipc();
			return b;
		}

		DGRAPH_API int GetLocalZedBodyCount() {
			return zedAdapterp->get_numbodies();

		}


		DGRAPH_API long long GetLocalZedElapsed() {
			long long xx = zedAdapterp->get_elapsed();
			spdlog::info("Got elapsed in plugin: {}", xx);
			return (long long) zedAdapterp->get_elapsed();
		}

		DGRAPH_API void GetLocalZedRootTransform(float fp []) {
			zedAdapterp->get_roottransform(fp);

		}
		DGRAPH_API void GetLocalZedBoneData(float fp []) {
			zedAdapterp->get_bodyData(fp);
		}

		DGRAPH_API void GetLocalZedBodyIDs(int ip []) {
			zedAdapterp->get_bodyIDs(ip);
		}

		DGRAPH_API int GetUserID() {			
			return zedAdapterp->get_userID();
		}

		DGRAPH_API int GetPacketID() {			
			return zedAdapterp->get_packetID();
		}

		DGRAPH_API long long GetPacketTimestamp() {
			long long  xx = zedAdapterp->get_timestamp(false);
			spdlog::info("Got Packet Timestamp in plugin: {}", xx);
			return zedAdapterp->get_timestamp(false);
		}

		DGRAPH_API int GetDataSize() {			
			return zedAdapterp->get_dataSize();			
		}


		// When native client/listener gets a message, call this function with the message data
		DGRAPH_API void RegisterSignalCallback(void(*onSignalData)(const uint8_t* stream), int numBytes)
		{
			// TODO: update client/listener with this callback
		}

		// Trigger a producer-like signal, typically env
		DGRAPH_API void SendSignal(char* stream, int numBytes)
		{
			envAdapterp->send_env_msg(stream, numBytes);			
		}

		DGRAPH_API int GetEnvSignal(char ip[]) {

			return envAdapterp->get_env_msg(ip);			
			return 0;
		}

		DGRAPH_API bool ReadLocalEnvData() {
			bool b = envAdapterp->read_ipc();						
			return b;
		}


	}
}

