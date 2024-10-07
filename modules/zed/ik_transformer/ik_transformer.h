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
	
	//constexpr int EXTRAPOLATE_MAX_QUEUE_SIZE = 10;

	// The specification of the IK problem to solve
	
	struct IKTarget {
		// pivot_bone - the bone that is kept constant
		BODY_34_PARTS pivot_bone; 

		// end_bone - the bone that is moved towards the effector
		BODY_34_PARTS end_bone;

		// effector - the target effector position
		vec3 effector;

		// The time in the future when this problem needs to be solved
		// Maybe replace with a global timestamp
		float timeframe;

	};
	
	template 
		<typename D>
		std::string dump_array(const D* obj, int count) {
		std::stringstream ss;
		ss.str("");
		for (int i = 0; i < count; i++) {
			ss << "[" << obj[i].str() << "] ";
		}
		return ss.str();
		
	}

	// This class looks at the state of the world (music, local user, end users) and decides what IK problem is to be solved. Should be a base class
	template
		<typename Z>
		class IKSetterSolver {
		protected:
			IKTarget problem;
			int maxPasses = 1;
			// Looks at the state of the world and determines the IK problem to solve. Populates 
			// musicSignal TBD

		public:
			bool create_problem(const Z& target_skeleton, const Z& subject_skeleton, float musicSignal = 0.0f) {
				// Currently a stub test problem of forcing the hand to be at a certain place relative to the hips

				problem.effector = subject_skeleton.bone_keypoints[0] + vec3(0.0, 0.0, 200.0);
				problem.timeframe = 0.0;
				problem.pivot_bone = BODY_34_PARTS::RIGHT_CLAVICLE;
				problem.end_bone = BODY_34_PARTS::RIGHT_HANDTIP;

				return true;
			}


			bool solve_problem(Z& answer, Z& subject) {

				spdlog::debug("IKTrans: solve problem with subject pos {},{},{} and ori {},{},{}",
					subject.root_transform.pos.x,
					subject.root_transform.pos.y,
					subject.root_transform.pos.z,
					subject.root_transform.ori.x,
					subject.root_transform.ori.y,
					subject.root_transform.ori.z
				);
				// Also other criteria for termination, such as each pass isn't improving anything, or the end bone is sufficiently close to the effector

				memcpy((char*)&subject, (char*)&answer, sizeof(Z));

				static std::array<quat, NUM_BONES_FULL_34> output_rotations;

				for (int i = 0; i < NUM_BONES_FULL_34; i++) {
					output_rotations[i] = quat{ 0,0,0,1 };
				}

				for (int i = 0; i < NUM_BONES_COMPACT_34; i++) {
					output_rotations[BONELIST_34_COMPACT[i]] = subject.bone_rotations[i].unquantize();
				}

				
				spdlog::debug("IKTrans: pre-pass rot: {} ori {}", subject.root_transform.pos.str(), subject.root_transform.ori.str());
				spdlog::debug("IKTrans: pre-pass rotations {}", dump_array<quant_quat>(subject.bone_rotations.data(), NUM_BONES_COMPACT_34));
				spdlog::debug("IKTrans: pre-pass keypoints {}", dump_array<vec3>(subject.bone_keypoints.data(), NUM_BONES_FULL_34));
				spdlog::debug("IKTrans: Effector position: {}, bone-chain {} to {}", problem.effector.str(), (int)problem.pivot_bone, (int)problem.end_bone);


				spdlog::debug("IKTrans: pre-pass fullrots {}", dump_array<quat>(output_rotations.data(), NUM_BONES_FULL_34));

				for (int i = 0; i < maxPasses; i++) {
					subject.ccd_pass(output_rotations, problem.effector, problem.pivot_bone, problem.end_bone, subject.bone_keypoints);
				}
			
				spdlog::debug("IKTrans: post-pass fullrots {}", dump_array<quat>(output_rotations.data(), NUM_BONES_FULL_34));

				answer.calculate_keypoints(output_rotations, true);
				spdlog::debug("IKTrans: post-pass rot: {} ori {}", answer.root_transform.pos.str(), answer.root_transform.ori.str());
				spdlog::debug("IKTrans: post-pass rotations {}", dump_array<quant_quat>(answer.bone_rotations.data(), NUM_BONES_COMPACT_34));
				spdlog::debug("IKTrans: post-pass keypoints {}", dump_array<vec3>(answer.bone_keypoints.data(), NUM_BONES_FULL_34));
				

				//answer.calculate_keypoints(answer.bone_rotations, false);
				spdlog::debug("IKTrans: Solved Problem: skel 0 root pos: {},{},{}, root rot: {},{},{}",
					answer.root_transform.pos.x,
					answer.root_transform.pos.y,
					answer.root_transform.pos.z,
					answer.root_transform.ori.x,
					answer.root_transform.ori.y,
					answer.root_transform.ori.z);


				return true;
			}


	};

	class IKTransformer {
		
    public:

		sig::time_point start_time;
		bool init();
		int get_data(uint8_t* mem, sig::time_point& time, sig::SignalMemory& sigMem);
		void shutdown();

		void set_local_idx(uint16_t idx);

		IKTransformer(BufferProperties& bufProps) :
			bufferProperties(bufProps), read_counter(0) {}

		
		char* lastLocalUserFrame;
		char* rawSignalSkeletons;
		
		bool testSignalEnabled = false;

		// For test signals
		sig::SignalMetadata lastLocalMetadata;
		sig::SignalMetadata rawOriginalMetadata;


		const int TEST_USER_ADDITION = 256;

		bool sendNonLocalSignal = false;


		template
			<typename Z>
			bool init_local_user_ptr() {			
			ZedBodies<Z>* zp = ZedBodies<Z>::allocate(MAX_SKEL_COUNT);

			ZedBodies<Z>* sf = ZedBodies<Z>::allocate(MAX_SKEL_COUNT);
			
			if (zp == nullptr)
				return false;
			lastLocalUserFrame = (char*)zp;
			rawSignalSkeletons = (char*)sf;

			local_body_set = false;

			return true;
		}


		template
			<typename Z>
			void free_local_user_ptr() {
			ZedBodies<Z>* zp = (ZedBodies<Z> *) lastLocalUserFrame;
			free(zp);
		}

		template
			<typename Z>
			int ik_prediction(uint8_t* mem, sig::time_point time, sig::SignalMemory& sigMem) {
			sig::SignalHistory* envHist = sigMem.get_history("env/v1.0");
			sig::SignalHistory* zedHist = sigMem.get_history("zed/v2.1");

			ZedBodies<Z>* localZedBodies = (ZedBodies<Z> *)lastLocalUserFrame;
			IKSetterSolver<Z> ikProblem;

			ZedBodies<Z>* rawOriginalSignal = (ZedBodies<Z> *)rawSignalSkeletons;

			// Send the dummy test signal 
			if (testSignalEnabled && sendNonLocalSignal) {
				// TODO: Doesn't work in it's current state. Fix
				rawOriginalMetadata.userIdx = rawOriginalMetadata.userIdx + TEST_USER_ADDITION;
				spdlog::debug("Sending dummy local signal with userIdx {}", rawOriginalMetadata.userIdx);
				memcpy((uint8_t*)mem, (uint8_t*)&rawOriginalMetadata, sizeof(sig::SignalMetadata));
				memcpy((uint8_t*)mem + sizeof(sig::SignalMetadata), (uint8_t*)&rawOriginalSignal, ZedBodies<Z>::size(rawOriginalSignal->num_skeletons));
				sendNonLocalSignal = false;
				return sizeof(sig::SignalMetadata) + ZedBodies<Z>::size(rawOriginalSignal->num_skeletons);
			}

			if (zedHist->queue_size() <= 0) {
				return -1;
			}
			else {
				
				for (int q_entry = zedHist->queue_size() - 1; q_entry >= 0; q_entry--) {
					
					std::shared_ptr<sig::SignalMetadata> meta = zedHist->getmetadata(q_entry);
					if (meta->userIdx >= latest_user_packetId.size()) {
						spdlog::debug("IKTrans {}: Adding user {} to the packetID pool", meta->packetId, meta->userIdx);
						// We always want to handle packets from new users
						latest_user_packetId.resize(meta->userIdx + 1);
						latest_user_packetId[meta->userIdx] = 0;
					}

					if (meta->packetId > latest_user_packetId[meta->userIdx]) {
						spdlog::trace("IKTrans: User {} packet {} update over {}", meta->userIdx, meta->packetId, latest_user_packetId[meta->userIdx]);
						std::shared_ptr<char[]> memsrc = zedHist->getdata(q_entry);
						
						latest_user_packetId[meta->userIdx] = meta->packetId;

						ZedBodies<Z>* zm = (ZedBodies<Z> *) memsrc.get();
						ZedBodies<Z>* zedout = (ZedBodies<Z> *) (mem + sizeof(sig::SignalMetadata));

						if (zm->num_skeletons > 0) {
							spdlog::debug("IKTrans: User {}, packet {}, skel 0 root pos: {},{},{}, root rot: {},{},{}", meta->userIdx,
								meta->packetId,
								zm->skeletons[0].root_transform.pos.x,
								zm->skeletons[0].root_transform.pos.y,
								zm->skeletons[0].root_transform.pos.z,
								zm->skeletons[0].root_transform.ori.x,
								zm->skeletons[0].root_transform.ori.y,
								zm->skeletons[0].root_transform.ori.z);
						}
						int signal_size = ZedBodies<Z>::size(zm->num_skeletons);

						if ((meta->userIdx == local_idx) && (zm->num_skeletons > 0)) {
							spdlog::info("IKTrans {}: Updating local user {}", meta->packetId, meta->userIdx);
							memcpy((char*)&lastLocalMetadata, static_cast<void*>(meta.get()), sizeof(sig::SignalMetadata));
							memcpy((char*)lastLocalUserFrame, zm, zm->size(zm->num_skeletons));
							local_body_set = true;
							memcpy((char*)mem, static_cast<void*>(meta.get()), sizeof(sig::SignalMetadata));
							memcpy((char*)mem + sizeof(sig::SignalMetadata), zm, zm->size(zm->num_skeletons));
							return (zm->size(zm->num_skeletons) + sizeof(sig::SignalMetadata));

						}
						else if (!local_body_set) {
							spdlog::info("IKTrans {}: Local body not set, pass through user {} remote signal data", meta->packetId, meta->userIdx);
							memcpy((char*)mem, static_cast<void*> (meta.get()), sizeof(sig::SignalMetadata));
							memcpy((char*)mem + sizeof(sig::SignalMetadata), zm, zm->size(zm->num_skeletons));
							return (zm->size(zm->num_skeletons) + sizeof(sig::SignalMetadata));
						}
						else if (zm->num_skeletons == 0) {							
							return -1;
						}
						else {
							spdlog::debug("IKTrans {}: Morphing user {}", meta->packetId, meta->userIdx);
							//Copy over the metadata
							memcpy((char*)mem, static_cast<void*> (meta.get()), sizeof(sig::SignalMetadata));

							// The main meat of the transformer
							
							memcpy((char*)zedout, zm, ZedBodies<Z>::size(zm->num_skeletons));
							
							ZedBodies<Z>* zlluf = (ZedBodies<Z> *)lastLocalUserFrame;

							for (int i = 0; i < zm->num_skeletons; i++) {
								bool problemCreated = ikProblem.create_problem(zlluf->skeletons[0], zm->skeletons[i]);		

								if (problemCreated) {
									spdlog::debug("IKTransformer: Solving created problem for user {}", meta->userIdx);

									ikProblem.solve_problem(zedout->skeletons[i], zm->skeletons[i]);

								}
								else {
									spdlog::debug("IKTransformer: Passthrough for user {} with no created problem; size {}", meta->userIdx, sizeof(Z));

									memcpy((void*)&zedout->skeletons[i], (void*)&zm->skeletons[i], sizeof(Z));
								}
							}

							spdlog::debug("IKTrans: Returning data with skel 0 pos {},{},{} ori {},{},{}",
								zedout->skeletons[0].root_transform.pos.x,
								zedout->skeletons[0].root_transform.pos.y,
								zedout->skeletons[0].root_transform.pos.z,
								zedout->skeletons[0].root_transform.pos.x,
								zedout->skeletons[0].root_transform.pos.y,
								zedout->skeletons[0].root_transform.pos.z);

							std::stringstream ss;
							ss.str("");
							for (int i = 0; i < NUM_BONES_FULL_34; i++) {
								ss << "[" << zedout->skeletons[0].bone_keypoints[i].str() << "] ";
							}
							spdlog::debug("Final Answer: {}", ss.str());

							// We altered the signal, now save it

							memcpy((void*)&rawOriginalSignal, (void*)&zm, ZedBodies<Z>::size(zm->num_skeletons));
							memcpy((void*)&rawOriginalMetadata, (void*)&meta, sizeof(sig::SignalMetadata));


							sendNonLocalSignal = true;

							return ZedBodies<Z>::size(zm->num_skeletons) + sizeof(sig::SignalMetadata);
						}

						/*
							sig::SignalMetadata* zedoutm = (sig::SignalMetadata*)mem;
							for (int i = 0; i < zm->num_skeletons; i++) {
								zedout->skeletons[i].calculate_keypoints(localZedBodies->skeletons[0].bone_rotations);
								memcpy(&zedout->skeletons[i].bone_rotations, &localZedBodies->skeletons[0].bone_rotations, NUM_BONES_FULL_34 * sizeof(quant_quat));
							}
							return sizeof(sig::SignalMetadata) + ZedBodies<Z>::size(zm->num_skeletons);

						}
						*/

					}


				}

				return -1;
			}

		}

	protected:

		BufferProperties bufferProperties;
		int read_counter;
		uint16_t local_idx;
		bool local_body_set;
		// The last packetID we've seen from a given user
		std::vector<uint32_t> latest_user_packetId;


	};
	// Initialize the signal producer (alloc resources/caches, etc)
	DYNALO_EXPORT bool DYNALO_CALL SignalTransformerInitializeProducer(const sig::SignalProperties& sigProp);

	
	// Get signal data and the time of generation. Return number of written bytes
	DYNALO_EXPORT int DYNALO_CALL GetSignalDataTransformer(uint8_t* mem, sig::time_point& time, sig::SignalMemory & sigMem);

	// Shutdown the signal producer (free resources/caches, etc)
	DYNALO_EXPORT void DYNALO_CALL SignalTransformerShutdownProducer();


	//DYNALO_EXPORT std::string DYNALO_CALL GetName();
	DYNALO_EXPORT const char* DYNALO_CALL GetName();

	//DYNALO_EXPORT std::string DYNALO_CALL GetName();
	DYNALO_EXPORT void DYNALO_CALL SetLocalUserIdx(uint16_t idx);



	/**********************************************************************************************/
	// Possibly obsolescent


	DYNALO_EXPORT void DYNALO_CALL SignalTransformerShutdownConsumer();
	
}
