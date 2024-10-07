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
	
	void sigdump(std::string header, uint8_t* mem, int size) {
		std::stringstream ss = std::stringstream("");
		ss << std::hex;
		if (size >= 64) {
			for (int i = 0; i < 64; i++) {
				ss << (int)*(mem + i) << " ";
			}
			spdlog::trace("Mimic: {} sigdump: {}", header, ss.str());
		}
	}


	class MimicTransformer {
		
    public:

		sig::time_point start_time;
		bool init();
		int get_data(uint8_t* mem, sig::time_point& time, sig::SignalMemory& sigMem);
		void shutdown();

		void set_local_idx(uint16_t idx);

		MimicTransformer(BufferProperties& bufProps) :
			bufferProperties(bufProps), read_counter(0) {}

		char* lastLocalUserFrame;
		
		sig::SignalMetadata lastLocalMetadata;

		template
			<typename Z>
			bool init_local_user_ptr() {			
			ZedBodies<Z>* zp = ZedBodies<Z>::allocate(MAX_SKEL_COUNT);
			if (zp == nullptr)
				return false;
			lastLocalUserFrame = (char*)zp;
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
			int mimic_user(uint8_t* mem, sig::time_point time, sig::SignalMemory& sigMem) {

			sig::SignalHistory* envHist = sigMem.get_history("env/v1.0");
			sig::SignalHistory* zedHist = sigMem.get_history("zed/v2.1");

			ZedBodies<Z>* localZedBodies = (ZedBodies<Z> *)lastLocalUserFrame;

			// Nothing in the queue
			if (zedHist->queue_size() == 0) {
				return -1;
			}
			else {
				// Descend through the queue checking for data we've not seen yet
				for (int q_entry = zedHist->queue_size() - 1; q_entry >= 0; q_entry--) {

					
					//std::shared_ptr<sig::SignalMetadata> meta = zedHist->getmetadata(zedHist->queue_size() - 1);

					std::shared_ptr<sig::SignalMetadata> meta = zedHist->getmetadata(q_entry);

					if (meta->userIdx >= latest_user_packetId.size()) {
						spdlog::info("Mimic: Adding user {} to the packetID pool", meta->userIdx);
						latest_user_packetId.resize(meta->userIdx + 1);
						// If this is the first packet from this user we always want to handle it
						
						latest_user_packetId[meta->userIdx] = 0;
					}

					if (meta->packetId > latest_user_packetId[meta->userIdx]) {
						spdlog::trace("Mimic: User {} packet {} update over {}", meta->userIdx, meta->packetId, latest_user_packetId[meta->userIdx]);
						std::shared_ptr<char[]> memsrc = zedHist->getdata(q_entry);

						latest_user_packetId[meta->userIdx] = meta->packetId;

						ZedBodies<Z>* zm = (ZedBodies<Z> *)memsrc.get();
						ZedBodies<Z>* zedout = (ZedBodies<Z> *) (mem + sizeof(sig::SignalMetadata));

						int signal_size = ZedBodies<Z>::size(zm->num_skeletons);

						if ((meta->userIdx == local_idx) && (zm->num_skeletons > 0)) {

							spdlog::trace("Mimic: Updating local user {}", meta->userIdx);
							// Compiler didn't want to memcpy to a pointer to lastLocalMetadata so we'll do it longhand
							memcpy((char*)&lastLocalMetadata, static_cast<void*>(meta.get()), sizeof(sig::SignalMetadata));

							memcpy((char*)lastLocalUserFrame, zm, zm->size(zm->num_skeletons));
							local_body_set = true;

							// We also have to send the body upstream of course

							memcpy((char*)mem, static_cast<void*> (meta.get()), sizeof(sig::SignalMetadata));
							memcpy((char*)mem + sizeof(sig::SignalMetadata), zm, zm->size(zm->num_skeletons));
							//sigdump(std::string("localsave"), mem, ZedBodies<Z>::size(zm->num_skeletons));
							return (zm->size(zm->num_skeletons) + sizeof(sig::SignalMetadata));
						}

						else if (!local_body_set) {
							// Just pass through the actual body
							memcpy((char*)mem, static_cast<void*> (meta.get()), sizeof(sig::SignalMetadata));
							memcpy((char*)mem + sizeof(sig::SignalMetadata), zm, zm->size(zm->num_skeletons));
							//sigdump(std::string("passthrough"), mem, ZedBodies<Z>::size(zm->num_skeletons));
							return (zm->size(zm->num_skeletons) + sizeof(sig::SignalMetadata));


						}
						else if (zm->num_skeletons == 0) {
							// Not returning a signal for no skels
							return -1;
						}
						else {
							memcpy((char*)mem, (char*)meta.get(), sizeof(sig::SignalMetadata));

							ZedBodies<Z>* zlluf = (ZedBodies<Z> *) lastLocalUserFrame;
							//memcpy((char*)mem + sizeof(sig::SignalMetadata), (void*)zm, ZedBodies<Z>::size(zm->num_skeletons));
#if false
							// Fuck it, let's just copy the whole transform over
							spdlog::info("Mimic: Flat copy, user {}, with {} skels, producing metadata packet {} type {} Idx {} userIdx {}", meta->userIdx, zm->num_skeletons, zedoutm->packetId, zedoutm->sigType, zedoutm->sigIdx, zedoutm->userIdx);

							memcpy(mem + sizeof(sig::SignalMetadata), (uint8_t*)localZedBodies, ZedBodies<Z>::size(zlluf->num_skeletons));
							//sigdump(std::string("copy"), mem + sizeof(sig::SignalMetadata), ZedBodies<Z>::size(zlluf->num_skeletons));
							//sigdump(std::string("copysrc"), (uint8_t*)localZedBodies, ZedBodies<Z>::size(zm->num_skeletons));
							spdlog::info("Sigdata size is {}+{}={}", sizeof(sig::SignalMetadata), ZedBodies<Z>::size(zm->num_skeletons), sizeof(sig::SignalMetadata) + ZedBodies<Z>::size(zm->num_skeletons));

							return sizeof(sig::SignalMetadata) + ZedBodies<Z>::size(localZedBodies->num_skeletons);
					
#else
							sig::SignalMetadata* zedoutm = (sig::SignalMetadata*)mem;
							// Mimic the local user's rotations and unravel and reravel the skeleton to match
							for (int i = 0; i < zm->num_skeletons; i++) {
								spdlog::debug("Mimic: About to recalc keypoints");
								zedout->skeletons[i].calculate_keypoints(localZedBodies->skeletons[0].bone_rotations);
								memcpy(&zedout->skeletons[i].bone_rotations, &localZedBodies->skeletons[0].bone_rotations, NUM_BONES_FULL_34 * sizeof(quant_quat));
							}
							return sizeof(sig::SignalMetadata) + ZedBodies<Z>::size(zm->num_skeletons);

#endif							

						}
					}
					else {
						spdlog::trace("Mimic: User: {}, Current packet {} compared with Latest packet {}", meta->userIdx, meta->packetId, latest_user_packetId[meta->userIdx]);
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


	//DYNALO_EXPORT std::string DYNALO_CALL SetLocalUserIdx();
	DYNALO_EXPORT void DYNALO_CALL SetLocalUserIdx(uint16_t idx);



	/**********************************************************************************************/
	// Possibly obsolescent


	DYNALO_EXPORT void DYNALO_CALL SignalTransformerShutdownConsumer();
	
}
