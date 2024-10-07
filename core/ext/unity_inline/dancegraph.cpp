#include "dancegraph.h"

//#include <spdlog/sinks/>

#define DLL_MODE
#define ENET_IMPLEMENTATION
//#define ASYNC_TICK

#include <core/net/net.h>
#include <core/net/client.h>
#include <core/net/listener.h>
#include <core/net/message.h>
#include <core/net/formatter.h>

#include <spdlog/sinks/callback_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <magic_enum/magic_enum.hpp>

#include <core/net/channel.h>
#include <core/net/config.h>

//#include "env_adapter/env_adapter.h"

#ifdef ASYNC_TICK
#include <thread>
#include <mutex>
#include <atomic>

#include <core/common/task_queue.h>
#endif

std::unique_ptr<net::ClientState> g_ClientState;
std::unique_ptr<net::ListenerState> g_ListenerState;

bool isInitialized = false;

void(*onSignalData)(const uint8_t* stream, int numBytes, const sig::SignalMetadata& sigMeta) = nullptr;

const std::string DEFAULT_USERPRESET = std::string("default_localuser");

// TODO : Place this in 
constexpr char DANCEGRAPH_PRESETS_FILENAME[] = "dancegraph_rt_presets.json";


#ifdef ASYNC_TICK

/*
*	What are the differences with the single-thread version?
*		Initialize:				Nothing
*		ConnectJson:			Fire up the thread 
*		Disconnect:				Send shutdown request
* 
*		GetUserIdx				atomic variable
*		SetLogLevel				action to consume
*		SetLogToFile			action to consume
* 
*		Tick					Run the adapter's task queue processing
*		RegisterSignalCallback	The callback is going to used when we're consuming native-to-adapter signal data tasks
*		SendEnvSignal			Nothing -- IPC reading happens at adapter thread
*		GetEnvSignal			Nothing -- IPC reading happens at adapter thread
*		ReadLocalEnvData		Nothing -- IPC reading happens at adapter thread
*		NativeDebugLog			action to consume
*/

// threaded decoupling work
namespace td
{
	int threadedTickMs = 10;

	std::jthread nativeRunner;
	dancegraph::TaskQueue adapterTaskList; // This contains tasks assigned FROM the adapter TO the plugin
	dancegraph::TaskQueue nativeTaskList;  // This contains tasks assigned FROM the plugin TO the adapter

	struct record_adapter_signal_telemetry_task_t
	{
		sig::SignalMetadata sigMeta;
		sig::time_point time;

		void operator()() const
		{
			g_ClientState->record_adapter_signal_telemetry(sigMeta, time);
		}
	};

	struct native_to_adapter_signal_task_t
	{
		std::vector<uint8_t> data;
		sig::SignalMetadata metadata;

		native_to_adapter_signal_task_t() = default;

		// Create this from the native plugin
		native_to_adapter_signal_task_t(const uint8_t* stream, int numBytes, const sig::SignalMetadata& sigMeta)
			:data(stream, stream+numBytes)
			,metadata(sigMeta)
		{
		}

		// Execute this from the adapter
		void operator()() const
		{
			td::adapterTaskList.AppendLocalTask(td::record_adapter_signal_telemetry_task_t{ sigMeta = sigMeta, time = sig::time_now() });
			onSignalData(data.data(), int(data.size()), metadata);
		}
	};

	struct set_log_level_task_t
	{
		int logLevel=0;

		void operator()() const
		{
			spdlog::set_level((spdlog::level::level_enum)logLevel);
		}
	};

	struct set_log_to_file_task_t
	{
		std::string logFile;

		void operator()() const
		{
			static bool done = false;
			if (!done)
			{
				done = true;
				spdlog::default_logger()->sinks().emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFile));
			}
		}
	};

	struct native_debug_log_task_t
	{
		std::string msg;

		void operator()() const
		{
			spdlog::info(msg);
		}
	};

	

	std::atomic_bool native_runner_has_connected = false;

	void native_runner(std::stop_token stopToken, const net::cfg::Client& cfg)
	{
		native_runner_has_connected = false;
		g_ClientState = std::make_unique<net::ClientState>();
		auto ok = g_ClientState->initialize(cfg);
		if (ok)
		{
			native_runner_has_connected = true;
			// When we get signal data, save a local task
			g_ClientState->register_signal_callback([](const uint8_t* stream, int numBytes, const sig::SignalMetadata& sigMeta) {
				nativeTaskList.AppendLocalTask(native_to_adapter_signal_task_t(stream,numBytes, sigMeta));
			});

			const char* labelNative = "Native";
			uint32_t tickId = 0;
			auto start_time = std::chrono::high_resolution_clock::now();
			while (!stopToken.stop_requested())
			{
				auto current_time = std::chrono::high_resolution_clock::now();
				auto elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(current_time - start_time).count();
				if(elased_time < threadedTickMs)
					std::this_thread::sleep_for(std::chrono::milliseconds(threadedTickMs - elapsed_time));

				// Process input tasks, if any
				auto [numConsumed, numLeft] = adapterTaskList.TryConsumeTasks();
				if (numLeft >= 0)
					spdlog::info("{0}: consumed {1} left {2} (tick={3})", labelNative, numConsumed, numLeft, tickId);
				else
					spdlog::warn("{0}: Failed to get the lock to consume tasks (tick={1})", labelNative, tickId);

				// Tick!
				g_ClientState->tick();

				auto moved = nativeTaskList.TryMoveLocalToShared();
				if (moved)
					spdlog::info("{0}: moved local to shared (tick={1})", labelNative, tickId);
				else
					spdlog::warn("{0}: DID NOT MOVE local to shared (tick={1})", labelNative, tickId);
				++tickId;
			}
		}
		else
			spdlog::default_logger()->error("Error 
				
				client");
		native_runner_has_connected = false;
		g_ClientState->deinitialize();
		g_ClientState = {};
	}

	bool init(const net::cfg::Client& cfg)
	{
		using namespace std::chrono_literals;
		adapterTaskList.Clear();
		nativeTaskList.Clear();
		nativeRunner = std::jthread(native_runner, cfg);

		// wait until we're connected, up to 2 seconds
		int numTries = 200;
		const int msWaitPerTry = 100;
		while(!native_runner_has_connected && --numTries>=0)
			std::this_thread::sleep_for(100ms);
		return native_runner_has_connected;
	}
}

#endif


namespace dll
{
	//extern std::unique_ptr<EnvAdapter> envAdapterp;
	
	extern "C" {
#define MAX_ERROR_STRING_LENGTH 150
		
		char last_error[MAX_ERROR_STRING_LENGTH+1];
		spdlog::level::level_enum minimum_level;

		//std::string last_error = std::string("Unknown error");

		DGRAPH_API void InitBindings(void(*debugLog)(const char* s))
		{
			set_log_callback(debugLog);

			strncpy(last_error, "Unknown Error", MAX_ERROR_STRING_LENGTH+1); // Ensure trailing zero, thanks C
			minimum_level = spdlog::level::err; // Errors, or critical only!

			spdlog::default_logger()->sinks().emplace_back(std::make_shared<spdlog::sinks::callback_sink_mt>([debugLog](const spdlog::details::log_msg& msg) {
				
				
				if (msg.level >= minimum_level) {
					
					strncpy(last_error, msg.payload.data(), MAX_ERROR_STRING_LENGTH);
					minimum_level = msg.level;
				}
				auto s = fmt::format("[NATIVE] [{}] {}", magic_enum::enum_name(msg.level).data(), std::string(std::string_view(msg.payload.data(), msg.payload.size())).c_str());
				debugLog(s.c_str());
			}));
		}

		DGRAPH_API void GetLastDanceGraphError(char * strptr, int len) {

			unsigned int minval = (len < MAX_ERROR_STRING_LENGTH) ? len : MAX_ERROR_STRING_LENGTH;
			strncpy(strptr, last_error, minval);			
		}

		DGRAPH_API void SetNativeTickMs(int ms)
		{
#ifdef ASYNC_TICK 
			td::threadedTickMs = ms;
#endif
		}

		DGRAPH_API bool Initialize()
		{				
			if (!isInitialized)
			{
				if (!net::cfg::Root::load())
					return false;
				if(!net::initialize())
					return false;
				isInitialized = true;
			}
			return true;
		}

		DGRAPH_API void Disconnect()
		{
#ifdef ASYNC_TICK
			td::nativeRunner.request_stop();
			td::nativeRunner.join();
			td::nativeRunner = {};
#else
			if (isInitialized)
			{
				if (g_ClientState)
				{
					g_ClientState->deinitialize();
					g_ClientState.reset();
				}
				else if (g_ListenerState)
				{
					g_ListenerState->deinitialize();
					g_ListenerState.reset();
				}
			}
#endif
		}

		static std::string g_ZedReplayFilename;
		DGRAPH_API void SetZedReplay(const char* replayFilename)
		{
			g_ZedReplayFilename = replayFilename;
		}

		DGRAPH_API int GetUserIdx()
		{	

			// If we don't catch the null clientstate, Unity crashes, because it seems to be calling GetUserIdx inappropriately			
			if (g_ClientState)
				return g_ClientState->UserIdx();
			else {
				// Strangely, Unity doesn't print the error message
				spdlog::warn("Attempting to access a null client state! This is bad!");
				return -1;
			}
			
		}

		DGRAPH_API void SetLogLevel(int ll)
		{
#ifdef ASYNC_TICK
			td::adapterTaskList.AppendLocalTask(td::set_log_level_task_t{ ll = ll });
#else
			spdlog::set_level((spdlog::level::level_enum)ll);
#endif
		}

		DGRAPH_API void SetLogToFile(const char* logFile)
		{
#ifdef ASYNC_TICK
			td::adapterTaskList.AppendLocalTask(td::set_log_to_file_task_t{ logFile = logFile});
#else
			static bool done = false;
			if (!done)
			{
				done = true;
				spdlog::default_logger()->sinks().emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFile));
			}
#endif
		}
		
		
		DGRAPH_API bool Connect(const char* name, const char * serverIp, int serverPort, int thisPort, const char * scene_name, const char * user_role)
		{
			if (!Initialize())
				return false;
			g_ClientState = std::make_unique<net::ClientState>();
			auto thisIp = net::get_local_ip_address();
			net::cfg::Client cfg;
			cfg.address = { thisIp, thisPort };
			cfg.username = name;
			cfg.server_address = { serverIp, serverPort };
			cfg.scene = scene_name;
			cfg.role = user_role;
			cfg.include_ipc_consumers = false; // No ipc for now -- direct DLL calls
			
			
			
			//cfg.producer_overrides["zed/v2.0"] = {"camera"};
			cfg.producer_overrides["zed/v2.1"] = { "camera" };

			// TODO: add more parameters! like user role, etc
			auto ok = g_ClientState->initialize(cfg);
			if (ok && onSignalData != nullptr)
			{
				g_ClientState->register_signal_callback(onSignalData);
			}
			return ok;
		}
		

		DGRAPH_API bool ConnectJson(const char* jsonString)
		{
			if (!Initialize())
				return false;
			net::cfg::Client cfg = nlohmann::json::parse(jsonString);
#ifdef ASYNC_TICK
			return td::init(cfg);
#else

			g_ClientState = std::make_unique<net::ClientState>();
			auto ok = g_ClientState->initialize(cfg);

			if (!ok)
			{
				spdlog::debug("Failed ClientState initialization");
				g_ClientState->deinitialize();
				g_ClientState = {};
			}
			if (ok && onSignalData != nullptr)
			{

				spdlog::debug("Registering connection callback");
				g_ClientState->register_signal_callback([](const uint8_t* stream, int numBytes, const sig::SignalMetadata& sigMeta) {
					g_ClientState->record_adapter_signal_telemetry(sigMeta, sig::time_now());
					onSignalData(stream,numBytes, sigMeta);
				});

			}
			return ok;
#endif
		}


		DGRAPH_API void Tick()
		{
			if (!isInitialized)
				return;
#ifdef ASYNC_TICK
			if (td::native_runner_has_connected)
			{
				// Process input tasks, if any
				auto [numConsumed, numLeft] = td::nativeTaskList.TryConsumeTasks();
				const char * labelAdapter = "Adapter";
				if (numLeft >= 0)
					NativeDebugLog(std::format("{0}: consumed {1} left {2}", labelAdapter, numConsumed, numLeft).c_str());
				else
					NativeDebugLog(std::format("ERROR {0}: Failed to get the lock to consume tasks", labelAdapter, numConsumed, numLeft).c_str());

				auto moved = td::adapterTaskList.TryMoveLocalToShared();
				if (moved)
					NativeDebugLog(std::format("{0}: moved local to shared", labelAdapter).c_str());
				else
					NativeDebugLog(std::format("ERROR {0}: DID NOT MOVE local to shared", labelAdapter).c_str());
			}
#else
			if(g_ClientState)
				g_ClientState->tick();
#endif
			else if (g_ListenerState)
				g_ListenerState->tick();
		}

		// When native client/listener gets a message, call this function with the message data
		DGRAPH_API void RegisterSignalCallback(void(*zOnSignalData)(const uint8_t* stream, int numBytes, const sig::SignalMetadata& sigMeta))
		{
			onSignalData = zOnSignalData;
#if 0
			// Just for early validation before tick() - remove that later
			uint8_t stream[] = { 1,2,3,4 };
			sig::SignalMetadata sigMeta;
			sigMeta.acquisitionTime = {};
			sigMeta.packetId = 123;
			sigMeta.sigIdx = 234;
			sigMeta.sigType = (uint8_t)net::SignalType::Client;
			sigMeta.userIdx = 244;
			zOnSignalData(stream, 4, sigMeta);
#endif
		}

		// Trigger a producer-like signal, typically env
		DGRAPH_API void SendEnvSignal(const uint8_t* stream, int numBytes, int sigIdx)
		{
			// TODO: update client/listener with this data
			if (g_ClientState)
			{
				static std::vector<uint32_t> packetCounters;
				if (packetCounters.size() <= sigIdx)
					packetCounters.resize(sigIdx + 1, 0);
				sig::SignalMetadata sigMeta{ sig::time_now(), packetCounters[sigIdx]++, g_ClientState->UserIdx(), uint8_t(sigIdx), uint8_t(net::SignalType::Environment)};
				g_ClientState->send_signal(stream, numBytes, sigMeta);
			}
			else
			{
				spdlog::warn("Trying to sending a signal while client is not active");
			}
		}
		// 
		// Trigger a producer-like signal, typically env
		DGRAPH_API void SendSignal(char* stream, int numBytes)
		{
			/*
			if (!envAdapterp)
				env_initialize();
			envAdapterp->send_env_msg(stream, numBytes);
			*/
		}

		DGRAPH_API int GetEnvSignal(char ip[]) {

/*			if (!envAdapterp)
				env_initialize();
			return envAdapterp->get_env_msg(ip);
			*/
			return 0;
		}

		DGRAPH_API bool ReadLocalEnvData() {
			/*
			if (!envAdapterp)
				env_initialize();
			bool b = envAdapterp->read_ipc();
			return b;
			*/
			return false;
		}

		DGRAPH_API void NativeDebugLog(const char * msg) {
			spdlog::info(msg);
		}
	}
}

