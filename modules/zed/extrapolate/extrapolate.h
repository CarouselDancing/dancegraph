#pragma once

//#include <array>
#include <map>
#include <string>
#include <chrono>

#define DYNALO_EXPORT_SYMBOLS
#include <dynalo/symbol_helper.hpp>
#include <dynalo/dynalo.hpp>

#include <sig/signal_transformer.h>
#include <sig/signal_common.h>
#include <sig/signal_history.h>

#include <core/net/channel.h>

#include "zed_common.h"

// This should be managing the output rate

namespace zed {
	
	constexpr int EXTRAPOLATE_MAX_QUEUE_SIZE = 10;

	class SignalTransformer {
	public: 		
		bool initialize_signal(const sig::SignalProperties& sigProp, const sig::SignalConsumerRuntimeConfig& cfg);
		void shutdown_signal();
	};

	class SensorTransformer : public SignalTransformer {
		
    public:

		template
			<typename Z>
			void lerp_rotations(Z& output, const Z &in_skel_to, const Z &in_skel_from, float scale_factor) {
			// Takes the previous two frames of rotation data, and extrapolates to get a new frame of data, populating the whole skeleton struct
			output.id = in_skel_to.id;			
			//output.root_transform.pos = lerp(in_skel_from.root_transform.pos, in_skel_to.root_transform.pos, scale_factor);
			//output.root_transform.ori = slerp(in_skel_from.root_transform.ori, in_skel_to.root_transform.ori, scale_factor);

			for (int i = 0; i < Z::bones_transmitted(); i++) {
				output.bone_rotations[i] = slerp(in_skel_from.bone_rotations[i], in_skel_to.bone_rotations[i], scale_factor);
				/*if (i == 0)
					spdlog::info("Test for bone {}: {} -> {} ({}) => {}, {}",
						i, in_skel_from.bone_rotations[i].str(), in_skel_to.bone_rotations[i].str(), scale_factor, output.bone_rotations[i].str(),
						slerp(in_skel_from.bone_rotations[i], in_skel_to.bone_rotations[i], scale_factor).str());
				*/
			}
			output.root_transform.pos = lerp(in_skel_from.root_transform.pos, in_skel_to.root_transform.pos, scale_factor);
			output.root_transform.ori = slerp(in_skel_from.root_transform.ori, in_skel_to.root_transform.ori, scale_factor);
		}

		template
		<class Z>
		int simple_passthrough(ZedBodies<Z>& out_bodies, const ZedBodies<Z>& in_bodies_to, const ZedBodies<Z>& in_bodies_from, float scale_factor, int packetId = -1) {
			spdlog::debug("Dumb passthrough transformer");
			int signal_size = ZedBodies<Z>::size(in_bodies_to.num_skeletons);
			/*
			std::stringstream ss;
			ss = std::stringstream("");
			for (int i = 0; i < signal_size; i++) {
				ss << (int)(*((uint8_t*)&in_bodies_to + i)) << " ";
			}
			spdlog::debug("Precopy queue for {} is: {}", packetId, ss.str());
			*/

			memcpy((void*)&out_bodies, (void *) &in_bodies_to, signal_size);
			/*
			ss = std::stringstream("");
			for (int i = 0; i < signal_size; i++) {
				ss << (int)(*((uint8_t*)&out_bodies + i)) << " ";
			}
			spdlog::debug("Postcopy for pkt {} is: {}", packetId, ss.str());
			*/
			return signal_size;
		}

		template
			<typename Z>
			int lerp_skeletons(ZedBodies<Z> &out_bodies, const ZedBodies<Z> &in_bodies_to, const ZedBodies<Z> &in_bodies_from, float scale_factor) {
			
			out_bodies.num_skeletons = in_bodies_to.num_skeletons;
			//out_bodies.size = in_bodies_to.size;
			out_bodies.grab_delta = 0;
			out_bodies.track_delta = 0;
			out_bodies.process_delta = 0;

			// First order of business is to get the Skeletons that match.
			for (int i = 0; i < in_bodies_to.num_skeletons; i++) {
				bool skel_lerped = false;
				for (int j = 0; j < in_bodies_from.num_skeletons; j++) {
					// We have a match.
					if (in_bodies_from.skeletons[i].id == in_bodies_from.skeletons[j].id) {
						lerp_rotations(out_bodies.skeletons[i], in_bodies_to.skeletons[i], in_bodies_from.skeletons[j], scale_factor);

						skel_lerped = true;
						break;
					}
				}

				// If the last frame has a skeleton but the penultimate one doesn't, then we can't lerp, so just copy over the data as-is without lerping
				if (!skel_lerped) {
					out_bodies.skeletons[i] = in_bodies_to.skeletons[i];
				}
			
			}

			static int lerp_skeleton_counter = 0;
			/*
			for (int i = 0; i < out_bodies.num_skeletons; i++) {
				spdlog::trace("lerp_skeletons: {}, root transform lerp (sf {}, {}->{}->{}, bone 1 = {}->{}->{}", i, scale_factor,
					in_bodies_from.skeletons[i].root_transform.pos.str(), in_bodies_to.skeletons[i].root_transform.pos.str(), out_bodies.skeletons[i].root_transform.pos.str(),
					in_bodies_from.skeletons[i].bone_keypoints[1].str(), in_bodies_to.skeletons[i].bone_keypoints[1].str(), out_bodies.skeletons[i].bone_keypoints[1].str());
			}
			*/


			return ZedBodies<Z>::size(in_bodies_to.num_skeletons);		
		}; 

		template
			<typename Z>
			int extrapolate_rotation_data(uint8_t * mem, sig::time_point& time, sig::SignalMemory & sigMem) {

			sig::SignalHistory * envHist = sigMem.get_history("env/v1.0");
			sig::SignalHistory * zedHist = sigMem.get_history("zed/v2.1");

			if (zedHist->queue_size() == 0) // If we have no entries, there's nothing to do
			{

				return -1;
			}
			else if (zedHist->queue_size() == 1) // If there's one entry, then just use the last received data
			{

				//int signal_size = zedHist->max_signal_size;
				std::shared_ptr<char[]> memsrc = zedHist->getdata(zedHist->queue_size() - 1);				

				std::shared_ptr<sig::SignalMetadata> meta = zedHist->getmetadata(zedHist->queue_size() - 1);
				ZedBodies<Z>* zm = (ZedBodies<Z> *)memsrc.get();
				int signal_size = ZedBodies<Z>::size(zm->num_skeletons);
				
				assert(memsrc != nullptr);
				memcpy(mem, (void *) memsrc.get(), signal_size);
				spdlog::debug("Outputting a single queue entry for pkt", meta->packetId);
				return signal_size;
			}
			else { // Queue with more than one entry
				//spdlog::warn("Warning, unimplemented interpolation, falling back to prior frame");
				
				std::shared_ptr<sig::SignalMetadata> lastMeta = zedHist->getmetadata(zedHist->queue_size() - 1);
				std::shared_ptr<sig::SignalMetadata> prevMeta = zedHist->getmetadata(zedHist->queue_size() - 2);				

				std::chrono::system_clock::time_point time_now = std::chrono::system_clock::now();
				long long prevTime = std::chrono::duration_cast<std::chrono::microseconds> (prevMeta->acquisitionTime.time_since_epoch()).count();
				long long lastTime = std::chrono::duration_cast<std::chrono::microseconds> (lastMeta->acquisitionTime.time_since_epoch()).count();
				long long curTime = std::chrono::duration_cast<std::chrono::microseconds> (time_now.time_since_epoch()).count();

				int prevwindow = std::chrono::duration_cast<std::chrono::microseconds> (time_now - lastMeta->acquisitionTime).count();
				int curwindow = std::chrono::duration_cast<std::chrono::microseconds> (lastMeta->acquisitionTime - prevMeta->acquisitionTime).count();

				//spdlog::debug("Previous timestamps: {}, {}, {} -> ({}, {})", prevTime, lastTime, curTime, prevwindow,  curwindow);

				float time_factor = (float)(curwindow + prevwindow) / (float)prevwindow;

				ZedBodies<Z>* zbLast = (ZedBodies<Z> *) zedHist->getdata(zedHist->queue_size() - 1).get();
				ZedBodies<Z>* zbPenult = (ZedBodies<Z> *) zedHist->getdata(zedHist->queue_size() - 2).get();


				sig::SignalMetadata * sLast = zedHist->getmetadata(zedHist->queue_size() - 1).get();


				//int signal_size = lerp_skeletons(*(ZedBodies<Z> *)mem, *zbLast, *zbPenult, time_factor);
				//int signal_size = nuke_rotations(*(ZedBodies<Z> *)mem, *zbLast, *zbPenult, time_factor);
				int signal_size = simple_passthrough(*(ZedBodies<Z> *)mem, *zbLast, *zbPenult, time_factor);

				spdlog::debug("Outputting slerped rotation skel with {} bytes", signal_size);
				return signal_size;
			}
			spdlog::warn("Unexpected fallthrough in extrapolate_rotation_data");
			return 0;
		}

		template
			<typename Z>
			int extrapolate_kprot_data(uint8_t* mem, sig::time_point& time, sig::SignalMemory& sigMem) {

			int signal_size = 0;

			sig::SignalHistory* envHist = sigMem.get_history("env/v1.0");
			sig::SignalHistory* zedHist = sigMem.get_history("zed/v2.1");
			std::stringstream ss;
			ss = std::stringstream("");

			zedHist->lock();
			for (int i = 0; i < zedHist->queue_size(); i++) {
				ZedBodies<Z>* zb = (ZedBodies<Z> *) zedHist->getdata(i).get();
				ss << zedHist->getmetadata(i)->packetId << "/";
				ss << (int)zb->num_skeletons << " ";
			}
			zedHist->unlock();

			spdlog::debug("Zed queue is {}", ss.str());


			if (zedHist->queue_size() == 0) // If we have no entries, there's nothing to do
			{
				spdlog::debug("Transformer producing from an empty queue");
				return -1;
			}
			else if (zedHist->queue_size() == 1) // If there's one entry, then just use the last received data

			{
				std::shared_ptr<char[]> memsrc = zedHist->getdata(0);
				std::shared_ptr<sig::SignalMetadata> metasrc = zedHist->getmetadata(0);

				ZedBodies<Z>* zm = (ZedBodies<Z>*) memsrc.get();
				signal_size = ZedBodies<Z>::size(zm->num_skeletons);

				assert(memsrc != nullptr);
				spdlog::debug("Transformer producing packet {}, size {} from a single entry queue, skels {}", metasrc->packetId, signal_size, zm->num_skeletons);

				memcpy(mem, (void*)memsrc.get(), signal_size);

				std::stringstream ss;
				ss = std::stringstream("");
				for (int i = 0; i < signal_size; i++) {
					ss << (int)(*((uint8_t*)memsrc.get() + i)) << " ";
				}
				spdlog::debug("Single entry data from pkt {} = {}", metasrc->packetId, ss.str());

			}
			else { // Queue with more than one entry
				//spdlog::warn("Warning, unimplemented interpolation, falling back to prior frame");

				std::shared_ptr<sig::SignalMetadata> lastMeta = zedHist->getmetadata(zedHist->queue_size() - 1);
				std::shared_ptr<sig::SignalMetadata> prevMeta = zedHist->getmetadata(zedHist->queue_size() - 2);

				spdlog::debug("Transformer producing from a multiple-entry queue, last entry {}", lastMeta->packetId);

				std::chrono::system_clock::time_point time_now = std::chrono::system_clock::now();
				long long prevTime = std::chrono::duration_cast<std::chrono::microseconds> (prevMeta->acquisitionTime.time_since_epoch()).count();
				long long lastTime = std::chrono::duration_cast<std::chrono::microseconds> (lastMeta->acquisitionTime.time_since_epoch()).count();
				long long curTime = std::chrono::duration_cast<std::chrono::microseconds> (time_now.time_since_epoch()).count();

				int prevwindow = std::chrono::duration_cast<std::chrono::microseconds> (time_now - lastMeta->acquisitionTime).count();
				int curwindow = std::chrono::duration_cast<std::chrono::microseconds> (lastMeta->acquisitionTime - prevMeta->acquisitionTime).count();

				//spdlog::debug("Previous timestamps: {}, {}, {} -> ({}, {})", prevTime, lastTime, curTime, prevwindow, curwindow);

				float time_factor = (float)(curwindow + prevwindow) / (float)prevwindow;

				ZedBodies<Z>* zbLast = (ZedBodies<Z> *) zedHist->getdata(zedHist->queue_size() - 1).get();
				ZedBodies<Z>* zbPenult = (ZedBodies<Z> *) zedHist->getdata(zedHist->queue_size() - 2).get();

				ZedBodies<Z>* zm = (ZedBodies<Z> *) mem;

				//signal_size = lerp_skeletons(*(ZedBodies<Z> *)mem, *zbLast, *zbPenult, time_factor);
				signal_size = simple_passthrough(*(ZedBodies<Z> *)mem, *zbLast, *zbPenult, time_factor);

				for (int i = 0; i < zm->num_skeletons; i++) {
					assert(i < zm->num_skeletons);
					zm->skeletons[i].reset_keypoints();

				}

				for (int i = 0; i < zm->num_skeletons; i++) {
					for (int j = 0; j < zbLast->num_skeletons; j++) {
						if (zm->skeletons[i].id == zbLast->skeletons[j].id) {
							for (int k = 0; k < zbPenult->num_skeletons; k++) {
								if (zm->skeletons[j].id = zbPenult->skeletons[k].id) {}
								spdlog::info("Skel {}; RootPos: {}->{}->{}", zm->skeletons[i].id, zbPenult->skeletons[k].root_transform.pos.str(), zbLast->skeletons[i].root_transform.pos.str(), zm->skeletons[i].root_transform.pos.str());
								spdlog::info("Skel {}; Bone 0: {}->{}->{}", zm->skeletons[i].id, zbPenult->skeletons[k].bone_keypoints[0].str(), zbLast->skeletons[i].bone_keypoints[0].str(), zm->skeletons[i].bone_keypoints[0].str());
								spdlog::info("Skel {}; Bone 0Rot: {}->{}->{}", zm->skeletons[i].id, zbPenult->skeletons[k].bone_rotations[0].str(), zbLast->skeletons[i].bone_rotations[0].str(), zm->skeletons[i].bone_rotations[0].str());
								spdlog::info("Skel {}; Bone 1: {}->{}->{}", zm->skeletons[i].id, zbPenult->skeletons[k].bone_keypoints[1].str(), zbLast->skeletons[i].bone_keypoints[1].str(), zm->skeletons[i].bone_keypoints[1].str());
								spdlog::info("Skel {}; Bone 1Rot: {}->{}->{}", zm->skeletons[i].id, zbPenult->skeletons[k].bone_rotations[1].str(), zbLast->skeletons[i].bone_rotations[1].str(), zm->skeletons[i].bone_rotations[1].str());
							}
						}
					}
				}
			}
	

		

			zedHist->unlock();
			if (signal_size == 0)
				spdlog::warn("Unexpected fallthrough in extrapolate_rotation_data");

			return signal_size;

		};

		template
			<typename T>
			ZedBodies<T>* read();

		bool init();
        void shutdown();
        int get_data(uint8_t*, sig::time_point& time, sig::SignalMemory & sigMem);

		int extrapolate_data(sig::time_point & time);

		int get_current_frame();

		sig::time_point start_time;

		SensorTransformer(BufferProperties& bufProps) :
			bufferProperties(bufProps), read_counter(0) {}

	protected:
	/*
		template
		<class T>
		int process_zed_signal(uint8_t * mem);

		template
			<class T>
			int process_zed_kp_signal(uint8_t* mem);

		template
			<class T>
			int process_zed_kprot_signal(uint8_t* mem);
			*/
	protected:

        BufferProperties bufferProperties;
		int read_counter;
	};




	// Initialize the signal producer (alloc resources/caches, etc)
	DYNALO_EXPORT bool DYNALO_CALL SignalTransformerInitializeProducer(const sig::SignalProperties& sigProp);

	
	// Get signal data and the time of generation. Return number of written bytes
	DYNALO_EXPORT int DYNALO_CALL GetSignalDataTransformer(uint8_t* mem, sig::time_point& time, sig::SignalMemory & sigMem);

	// Shutdown the signal producer (free resources/caches, etc)
	DYNALO_EXPORT void DYNALO_CALL SignalTransformerShutdownProducer();


	//DYNALO_EXPORT std::string DYNALO_CALL GetName();
	DYNALO_EXPORT const char * DYNALO_CALL GetName();

	/**********************************************************************************************/
	// Possibly obsolescent

//	DYNALO_EXPORT void DYNALO_CALL ProcessSignalDataTransformer(const uint8_t* mem, int size, const sig::SignalMetadata& sigMeta);

	//DYNALO_EXPORT bool DYNALO_CALL SignalTransformerInitializeConsumer(const sig::SignalProperties& sigProp, const sig::SignalConsumerRuntimeConfig& cfg);

	DYNALO_EXPORT void DYNALO_CALL SignalTransformerShutdownConsumer();
	
}
