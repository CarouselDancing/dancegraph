
#pragma once


#define DYNALO_EXPORT_SYMBOLS
#include <dynalo/symbol_helper.hpp>
#include <dynalo/dynalo.hpp>

#include <sig/signal_transformer.h>
#include <sig/signal_common.h>
#include <sig/signal_history.h>

#include "zed_common.h"

#include <string>

#include <torch/torch.h>
#include <torch/script.h>



namespace zed {

	enum class PREDICTION_TYPE {
		NONE,
		KEYPOINTS34,
		KEYPOINTS38,
		QUATS34,
		QUATS38
	};
	

	const torch::Tensor tensor_none = torch::tensor({ }, torch::TensorOptions().dtype(torch::kInt32));

	const torch::Tensor joints_used_18_34 = torch::tensor({ 0, 1, 2, 3, 4, 5, 6, 7, 11, 12, 13, 14, 18, 19, 20, 22, 23, 24 }, torch::TensorOptions().dtype(torch::kInt32));

	// This one is wrong. Fix it!
	const torch::Tensor joints_used_18_38 = torch::tensor({ 0, 1, 2, 3, 4, 5, 6, 7, 11, 12, 13, 14, 18, 19, 20, 22, 23, 24 }, torch::TensorOptions().dtype(torch::kInt32));

	struct RemoteClientData {
		// Estimated latency
		double latency;
		
		sig::SignalHistory history;
	};

	class SignalTransformer {
	public:
		bool initialize_signal(const sig::SignalProperties& sigProp, const sig::SignalConsumerRuntimeConfig& cfg);
		void shutdown_signal();
	};


	template	
		<typename Z>
		int predict_data_rot(uint8_t* mem, sig::time_point& time, sig::SignalMemory& sigMem, int minimum_size, torch::jit::script::Module model, torch::Tensor used_bones = tensor_none) {
		return -1;
	}

	class TorchTransformer : public SignalTransformer {
	public:
		bool init(std::string sigType, int max_zed_sig_size);


		int get_data(uint8_t*, sig::time_point& time, sig::SignalMemory& sigMem);
		void shutdown();

		PREDICTION_TYPE predType;

		torch::jit::script::Module model;

		std::vector<float> kptensor;
		std::vector<float> quattensor;

		void populate_tensors();

		TorchTransformer(BufferProperties& bufProps) :
			bufferProperties(bufProps), read_counter(0) {}

		void set_local_idx(uint16_t idx) {
			local_idx = idx;
		}

		
		
		template
			<typename Z>
			int predict_data_kp(uint8_t* mem, sig::time_point& time, sig::SignalMemory& sigMem, int minimum_size, torch::jit::script::Module model, torch::Tensor used_bones = tensor_none) {

			sig::SignalHistory* envHist = sigMem.get_history("env/v1.0");
			sig::SignalHistory* zedHist = sigMem.get_history("zed/v2.1");

			if (zedHist->queue_size() == 0) {
				spdlog::debug("TorchTrans: No history queue for packet {}, returning -1");
				return -1;
			}
			else {
				for (int q_entry = zedHist->queue_size() - 1; q_entry >= 0; q_entry--) {
					std::shared_ptr<sig::SignalMetadata> meta = zedHist->getmetadata(q_entry);
					if (meta->userIdx >= latest_user_packetId.size()) {
						// New User; always process this
						latest_user_packetId.resize(meta->userIdx + 1);
						latest_user_packetId[meta->userIdx] = 0;
					}

					if (meta->packetId > latest_user_packetId[meta->userIdx]) {
						std::shared_ptr<char[]> memsrc = zedHist->getdata(q_entry);
						latest_user_packetId[meta->userIdx] = meta->packetId;

						ZedBodies<Z>* zm = (ZedBodies<Z> *) memsrc.get();
						ZedBodies<Z>* zedout = (ZedBodies<Z> *) (mem + sizeof(sig::SignalMetadata));

						int signal_size = ZedBodies<Z>::size(zm->num_skeletons);


						if (zm->num_skeletons == 0) {
							spdlog::debug("TorchTrans: No skeletons in ZedBodies packet, returning -1");
							// Nothing to see here
							return -1;
						}
						else if (meta->userIdx == local_idx) {
							// Add the local user to the signal history and pass on the original

							//signal_histories[meta->userIdx].add((const uint8_t *)memsrc.get(), signal_size, *meta);
							add_history(meta->userIdx, (const uint8_t*)memsrc.get(), signal_size, meta);
							memcpy((char*)mem, static_cast<void*> (meta.get()), sizeof(sig::SignalMetadata));
							memcpy((char*)mem + sizeof(sig::SignalMetadata), zm, zm->size(zm->num_skeletons));

							int output_size = zm->size(zm->num_skeletons) + sizeof(sig::SignalMetadata);
							local_body_set = true;

							spdlog::debug("TorchTrans: Local User {} being saved and passed on, passing on {} bytes", meta->userIdx, output_size);
							return output_size;
						}
						else if (!local_body_set) {
							// We don't have a local user. Pass the body through, and store it in case the local user shows up soon

							add_history(meta->userIdx, (const uint8_t*)memsrc.get(), signal_size, meta);
							//signal_histories[meta->userIdx].add((const uint8_t*)memsrc.get(), signal_size, *meta);

							memcpy((char*)mem, static_cast<void*>(meta.get()), sizeof(sig::SignalMetadata));
							memcpy((char*)mem + sizeof(sig::SignalMetadata), zm, zm->size(zm->num_skeletons));

							int output_size = zm->size(zm->num_skeletons) + sizeof(sig::SignalMetadata);
							spdlog::debug("TorchTrans: Local User {} being initialized and passed on, passing on {} bytes", meta->userIdx, output_size);
							return output_size;

						}
						else if (clients[meta->userIdx].history.queue_size() < prediction_size) {
							spdlog::debug("TorchTrans: Remote user {} - history {}/{} not long enough", meta->userIdx, clients[meta->userIdx].history.queue_size(), prediction_size);
							add_history(meta->userIdx, (const uint8_t*)memsrc.get(), signal_size, meta);


							memcpy((char*)mem, static_cast<void*>(meta.get()), sizeof(sig::SignalMetadata));
							memcpy((char*)mem + sizeof(sig::SignalMetadata), zm, zm->size(zm->num_skeletons));
							return (zm->size(zm->num_skeletons) + sizeof(sig::SignalMetadata));

						}
						else {
							// We want to do some predictions
							spdlog::debug("TorchTrans: Remote user {} being considered for prediction", meta->userIdx);
							// Add the latest input to the history
							
							add_history(meta->userIdx, (const uint8_t*)memsrc.get(), signal_size, meta);

							// Now assemble an appropriately sized libtorch tensor made of the histories

							// First, make the blob
							
							for (int i = 0; i < prediction_size; i++) {
								for (int j = 0; j < Z::bones_transmitted(); j++) {
									
									ZedBodies<Z>* zb = (ZedBodies<Z> *) clients[meta->userIdx].history.getdata(i).get();

									kptensor[i * (Z::bones_transmitted() + j) * 3 + 0] = zb->skeletons[0].bone_keypoints[j].x;
									kptensor[i * (Z::bones_transmitted() + j) * 3 + 1] = zb->skeletons[0].bone_keypoints[j].y;
									kptensor[i * (Z::bones_transmitted() + j) * 3 + 2] = zb->skeletons[0].bone_keypoints[j].z;
								}
							}

							auto kp_blob = torch::from_blob(kptensor.data(), { prediction_size, Z::bones_transmitted(), 3 }, torch::TensorOptions().dtype(torch::kFloat32));

							auto inputs_preshape = kp_blob.index({ torch::indexing::Slice(0, 50, torch::indexing::None), joints_used_18_34 });
							auto inputs = inputs_preshape.reshape({ 1, 50, -1 }).to(torch::kCUDA);

							try {
								auto model_output = model.forward({ inputs });
								torch::Tensor outputs = model_output.toTensor();
								// Doing nothing
							}
							catch (const std::exception e) {
								spdlog::debug("TorchTrans: Error running Model: {}", e.what());
								return -1;
							}

							memcpy((char*)mem, static_cast<void*>(meta.get()), sizeof(sig::SignalMetadata));
							memcpy((char*)mem + sizeof(sig::SignalMetadata), zm, zm->size(zm->num_skeletons));
							return (zm->size(zm->num_skeletons) + sizeof(sig::SignalMetadata));

						}
					}
					else {
						// We got a packet that predates the latest
						spdlog::debug("Fallthrough on packet {} for user {}, not later than {}", meta->packetId, meta->userIdx, latest_user_packetId[meta->userIdx]);
						return -1;
					}
				}
			}
		}


	protected:
		int minimum_history_size = 50; // We need a minimal history to predict from; populate from config
		BufferProperties bufferProperties;
		int read_counter;
		uint16_t local_idx = -1;
		
		bool local_body_set = false;
		// Just reusing the sig::SignalHistory class as a storage for each user
		std::vector<RemoteClientData> clients;

		void add_history(uint16_t, const uint8_t *,int, std::shared_ptr<sig::SignalMetadata>);

		int prediction_size;
		int max_zed_signal_size;
		int max_zed_queue_size;

		// Last packets seen by any given user
		std::vector<uint32_t> latest_user_packetId;
	};

	
	// Initialize the signal producer (alloc resources/caches, etc)
	DYNALO_EXPORT bool DYNALO_CALL SignalTransformerInitializeProducer(const sig::SignalProperties& sigProp);


	// Get signal data and the time of generation. Return number of written bytes
	DYNALO_EXPORT int DYNALO_CALL GetSignalDataTransformer(uint8_t* mem, sig::time_point& time, sig::SignalMemory& sigMem);

	// Shutdown the signal producer (free resources/caches, etc)
	DYNALO_EXPORT void DYNALO_CALL SignalTransformerShutdownProducer();

	DYNALO_EXPORT const char* DYNALO_CALL GetName();

	/**********************************************************************************************/
	// Possibly obsolescent

//	DYNALO_EXPORT void DYNALO_CALL ProcessSignalDataTransformer(const uint8_t* mem, int size, const sig::SignalMetadata& sigMeta);

	//DYNALO_EXPORT bool DYNALO_CALL SignalTransformerInitializeConsumer(const sig::SignalProperties& sigProp, const sig::SignalConsumerRuntimeConfig& cfg);

	DYNALO_EXPORT void DYNALO_CALL SignalTransformerShutdownConsumer();

	//DYNALO_EXPORT std::string DYNALO_CALL SetLocalUserIdx();
	DYNALO_EXPORT void DYNALO_CALL SetLocalUserIdx(uint16_t idx);



}