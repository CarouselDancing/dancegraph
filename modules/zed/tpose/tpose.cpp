// Zedcam producer for spitting out a completely zeroed set of body orientations, in order to investigate what the default position of the zedcam is

#include <fstream>
#include <iostream>
#include <memory>
#include <cstring>

#include <math.h>

//#include <vector>

#include <modules/zed/zed_common.h>
#include "tpose.h"

using namespace std;
using namespace zed;

namespace dll {



	template
		<class T> 
		int populate_tpose(ZedBodies<T>* nullbodies, int size, int current_grab, int current_track, int current_processing) {
		nullbodies->num_skeletons = 1;
		//nullbodies->elapsed = current_elapsed;
		nullbodies->grab_delta = current_grab;
		nullbodies->track_delta = current_track;
		nullbodies->process_delta = current_processing;
		nullbodies->skeletons[0].id = 0;
		nullbodies->skeletons[0].root_transform = zed::xform_quant{ (0, 0, 0), (0, 0, 0) };
		for (int i = 0; i < size; i++) {
			nullbodies->skeletons[0].bone_rotations[i] = zed::quant_quat{ 0, 0, 0 };
		}
		return ZedBodies<T>::size(nullbodies->num_skeletons);
	}


	template
		<class T>
		int populate_tposeKP(ZedBodies<T>* nullbodies, int size, int current_grab, int current_track, int current_processing) {
		nullbodies->num_skeletons = 1;

		nullbodies->skeletons[0].id = 0;
		nullbodies->skeletons[0].root_transform = zed::xform_quant{ (0, 0, 0), (0, 0, 0) };
		//for (int i = 0; i < size; i++) {
			//nullbodies->skeletons[0].bone_rotations[i] = zed::quant_quat{ 0, 0, 0 };
		//}
		nullbodies->grab_delta = current_grab;
		nullbodies->track_delta = current_track;
		nullbodies->process_delta = current_processing;

		return ZedBodies<T>::size(nullbodies->num_skeletons);
	}


	template
		<class T>
		int populate_tposeKPRot(ZedBodies<T>* nullbodies, int size, int fullsize, int current_grab, int current_track, int current_processing) {
		nullbodies->num_skeletons = 1;
		nullbodies->skeletons[0].id = 0;
		nullbodies->skeletons[0].root_transform = zed::xform_quant{ (0, 0, 0), (0, 0, 0) };

		nullbodies->grab_delta = current_grab;
		nullbodies->track_delta = current_track;
		nullbodies->process_delta = current_processing;

		for (int i = 0; i < size; i++) {
			nullbodies->skeletons[0].bone_rotations[i] = zed::quant_quat{ 0, 0, 0 };
		}

		for (int i = 0; i < fullsize; i++) {
			nullbodies->skeletons[0].bone_keypoints[i] = SDK_TO_UNITY_SCALE * T::tpose_position(i);
		}

		return ZedBodies<T>::size(nullbodies->num_skeletons);
	}


	//std::unique_ptr<ZedNullUndumper> g_zedUndumper;
	BufferProperties bProp;
	chrono::time_point<chrono::system_clock> start_time;
	std::chrono::system_clock::time_point previous_elapsed;
	long int tick_time;
	Zed4SignalType sigType;

	// Initialize the signal producer (alloc resources/caches, etc)
	DYNALO_EXPORT bool DYNALO_CALL SignalProducerInitialize(const sig::SignalProperties& sigProp)
	{
		spdlog::set_default_logger(sigProp.logger);
		sigType = Zed4SignalType::Body_34_Compact;

		bProp = BufferProperties();
		bProp.populate_from_json(json::parse(sigProp.jsonConfig)); // Pass in the name of a config file if we want, otherwise it finds a default
		start_time = std::chrono::system_clock::now();

		tick_time = (long int)((float)1000000 / (float)bProp.fps);

		sigType = bProp.bodySignalType;
		return true;
	}

	// Get signal data and the time of generation. Return number of written bytes
	DYNALO_EXPORT int DYNALO_CALL GetSignalData(uint8_t* mem, sig::time_point& time)
	{
		static int framecount = 0;
		static int prevfps = 0;

		auto now_time = std::chrono::system_clock::now();
		time = now_time;
		

		auto current_elapsed = std::chrono::system_clock::now();

		if (std::chrono::duration_cast<std::chrono::microseconds>(current_elapsed - previous_elapsed).count() < tick_time)
			return -1;

		previous_elapsed = current_elapsed;

		int bonecount = zed::signal_bonecount[sigType].first;
		int fullbonecount = zed::signal_bonecount[sigType].second;
		// sigh
		switch (sigType) {
		case Body_38_Compact:
			return populate_tpose((ZedBodies<ZedSkeletonCompact_38> *)mem, bonecount, 0, 0, 0); break;
		case Body_34_Full:
			return populate_tpose((ZedBodies<ZedSkeletonFull_34> *)mem, bonecount, 0, 0, 0); break;
		case Body_38_Full:
			return populate_tpose((ZedBodies<ZedSkeletonFull_38> *)mem, bonecount, 0,0,0); break;
		case Body_34_Keypoints:
			return populate_tposeKP((ZedBodies<ZedSkeletonKP_34> *)mem, bonecount, 0,0,0); break;
		case Body_38_Keypoints:
			return populate_tposeKP((ZedBodies<ZedSkeletonKP_38> *)mem, bonecount, 0,0,0); break;
		case Body_34_KeypointsPlus:
			return populate_tposeKPRot((ZedBodies<ZedSkeletonKPRot_34> *)mem, bonecount, fullbonecount, 0, 0, 0); break;
		case Body_38_KeypointsPlus:
			return populate_tposeKPRot((ZedBodies<ZedSkeletonKPRot_38> *)mem, bonecount, fullbonecount, 0, 0, 0); break;


		case Body_34_Compact: default:
			return populate_tpose((ZedBodies<ZedSkeletonCompact_34> *)mem, bonecount, 0, 0, 0); break;

		}
		//return ZedBodies::size(sigType, nullbodies->num_skeletons);
		return -1;
	};


	// Shutdown the signal producer (free resources/caches, etc)
	DYNALO_EXPORT void DYNALO_CALL SignalProducerShutdown()
	{
	}
}