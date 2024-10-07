// Producer that spits out a config-determined state packet on connection

#include "state_provider.h"
#include <spdlog/spdlog.h>
#include <sig/signal_common.h>

using namespace std;

using json = nlohmann::json;

/*
  struct EnvUserStateBody {
	  int32_t userID = -1;

	  char userName[ENV_USERNAME_MAX_SIZE] = "";

	  // Placeholder description of the user appearance (Unity prefab name, or little bundle of json?)
	  char avatarType[ENV_USERAVATARNAME_MAX_SIZE] = "";

	  char avatarParams[ENV_USERAVATARPARAMS_MAX_SIZE] = "";

	  // Origin position of the avatar in the gameworld
	  float position[3] = { 0.0f,0.0f,0.0f };
	  // Origin orientation of the avatar in the gameworld
	  uint16_t orientation[3] = { 0,0,0 };

	  uint8_t isActive = 0;
	  uint8_t clientType = 0; // A client-sided field to denote the type of avatarr 0 = human, 1 = demo bot, etc
  };
  */

namespace dll {
#define min(a, b) ((a) < (b)) ? (a) : (b)

	//resolution = opts.at("zed/v2.1").at("zedResolution");
	unique_ptr<UserStateProvider> g_UserStateProvider;
	
	void UserStateProvider::populate_from_json(json jOpts) {
		spdlog::debug("Populating local copy of EnvUserState from json");
		


		std::memset((void*)&userState, 0, sizeof(EnvUserState));

		spdlog::debug("User State ID is {}", userState.signalID);
		userState.mBody.isActive = 1; // Default properly

		try {
			std::string userName = jOpts.at("userName");
			memcpy(userState.mBody.userName, userName.c_str(), min(userName.size(), ENV_USERNAME_MAX_SIZE));
		}
		catch (json::exception e) {}

		
		try {
			
			std::string avatarType = jOpts.at("avatarType");			
			memcpy(userState.mBody.avatarType, avatarType.c_str(), min(avatarType.size(), ENV_USERAVATARNAME_MAX_SIZE));
		}
		catch (json::exception e) {}


		try {
			std::string avatarParams = jOpts.at("avatarParams");			
			memcpy(userState.mBody.avatarParams, avatarParams.c_str(), min(avatarParams.size(), ENV_USERAVATARPARAMS_MAX_SIZE));
		}
		catch (json::exception e) {}

		try {
			std::vector<float> pos = jOpts.at("position");
			std::vector<uint16_t> ori = jOpts.at("orientation");
			for (int i = 0; i < 3; i++) {
				userState.mBody.position[i] = pos[i];
				userState.mBody.orientation[i] = ori[i];
			}
		}
		catch (json::exception e) {}

		//g_UserStateProvider->userState.isActive = jOpts.at("env/v1.0").at("isActive");
		try {
			userState.mBody.clientType = jOpts.at("clientType");
		}
		catch (json::exception e) {}

		try {
			timeDelay = jOpts.at("timeDelay");
		}
		catch (json::exception e) {}

		userState.signalID = EnvUserStateID;

		spdlog::debug("Populated user with username {}, avatarType {}, and clientType {}", userState.mBody.userName, userState.mBody.avatarType, userState.mBody.clientType );

	}

	int UserStateProvider::send_user_data(uint8_t * mem, sig::time_point& time) {
		
		if (firstTime) {
			start_time = sig::time_now();
			firstTime = false;
		}
		

		if (sent == false) {
			
			time = sig::time_now();
			float tdiff = std::chrono::duration_cast<std::chrono::seconds> (time - start_time).count();
			if (tdiff < timeDelay) {
				return -1;
			}

			spdlog::info("Sending user Environment state to server");

			memcpy(mem, (uint8_t *) &userState, sizeof(EnvUserState));


			sent = true;

			std::stringstream ss;
			EnvUserState* eus = (EnvUserState*)mem;
			ss << std::hex;
			for (int i = 0; i < sizeof(EnvUserState); i++) {
				ss << (int)*(mem + i) << " ";
			}
			spdlog::debug("Producer sends: {}", ss.str());


			return sizeof(EnvUserState);
		}
		else
			return -1;
	}


	// Initialize the signal producer (alloc resources/caches, etc)
	DYNALO_EXPORT bool DYNALO_CALL SignalProducerInitialize(const sig::SignalProperties& sigProp)
	{
		spdlog::set_default_logger(sigProp.logger);
		spdlog::debug("Initializing EnvUserState producer");
		g_UserStateProvider = unique_ptr<UserStateProvider>(new UserStateProvider);

		nlohmann::json jOpts = json::parse(sigProp.jsonConfig);
		g_UserStateProvider->populate_from_json(jOpts);

		g_UserStateProvider->sent = false;

		return true;
	}

	// Get signal data and the time of generation. Return number of written bytes
	DYNALO_EXPORT int DYNALO_CALL GetSignalData(uint8_t* mem, sig::time_point& time)
	{		
		return g_UserStateProvider->send_user_data(mem, time);
	}

	// Shutdown the signal producer (free resources/caches, etc)
	DYNALO_EXPORT void DYNALO_CALL SignalProducerShutdown()
	{

	}
}