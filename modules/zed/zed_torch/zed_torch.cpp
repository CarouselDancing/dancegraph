
#include "zed_torch.h"

#include <filesystem>

#include <core/net/config.h>
#include <core/net/config_master.h>
#include <core/common/utility.h>

#include <torch/torch.h>
#include <torch/script.h>

using namespace std;

namespace zed {

	struct defaultedPREDICTION_TYPE {
		PREDICTION_TYPE t = PREDICTION_TYPE::NONE;
	};
	

	std::map<std::string, defaultedPREDICTION_TYPE > predictiontypemap{
		{std::string(""), defaultedPREDICTION_TYPE{PREDICTION_TYPE::NONE}},
		{std::string("keypoints_34"), defaultedPREDICTION_TYPE{ PREDICTION_TYPE::KEYPOINTS34} },
		{ std::string("keypoints_38"), defaultedPREDICTION_TYPE{PREDICTION_TYPE::NONE} },
		{ std::string("quats_34"), defaultedPREDICTION_TYPE{PREDICTION_TYPE::NONE} },
		{ std::string("quats_38"), defaultedPREDICTION_TYPE{PREDICTION_TYPE::NONE} }
	};

	std::string TRANSFORMER_NAME = std::string("torch_transformer/v1.0");

	unique_ptr<TorchTransformer> g_TorchTransformer;


	void zed::TorchTransformer::populate_tensors() {
		int fullbones, compactbones;

		switch (bufferProperties.bodySignalType) {
		case zed::Zed4SignalType::Body_34_Compact:
			fullbones = zed::ZedSkeletonCompact_34::bones_skeleton(); compactbones = zed::ZedSkeletonCompact_34::bones_transmitted();
			break;
		case zed::Zed4SignalType::Body_38_Compact:
			fullbones = zed::ZedSkeletonCompact_38::bones_skeleton(); compactbones = zed::ZedSkeletonCompact_38::bones_transmitted();

			break;
		case zed::Zed4SignalType::Body_34_Full:
			fullbones = zed::ZedSkeletonFull_34::bones_skeleton(); compactbones = zed::ZedSkeletonFull_34::bones_transmitted();

			break;
		case zed::Zed4SignalType::Body_38_Full:
			fullbones = zed::ZedSkeletonFull_38::bones_skeleton(); compactbones = zed::ZedSkeletonFull_38::bones_transmitted();

			break;
		case zed::Zed4SignalType::Body_34_KeypointsPlus:
			fullbones = zed::ZedSkeletonKPRot_34::bones_skeleton(); compactbones = zed::ZedSkeletonKPRot_34::bones_transmitted();

			break;
		case zed::Zed4SignalType::Body_38_KeypointsPlus:
			fullbones = zed::ZedSkeletonKPRot_38::bones_skeleton(); compactbones = zed::ZedSkeletonKPRot_38::bones_transmitted();
			break;
		case zed::Zed4SignalType::Body_34_Keypoints:
		case zed::Zed4SignalType::Body_38_Keypoints:
		default:
			spdlog::debug("Extrapolation Transformer: Body Signal Type {} not currently implemented", int(bufferProperties.bodySignalType));
			break;
		}

		for (int j = 0; j < prediction_size; j++) {
			for (int i = 0; i < fullbones; i++) {
				kptensor.push_back(0.0); kptensor.push_back(0.0); kptensor.push_back(0.0);
				quattensor.push_back(0.0); quattensor.push_back(0.0); quattensor.push_back(0.0); quattensor.push_back(1.0);
			}
		}
	}

	bool zed::TorchTransformer::init(std::string sigType, int signalSize) {

		if (!net::cfg::Root::load())
		{
			spdlog::error("Failed loading dancegraph.json");
			return false;
		}
		
		const auto& master_cfg = net::cfg::Root::instance();

		// I'm not so happy that we're calling the master config from inside a module
		auto tfs = dancenet::string_split(TRANSFORMER_NAME, '/');

		spdlog::info("Transformer name is {} or {}, {}", TRANSFORMER_NAME, tfs[0], tfs[1]);
		auto tconfig = master_cfg.transformers.at(tfs[0]).at(tfs[1]);

		

		std::string model_file;
		std::string ptstring;

		max_zed_signal_size = signalSize;

		try {
			model_file = tconfig.opts.at(sigType).at("prediction_model_file");
			ptstring = tconfig.opts.at(sigType).at("prediction_model_type");
			prediction_size = tconfig.opts.at(sigType).at("prediction_size");

			predType = predictiontypemap.at(ptstring).t;
			
			max_zed_queue_size = tconfig.opts.at(sigType).at("signal_queue_size");

			spdlog::info("Torch transformer initializing model {} with type {}", model_file, ptstring);
		}
		catch (json::exception e) {
			spdlog::warn("Failed to get torch_transformer model file or type from config: {}", e.what());
			return false;
		}
		catch (std::exception e) {
			spdlog::warn("Failed to get torch_transformer model file or type from config: {}", e.what());
			return false;
		}


		std::filesystem::path torch_model_file = std::filesystem::path(net::cfg::DanceGraphAppDataPath()) / "modules" / model_file;

		ifstream f(torch_model_file.c_str(), std::ios::binary);
		if (!f.good()) {
			//spdlog::warn("Failed to find torch transformer model file at {}", torch_model_file.c_str());
			spdlog::warn("Failed to find torch transformer model file {}", torch_model_file.string());
			return false;
		}

		if (predType == PREDICTION_TYPE::NONE) {
			spdlog::warn("Torch transformer failed due to unknown prediction type {}", ptstring);
			return false;
		}
		//f.close();
		
		try {
			spdlog::info("Loading Transformer from file {}", torch_model_file.string());
			model = torch::jit::load(f);
		}
		catch (c10::Error& e) {
			spdlog::error("A:Error loading model {}: {}", model_file, e.what());
		}
		catch (std::exception e) {
			spdlog::error("B:Error loading model {}: {}", model_file, e.what());
			return false;
		}
		
		/*
		model.eval();
		model.to(torch::kCUDA);
		*/

		clients = std::vector<RemoteClientData>();
		populate_tensors();

		return true;
	}



	void zed::TorchTransformer::shutdown() {
		// Intentionally left blank
	}

	DYNALO_EXPORT bool DYNALO_CALL SignalTransformerInitializeProducer(const sig::SignalProperties& sigProp) {


		spdlog::set_default_logger(sigProp.logger);
		spdlog::debug("SignalProducerInitialize Called for Transformer");
		BufferProperties bProp = BufferProperties();
		spdlog::info("zed_torch properties: {}", sigProp.jsonConfig);

		const json& jsonProps = json::parse(sigProp.jsonConfig);

		bProp.populate_from_json(jsonProps); 

		g_TorchTransformer = unique_ptr<TorchTransformer>(new TorchTransformer(bProp));

		spdlog::info("SignalProducerInitialize Ended");
		return g_TorchTransformer->init(sigProp.signalType, bProp.signalSize());
		
	}

	int TorchTransformer::get_data(uint8_t* mem, sig::time_point& time, sig::SignalMemory& sigMem) {
		
		int retval = -1;

		switch (bufferProperties.bodySignalType) {
		case zed::Zed4SignalType::Body_34_Compact:
			retval = predict_data_rot<ZedSkeletonCompact_34>(mem, time, sigMem, minimum_history_size, model);
			break;
		case zed::Zed4SignalType::Body_38_Compact:
			retval = predict_data_rot<ZedSkeletonCompact_38>(mem, time, sigMem, minimum_history_size, model);
			break;
		case zed::Zed4SignalType::Body_34_Full:
			retval = predict_data_rot<ZedSkeletonFull_34>(mem, time, sigMem, minimum_history_size, model);
			break;
		case zed::Zed4SignalType::Body_38_Full:
			retval = predict_data_rot<ZedSkeletonFull_38>(mem, time, sigMem, minimum_history_size, model);
			break;
		case zed::Zed4SignalType::Body_34_KeypointsPlus:
			
			retval = predict_data_kp<ZedSkeletonKPRot_34>(mem, time, sigMem, minimum_history_size, model, joints_used_18_34);
			break;
		case zed::Zed4SignalType::Body_38_KeypointsPlus:
			retval = predict_data_kp<ZedSkeletonKPRot_38>(mem, time, sigMem, minimum_history_size, model, joints_used_18_38);
			break;
		case zed::Zed4SignalType::Body_34_Keypoints:
		case zed::Zed4SignalType::Body_38_Keypoints:
		default:
			spdlog::debug("Extrapolation Transformer: Body Signal Type {} not currently implemented", int(bufferProperties.bodySignalType));
			retval = -1;
			break;
		}

		return retval;
	}

	void TorchTransformer::add_history(uint16_t userIdx, const uint8_t * memdata, int size, std::shared_ptr<sig::SignalMetadata> meta) {
		
		if (userIdx >= clients.size()) {
			int old_size = clients.size();
			clients.resize(userIdx + 1);

			for (int i = old_size; i <= userIdx; i++) {
				spdlog::debug("Torch Xform: initializing signal for user {}, queue {}, sigsize {}", i, max_zed_queue_size, max_zed_signal_size);
				clients[i] = RemoteClientData{ 0.0, sig::SignalHistory{} };
				clients[i].history.initialize(max_zed_queue_size, max_zed_signal_size);
				
				clients[i].latency = 0.0;
				spdlog::debug("Adding placeholder latency {} to client {}", clients[i].latency, i);
			}
		}

		spdlog::debug("Torch Xform: adding signal entry for user {}, sigsize {}/{}", userIdx, clients[userIdx].history.signalList.size(), clients[userIdx].history.max_queue_size);

		//clients[userIdx].history.initialize(max_zed_queue_size, max_zed_signal_size);
		clients[userIdx].history.add(memdata, size, *meta);
	}



	// Get signal data and the time of generation. Return number of written bytes
	DYNALO_EXPORT int DYNALO_CALL GetSignalDataTransformer(uint8_t* mem, sig::time_point& time, sig::SignalMemory& sigMem) {

		int rv = g_TorchTransformer->get_data(mem, time, sigMem);
		spdlog::debug("Transformer producing {} bytes", rv);
		return rv;
	}

	// Shutdown the signal producer (free resources/caches, etc)

	DYNALO_EXPORT void DYNALO_CALL SignalTransformerShutdownProducer() {
		g_TorchTransformer->shutdown();
	}

	DYNALO_EXPORT void DYNALO_CALL SignalTransformerShutdownConsumer() {

	}


	DYNALO_EXPORT void DYNALO_CALL SetLocalUserIdx(uint16_t idx) {
		g_TorchTransformer->set_local_idx(idx);
	}


	DYNALO_EXPORT const char* DYNALO_CALL GetName() {
		return TRANSFORMER_NAME.c_str();
	}

}