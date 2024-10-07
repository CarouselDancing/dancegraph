
#include <memory>

#include <sl/Camera.hpp>

#define DYNALO_EXPORT_SYMBOLS
#include <dynalo/symbol_helper.hpp>
#include <dynalo/dynalo.hpp>

#include <spdlog/spdlog.h>
#include <magic_enum/magic_enum.hpp>

#include "producer.h"

#include <modules/zed/zed_common.h>

using namespace sl;
using namespace std;

namespace zed {

	// g_SensorProducer is stored as a pointer to delay construction until we've got arguments to construct with
	// (Since the ringbuffer has no default constructor, we can't store the actual class without a lot more hassle)
	unique_ptr<SensorProducer> g_SensorProducer;


	// Takes a Position/Orientation from the SDK and copies it into a waiting chunk of memory. 
	// Until we stabilize on the data format for the IPC copy, I'm not trusting copying this
	// any other way than the dumb longhand field-by-field method.
	 
	// TODO: work out the smartest/fastest way to copy these and/or lists of these
	

	void copyVec3(vec3 & vTo, Vector3<float>& vFrom) {
		vTo.x = vFrom.tx;
		vTo.y = vFrom.ty;
		vTo.z = vFrom.tz;		
	}

	void copyVec4(quat & vTo, Vector4<float>& vFrom) {
		vTo.x = vFrom.ox;
		vTo.y = vFrom.oy;
		vTo.z = vFrom.oz;
		vTo.w = vFrom.ow;
	}

	void copyTransform(xform & tTo, Vector3<float> &posFrom, Vector4<float> &oriFrom) {
		tTo.pos.x = posFrom.tx;
		tTo.pos.y = posFrom.ty;
		tTo.pos.z = posFrom.tz;

		tTo.ori.x = oriFrom.ox;

		tTo.ori.y = oriFrom.oy;
		tTo.ori.z = oriFrom.oz;
		tTo.ori.w = oriFrom.ow;

	}

	void copyVec4ToVec3(vec3& tTo, Vector4<float> & oriFrom) {
		if (oriFrom.ow < 0) {
			tTo.x = -oriFrom.ox;
			tTo.y = -oriFrom.oy;
			tTo.z = -oriFrom.oz;
		}
		else {
			tTo.x = oriFrom.ox;
			tTo.y = oriFrom.oy;
			tTo.z = oriFrom.oz;
		}
	}


	void copyTransformCompact(xform_compact& tTo, Vector3<float> &posFrom, Vector4<float> &oriFrom) {
		tTo.pos.x = posFrom.tx;
		tTo.pos.y = posFrom.ty;
		tTo.pos.z = posFrom.tz;

		copyVec4ToVec3(tTo.ori, oriFrom);

	}

	void copyVec4ToVec3Quant(quant_quat& vTo, Vector4<float>& vFrom) {
		if (vFrom.ow < 0) {
			vTo.x = (uint16_t)(-vFrom.ox * 32767);
			vTo.y = (uint16_t)(-vFrom.oy * 32767);
			vTo.z = (uint16_t)(-vFrom.oz * 32767);

		}
		else {
			vTo.x = (uint16_t)(vFrom.ox * 32767);
			vTo.y = (uint16_t)(vFrom.oy * 32767);
			vTo.z = (uint16_t)(vFrom.oz * 32767);
		}
	}

	void copyTransformQuant(xform_quant& tTo, Vector3<float>& posFrom, Vector4<float>& oriFrom) {
		tTo.pos.x = posFrom.tx;
		tTo.pos.y = posFrom.ty;
		tTo.pos.z = posFrom.tz;

		copyVec4ToVec3Quant(tTo.ori, oriFrom);
	}



	// Initialize the signal producer (alloc resources/caches, etc)
	DYNALO_EXPORT bool DYNALO_CALL SignalProducerInitialize(const sig::SignalProperties& sigProp)

	{	
		
		spdlog::set_default_logger(sigProp.logger);
		spdlog::info(  "SignalProducerInitialize Called");
		BufferProperties bProp = BufferProperties();
		spdlog::info("zedcam_producer.cpp Json properties: {}", sigProp.jsonConfig);
		bProp.populate_from_json(json::parse(sigProp.jsonConfig)); // Pass in the name of a config file if we want, otherwise it finds a default
		
		g_SensorProducer = unique_ptr<SensorProducer>(new SensorProducer(bProp));
		spdlog::info(  "SignalProducerInitialize Ended");

		return g_SensorProducer->init();

	}

	// Get signal data and the time of generation. Return number of written bytes
	DYNALO_EXPORT int DYNALO_CALL GetSignalData(uint8_t* mem, sig::time_point& time)
	{
		return g_SensorProducer->get_data(mem, time);
	}

	// Shutdown the signal producer (free resources/caches, etc)
	DYNALO_EXPORT void DYNALO_CALL SignalProducerShutdown()
	{
		g_SensorProducer->shutdown();
	}


	bool sdk_version_quit(int major, int minor, int patch) {
		spdlog::critical("Zed SDK Version mismatch, requires version {}.{}.{} or later, found {}.{}.{}", ZED_VERSION_MAJOR_MINIMUM, ZED_VERSION_MINOR_MINIMUM, ZED_VERSION_PATCH_MINIMUM, major, minor, patch);
		return false;
	}

	bool check_AI_model(sl::BODY_FORMAT format, sl::BODY_TRACKING_MODEL model) {

		AI_MODELS aim;

		if (format == sl::BODY_FORMAT::BODY_38) {
			switch (model) {
			case sl::BODY_TRACKING_MODEL::HUMAN_BODY_FAST:
				aim = sl::AI_MODELS::HUMAN_BODY_38_FAST_DETECTION; break;

			case sl::BODY_TRACKING_MODEL::HUMAN_BODY_MEDIUM:
				aim = sl::AI_MODELS::HUMAN_BODY_38_MEDIUM_DETECTION; break;

			case sl::BODY_TRACKING_MODEL::HUMAN_BODY_ACCURATE:
				aim = sl::AI_MODELS::HUMAN_BODY_38_ACCURATE_DETECTION; break;
			}
		}
		else if (format == sl::BODY_FORMAT::BODY_34) {
			switch (model) {
			case sl::BODY_TRACKING_MODEL::HUMAN_BODY_FAST:
				aim = sl::AI_MODELS::HUMAN_BODY_FAST_DETECTION; break;
			case sl::BODY_TRACKING_MODEL::HUMAN_BODY_MEDIUM:
				aim = sl::AI_MODELS::HUMAN_BODY_MEDIUM_DETECTION; break;
			case sl::BODY_TRACKING_MODEL::HUMAN_BODY_ACCURATE:
				aim = sl::AI_MODELS::HUMAN_BODY_ACCURATE_DETECTION; break;
			}
		}
		else {
			spdlog::warn("Can't find correct sl::BODY_FORMAT for AI model detection: {}", (int)format);
			return false;
		}

		/*
		// For some reason, sl::checkAIModelStatus doesn't seem to work well.
	
		spdlog::info("Checking AI Model status for model type {} / model {}, format {}", (int)aim, (int)model, (int)format);

		sl::AI_Model_status stat = sl::checkAIModelStatus(aim);
		spdlog::info("AI Model Status: {} / {}", stat.downloaded, stat.optimized);
		return stat.optimized;
		*/
		return true;
	}


	bool SensorProducer::init() {

		// Switch on the camera and ready it


		spdlog::info("Camera about to start being initialized");

		InitParameters init_parameters;

		init_parameters.depth_mode = depthmap.at(bufferProperties.depth).d;
		init_parameters.camera_fps = fpsmap.at(bufferProperties.fps).f;
		init_parameters.camera_resolution = resolutionmap.at(bufferProperties.resolution).r;
		init_parameters.coordinate_system = coordmap.at(bufferProperties.coordsystem).c;

		if (bufferProperties.minDepthDistance > 0) {
			init_parameters.depth_minimum_distance = bufferProperties.minDepthDistance;
		}

		if (bufferProperties.maxDepthDistance > 0) {
			init_parameters.depth_maximum_distance = bufferProperties.maxDepthDistance;
		}

		
		if (bufferProperties.svoInput.size() > 0) {
			init_parameters.input.setFromSVOFile(bufferProperties.svoInput.c_str());
			spdlog::info("Replaying SVO file data from {} instead of live camera", bufferProperties.svoInput);
		}

		int major, minor, patch;
		
		sl::Camera::getSDKVersion(major, minor, patch);


		long int vernum = 1000000 * major + 1000 * minor + patch;
		long int minvernum = 1000000 * ZED_VERSION_MAJOR_MINIMUM + 1000 * ZED_VERSION_MINOR_MINIMUM + ZED_VERSION_PATCH_MINIMUM;

		if (vernum < minvernum)
			return sdk_version_quit(major, minor, patch);

		ERROR_CODE zed_err;

		zed_err = zed.open(init_parameters);

		if (zed_err != ERROR_CODE::SUCCESS) {
			spdlog::critical(  "Error opening cam: {}", magic_enum::enum_name(zed_err));
			zed.close();
			return false;
		}

		PositionalTrackingParameters positional_tracking_parameters;
		positional_tracking_parameters.set_as_static = bufferProperties.staticCamera;
		zed_err = zed.enablePositionalTracking(positional_tracking_parameters);

		if (zed_err != ERROR_CODE::SUCCESS) {
			spdlog::warn(  "Warning: Positional Tracking enabling problem: {}", magic_enum::enum_name(zed_err));
		}

		RecordingParameters rp;
		rp.compression_mode = SVO_COMPRESSION_MODE::H264;

		if (bufferProperties.videoFile.size() > 0) {
			stringstream ss;
			auto t = time(nullptr);
			auto lt = *localtime(&t);

			ss << bufferProperties.videoFile << "_" << std::put_time(&lt, "%Y%m%d_%H%M%S") << ".svo";
			rp.video_filename = ss.str().c_str();

			isRecording = true;

			//spdlog::info(  "Recording to {}", rp.video_filename);

			zed.enableRecording(rp);
		}

		BodyTrackingParameters body_tracker_params;


		sl::BODY_FORMAT body_format = bodyformatmap.at(bufferProperties.bodySignalType).f;

		


		body_tracker_params.enable_tracking = true; // track people across images flow
		body_tracker_params.enable_body_fitting = true; // smooth skeletons moves
		body_tracker_params.body_format = body_format;		
		body_tracker_params.detection_model = trackingmodelmap.at(bufferProperties.trackingModel).f;
		body_tracker_params.allow_reduced_precision_inference = bufferProperties.allowReducedPrecision;

		// For some reason the ZED SDK is returning false negatives
		
		if (!check_AI_model(body_tracker_params.body_format, body_tracker_params.detection_model)) {

			spdlog::critical("Warning, ZED SDK body tracking model is not optimized. Please refer to ZED SDK documentation to optimize the models");
			
			zed.close();
			return false;
		}
		
		


		zed_err = zed.enableBodyTracking(body_tracker_params);
		if (zed_err != ERROR_CODE::SUCCESS) {
			spdlog::critical(  "Error enabling ZED object detection:{}", magic_enum::enum_name(zed_err));
			zed.close();
			return false;
		}

		const int& output_signal_bonecount = zed::signal_bonecount.at(bufferProperties.bodySignalType).first;
	
		// Configure object detection runtime parameters

		body_tracking_parameters_rt.detection_confidence_threshold = bufferProperties.confidence;
		body_tracking_parameters_rt.skeleton_smoothing = bufferProperties.skeletonSmoothing;
		body_tracking_parameters_rt.minimum_keypoints_threshold = bufferProperties.keypointsThreshold;
		
//		object_detection_params_rt.detection_confidence_threshold = bufferProperties.confidence;
		
		need_floor_plane = positional_tracking_parameters.set_as_static;

		cam_pose.pose_data.setIdentity();

		//start_time = sig::time_now();		

		return true;
	}
	
	// How does the calling function know whether we have enough memory here?



	template
		<class T>
		int SensorProducer::process_zed_signal(uint8_t* mem) {

		int bodiesSent = min((int)bodies.body_list.size(), bufferProperties.maxBodyCount);

		ZedBodies<T>* zedBodies = (ZedBodies<T> *) mem;
		zedBodies->num_skeletons = bodiesSent;

		sl::Timestamp timestamp = zed.getTimestamp(TIME_REFERENCE::IMAGE);

		const int & output_signal_bonecount = zed::signal_bonecount.at(bufferProperties.bodySignalType).first;

		auto& usefulbones = useful_bone_indices[bufferProperties.bodySignalType];

		for (int i = 0; i < bodiesSent; i++) {
			auto body = bodies.body_list[i];
			zedBodies->skeletons[i].id = body.id;

			copyTransformQuant(zedBodies->skeletons[i].root_transform, body.position, body.global_root_orientation);


			for (int j = 0; j < output_signal_bonecount; j++) {
				copyVec4ToVec3Quant(zedBodies->skeletons[i].bone_rotations[j], body.local_orientation_per_joint[usefulbones[j]]);

			}
		}

		int sent_data = ZedBodies<T>::size(bodiesSent);
		return sent_data;
	}



	template
		<class T>
		int SensorProducer::process_zed_kp_signal(uint8_t* mem) {

		int bodiesSent = min((int)bodies.body_list.size(), bufferProperties.maxBodyCount);
		
		ZedBodies<T>* zedBodies = (ZedBodies<T> *) mem;
		zedBodies->num_skeletons = bodiesSent;

		sl::Timestamp timestamp = zed.getTimestamp(TIME_REFERENCE::IMAGE);

		const int& output_signal_kpcount = zed::signal_bonecount.at(bufferProperties.bodySignalType).second;
		//const int& output_signal_oricount = zed::signal_bonecount.at(bufferProperties.bodySignalType).first;

		for (int i = 0; i < bodiesSent; i++) {
			auto body = bodies.body_list[i];
			zedBodies->skeletons[i].id = body.id;

			copyTransformQuant(zedBodies->skeletons[i].root_transform, body.position, body.global_root_orientation);			
			for (int j = 0; j < output_signal_kpcount; j++) {
				copyVec3(zedBodies->skeletons[i].bone_keypoints[j], body.keypoint[j]);
			}

/*
			for (int j = 0; j < output_signal_oricount; j++) {
				copyVec4ToVec3Quant(zedBodies->skeletons[i].bone_rotations[j], body.local_orientation_per_joint[j]);
			}
*/
		}

		int sent_data = ZedBodies<T>::size(bodiesSent);
		return sent_data;
	}


	template
		<class T>
		int SensorProducer::process_zed_kprot_signal(uint8_t* mem) {

		int bodiesSent = min((int)bodies.body_list.size(), bufferProperties.maxBodyCount);

		ZedBodies<T>* zedBodies = (ZedBodies<T> *) mem;
		zedBodies->num_skeletons = bodiesSent;

		auto& usefulbones = useful_bone_indices[bufferProperties.bodySignalType];

		sl::Timestamp timestamp = zed.getTimestamp(TIME_REFERENCE::IMAGE);

		const int& output_signal_kpcount = zed::signal_bonecount.at(bufferProperties.bodySignalType).second;
		auto & output_signal_oricount = zed::signal_bonecount.at(bufferProperties.bodySignalType).first;

		for (int i = 0; i < bodiesSent; i++) {
			auto body = bodies.body_list[i];
			zedBodies->skeletons[i].id = body.id;

			copyTransformQuant(zedBodies->skeletons[i].root_transform, body.position, body.global_root_orientation);
			for (int j = 0; j < output_signal_kpcount; j++) {
				copyVec3(zedBodies->skeletons[i].bone_keypoints[j], body.keypoint[j]);
			}
			
			for (int j = 0; j < output_signal_oricount; j++) {
				copyVec4ToVec3Quant(zedBodies->skeletons[i].bone_rotations[j], body.local_orientation_per_joint[usefulbones[j]]);
			}
			
		}

		int sent_data = ZedBodies<T>::size(bodiesSent);
		return sent_data;
	}

	template
		<class Z>
		void populate_deltas(uint8_t * mem, 

		const sig::time_point& call_time,
		const sig::time_point& grab_time,
		const sig::time_point& track_time,
		const sig::time_point& exit_time) {

		ZedBodies<Z>* zb = (ZedBodies<Z> *) mem;

		zb->grab_delta = std::chrono::duration_cast<std::chrono::microseconds>(grab_time - call_time).count();
		zb->track_delta = std::chrono::duration_cast<std::chrono::microseconds>(track_time - grab_time).count();
		zb->process_delta = std::chrono::duration_cast<std::chrono::microseconds>(exit_time - track_time).count();

	}


	int SensorProducer::get_data(uint8_t* mem, sig::time_point& time) {

		static int framecounter = 0;

		sig::time_point call_time = sig::time_now();
		

		ERROR_CODE zed_err = zed.grab();
		if (zed_err != ERROR_CODE::SUCCESS) {
			spdlog::warn("Error grabbing zed: {}", magic_enum::enum_name(zed_err));
			return 0;
		}
		else {
			if (need_floor_plane) {
				if (zed.findFloorPlane(floor_plane, reset_from_floor_plane) == ERROR_CODE::SUCCESS) {
					need_floor_plane = false;
				}
			}
		}

		sig::time_point grab_time = sig::time_now();

		zed_err = zed.retrieveBodies(bodies, body_tracking_parameters_rt);
		if (zed_err != ERROR_CODE::SUCCESS) {
			spdlog::warn("Error retrieving zed bodies: {}", magic_enum::enum_name(zed_err));

			return 0;
		}

		sig::time_point track_time = sig::time_now();

		framecounter += 1;

		int retval;

		switch (bufferProperties.bodySignalType) {

		case Body_38_Keypoints:
			retval = process_zed_kp_signal<ZedSkeletonKP_38>(mem);			
			populate_deltas<ZedSkeletonKP_38>(mem, call_time, grab_time, track_time, sig::time_now());
			break;
		case Body_34_Keypoints:
			retval = process_zed_kp_signal<ZedSkeletonKP_34>(mem);
			populate_deltas <ZedSkeletonKP_34> (mem, call_time, grab_time, track_time, sig::time_now());
			break;
		case Body_38_Compact:
			retval = process_zed_signal<ZedSkeletonCompact_38>(mem);
			populate_deltas<ZedSkeletonCompact_38>(mem, call_time, grab_time, track_time, sig::time_now());
			break;
		case Body_34_Full:
			retval = process_zed_signal<ZedSkeletonFull_34>(mem);
			populate_deltas<ZedSkeletonFull_34>(mem, call_time, grab_time, track_time, sig::time_now());
			break;
		case Body_38_Full:
			retval = process_zed_signal<ZedSkeletonFull_38>(mem);
			populate_deltas<ZedSkeletonFull_38>(mem, call_time, grab_time, track_time, sig::time_now());
			break;
		case Body_34_KeypointsPlus:
			retval = process_zed_kprot_signal<ZedSkeletonKPRot_34>(mem);
			populate_deltas<ZedSkeletonKPRot_34>(mem, call_time, grab_time, track_time, sig::time_now());
			break;
		case Body_38_KeypointsPlus:
			retval = process_zed_kprot_signal<ZedSkeletonKPRot_38>(mem); 
			populate_deltas<ZedSkeletonKPRot_38>(mem, call_time, grab_time, track_time, sig::time_now());
			break;
		case Body_34_Compact:default:
			retval = process_zed_signal<ZedSkeletonCompact_34>(mem);
			populate_deltas<ZedSkeletonCompact_34>(mem, call_time, grab_time, track_time, sig::time_now());
			break;

		}

		sig::time_point exit_time = sig::time_now();

		//populate_deltas(mem, call_time, grab_time, track_time, exit_time);

		if (bufferProperties.timeTracking) {			
			long int grab_delta = std::chrono::duration_cast<std::chrono::microseconds>(call_time - grab_time).count();
			long int track_delta = std::chrono::duration_cast<std::chrono::microseconds>(track_time - grab_time).count();
			long int process_delta = std::chrono::duration_cast<std::chrono::microseconds>(exit_time - grab_time).count();
			spdlog::debug("{}: Zed4 producer: grab: {}, track: {}, process: {}", framecounter, grab_delta, track_delta, process_delta);
			
		}
		


		time = grab_time;

		std::stringstream ss;

		ss = std::stringstream("");
		for (int i = 0; i < retval; i++) {
			ss << (int)(*((uint8_t*)& mem + i)) << " ";
		}

		return retval;
	}

	void SensorProducer::shutdown() {
		bodies.body_list.clear();

		if (isRecording)
			zed.disableRecording();

		zed.disableObjectDetection();
		zed.disablePositionalTracking();
		zed.close();

	}

}