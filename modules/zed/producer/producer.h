#pragma once

#include <array>
#include <map>
#include <string>
#include <chrono>

#define DYNALO_EXPORT_SYMBOLS
#include <dynalo/symbol_helper.hpp>
#include <dynalo/dynalo.hpp>

#include <core/lib/sig/signal_producer.h>
#include <core/lib/sig/signal_common.h>

#include "zed_common.h"

#include <sl/Camera.hpp>


namespace zed {


	constexpr int ZED_VERSION_MAJOR_MINIMUM = 4;
	constexpr int ZED_VERSION_MINOR_MINIMUM = 0;
	constexpr int ZED_VERSION_PATCH_MINIMUM = 8;



	// Slightly clunky use of std::map to get default entries
	struct defaultedRES {
		sl::RESOLUTION r = sl::RESOLUTION::HD1080;
	};

	struct defaultedFPS {
		int f = 30;
	};

	struct defaultedDEPTH_MODE {
		sl::DEPTH_MODE d = sl::DEPTH_MODE::PERFORMANCE;
	};

	struct defaultedCOORD_MODE {
		sl::COORDINATE_SYSTEM c = sl::COORDINATE_SYSTEM::RIGHT_HANDED_Z_UP_X_FWD;
	};

	struct defaultedBODYFORMAT {
		sl::BODY_FORMAT f = sl::BODY_FORMAT::BODY_34;
	};

	struct defaultedTRACKING_MODEL {
		sl::BODY_TRACKING_MODEL f = sl::BODY_TRACKING_MODEL::HUMAN_BODY_FAST;
	};


	std::map<std::string, defaultedRES> resolutionmap{ {std::string("HD2K"), defaultedRES{sl::RESOLUTION::HD2K}},
										   {std::string("1080"),   defaultedRES{sl::RESOLUTION::HD1080}},
										   {std::string("720"),    defaultedRES{sl::RESOLUTION::HD720}},
										   {std::string("VGA"),    defaultedRES{sl::RESOLUTION::VGA}} };

	std::map<int, defaultedFPS> fpsmap = { {15, defaultedFPS{15}},
									{30,  defaultedFPS{30}},
									{60,  defaultedFPS{60}},
									{100, defaultedFPS{100}} };

	std::map<std::string, defaultedDEPTH_MODE> depthmap = { { std::string("NONE"), defaultedDEPTH_MODE{sl::DEPTH_MODE::NONE} },
										{std::string("PERFORMANCE"),     defaultedDEPTH_MODE{sl::DEPTH_MODE::PERFORMANCE}},
										{std::string("QUALITY"),         defaultedDEPTH_MODE{sl::DEPTH_MODE::QUALITY}},
										{std::string("ULTRA"),           defaultedDEPTH_MODE{sl::DEPTH_MODE::ULTRA}},
										{std::string("NEURAL"),          defaultedDEPTH_MODE{sl::DEPTH_MODE::NEURAL}} };

	std::map<std::string, defaultedTRACKING_MODEL> trackingmodelmap = { { std::string("FAST"), defaultedTRACKING_MODEL{sl::BODY_TRACKING_MODEL::HUMAN_BODY_FAST} },
										{std::string("MEDIUM"),     defaultedTRACKING_MODEL{sl::BODY_TRACKING_MODEL::HUMAN_BODY_MEDIUM}},
										{std::string("ACCURATE"),          defaultedTRACKING_MODEL{sl::BODY_TRACKING_MODEL::HUMAN_BODY_ACCURATE}} };


	std::map<std::string, defaultedCOORD_MODE> coordmap = { {std::string("LEFT_HANDED_Y_UP"), defaultedCOORD_MODE{sl::COORDINATE_SYSTEM::LEFT_HANDED_Y_UP}}, // Unity
												{std::string("LEFT_HANDED_Z_UP"), defaultedCOORD_MODE{sl::COORDINATE_SYSTEM::LEFT_HANDED_Z_UP}}, // Unreal
												{std::string("RIGHT_HANDED_Y_UP"), defaultedCOORD_MODE{sl::COORDINATE_SYSTEM::RIGHT_HANDED_Y_UP}}, // OpenGL
												{std::string("RIGHT_HANDED_Z_UP"), defaultedCOORD_MODE{sl::COORDINATE_SYSTEM::RIGHT_HANDED_Z_UP}}, // ROS
												{std::string("RIGHT_HANDED_Z_UP_X_FWD"), defaultedCOORD_MODE{sl::COORDINATE_SYSTEM::RIGHT_HANDED_Z_UP_X_FWD}}, // ??
												{std::string("IMAGE"), defaultedCOORD_MODE{sl::COORDINATE_SYSTEM::IMAGE}}  // ZED SDK Default
		};
		

	// Used to decide the body format for camera input
	std::map<zed::Zed4SignalType, defaultedBODYFORMAT> bodyformatmap = {
		{zed::Zed4SignalType::Body_34_Compact, defaultedBODYFORMAT{sl::BODY_FORMAT::BODY_34}},
		{zed::Zed4SignalType::Body_38_Compact, defaultedBODYFORMAT{sl::BODY_FORMAT::BODY_38}},
		{zed::Zed4SignalType::Body_34_Full, defaultedBODYFORMAT{sl::BODY_FORMAT::BODY_34}},
		{zed::Zed4SignalType::Body_38_Full, defaultedBODYFORMAT{sl::BODY_FORMAT::BODY_38}},
		{zed::Zed4SignalType::Body_34_Keypoints, defaultedBODYFORMAT{sl::BODY_FORMAT::BODY_34}},
		{zed::Zed4SignalType::Body_38_Keypoints, defaultedBODYFORMAT{sl::BODY_FORMAT::BODY_38}},
		{zed::Zed4SignalType::Body_34_KeypointsPlus, defaultedBODYFORMAT{sl::BODY_FORMAT::BODY_34}},
		{zed::Zed4SignalType::Body_38_KeypointsPlus, defaultedBODYFORMAT{sl::BODY_FORMAT::BODY_38}},


	};

	class SensorProducer {
		
    public:

		template
			<typename T>
			ZedBodies<T>* read();


        bool init();
        void shutdown();
        int get_data(uint8_t*, sig::time_point& time);
		void read_data_from_file(std::string file_path);
		int get_current_frame();

		//sig::time_point start_time;

		SensorProducer(BufferProperties& bufProps) :
			bufferProperties(bufProps), read_counter(0) {}

	protected:

		template
		<class T>
		int process_zed_signal(uint8_t * mem);

		template
			<class T>
			int process_zed_kp_signal(uint8_t* mem);

		template
			<class T>
			int process_zed_kprot_signal(uint8_t* mem);

	protected:



        sl::Camera zed;
        BufferProperties bufferProperties;
        //sl::ObjectDetectionRuntimeParameters object_detection_params_rt;
		
		sl::BodyTrackingRuntimeParameters body_tracking_parameters_rt;
		int read_counter;

        //sl::Objects bodies;
		sl::Bodies bodies;

        sl::Plane floor_plane;
        sl::Transform reset_from_floor_plane;
        bool need_floor_plane;

		sl::Pose cam_pose;

		bool isRecording = false;

	};

	// Initialize the signal producer (alloc resources/caches, etc)
	DYNALO_EXPORT bool DYNALO_CALL SignalProducerInitialize(const sig::SignalProperties& sigProp);

	// Get signal data and the time of generation. Return number of written bytes
	DYNALO_EXPORT int DYNALO_CALL GetSignalData(uint8_t* mem, sig::time_point& sigTime);

	// Shutdown the signal producer (free resources/caches, etc)
	DYNALO_EXPORT void DYNALO_CALL SignalProducerShutdown();

}
