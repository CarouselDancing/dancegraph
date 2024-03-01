#pragma once

#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif


// ENV_USERNAME_MAX_SIZE MUST be bigger than 44

constexpr int ENV_SIGNAL_MAX_SIZE = 1024;
constexpr int ENV_SCENENAME_MAX_SIZE = 256;
constexpr int ENV_USERNAME_MAX_SIZE = 256;
constexpr int ENV_USERAVATARNAME_MAX_SIZE = 32;
constexpr int ENV_USERAVATARPARAMS_MAX_SIZE = 256;
constexpr int ENV_MUSICTRACK_MAX_SIZE = 256;

// May want to have these inherit some type information regarding whether they contain server-stored information or similar

enum class EnvMessageType {
    NonState,
    GlobalState,
    UserState
};


enum EnvMessageID {
    EnvMessageGenericID = -1,
    EnvSceneStateID = 4,
    EnvSceneRequestID = 5,
    EnvUserStateID = 8,
    EnvUserRequestID = 9,
    EnvMusicStateID = 12,
    EnvMusicRequestID = 13,
    EnvTestStateID = 16,
    EnvTestRequestID = 17
};

#ifdef _MSC_VER
__pragma(pack(push, 1))
#endif

template <typename T, int32_t mID>
struct EnvMessage {
    int8_t signalID = mID;
    T mBody;
    /*
    virtual EnvMessageType isStateMessage() {
        return EnvMessageType::NonState;
    }

    virtual void copy(T* dest) {
        // Do nothing. No state for these types of messages       
    }
    */

};

template <typename T, int32_t mID>
struct EnvStateGlobal {
    int8_t signalID = mID;
    T mBody;
    /*
    virtual EnvMessageType isStateMessage() {
        return EnvMessageType::GlobalState;
    }

    virtual void copy(T* dest) {
        // Copy into the global state repository
        memcpy(dest, &mBody, sizeof(mBody));
    }
    
    virtual std::string toString() {
        return std::string(fmt::format("EnvGlobal {}: {}", signalID, mBody.toString()));
    }
    */
    std::string toString() {
        return std::string("Intentionally left blank");
    }

};

template <typename T, int32_t mID>
struct EnvStateUser {
    int8_t signalID = mID;
    T mBody;    
};


struct EnvSceneStateBody {
    char sceneName[ENV_SCENENAME_MAX_SIZE]="";
    uint8_t padding[7];
};


struct SceneDataRequestBody {
    uint8_t padding[7];
};

struct EnvUserStateBody {
    uint32_t userID = 0xffff;
    // Name of the user
    
    char userName[ENV_USERNAME_MAX_SIZE]="";
    
    // Placeholder description of the user appearance (Unity prefab name, or little bundle of json?)
    char avatarType[ENV_USERAVATARNAME_MAX_SIZE] = "";

    char avatarParams[ENV_USERAVATARPARAMS_MAX_SIZE] = "";

    // Origin position of the avatar in the gameworld
    float position[3] = {0.0f,0.0f,0.0f};
    // Origin orientation of the avatar in the gameworld
    uint16_t orientation[3] = {0,0,0};
    uint8_t isActive=0;
};

struct EnvUserRequestBody {
    uint32_t userID=0xffff;
    uint8_t padding[3];
};

struct EnvMusicStateBody {
    char trackName[ENV_MUSICTRACK_MAX_SIZE]="";
    uint64_t musicTime=0;
    bool isPlaying=false;
    uint8_t padding[6]; // !!!!
};

struct MusicDataRequestBody {
    uint8_t padding[7];
};

struct GenericTestBody {
    uint8_t padding[7];
};


struct EnvTestStateBody {
    uint32_t payload=0;
    uint8_t padding[3];

};

struct TestDataRequestBody {
    uint8_t padding[7];
};

#ifdef _MSC_VER
__pragma(pack(pop))
#endif

typedef
    EnvStateGlobal <EnvSceneStateBody, EnvSceneStateID> EnvSceneState;

typedef
    EnvMessage <SceneDataRequestBody, EnvSceneRequestID> EnvSceneRequest;

typedef
    EnvStateUser <EnvUserStateBody, EnvUserStateID> EnvUserState;

typedef
    EnvMessage <EnvUserRequestBody, EnvUserRequestID> EnvUserRequest;

typedef
    EnvStateGlobal <EnvMusicStateBody, EnvMusicStateID> EnvMusicState;

typedef
    EnvMessage <MusicDataRequestBody, EnvMusicRequestID> EnvMusicRequest;

typedef
    EnvStateGlobal <EnvTestStateBody, EnvTestStateID> EnvTestState;

typedef
    EnvMessage <TestDataRequestBody, EnvTestRequestID> EnvTestRequest;

typedef EnvStateGlobal<GenericTestBody, EnvMessageGenericID> EnvMessageGeneric;

