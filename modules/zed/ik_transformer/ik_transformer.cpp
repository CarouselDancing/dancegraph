

#define DYNALO_EXPORT_SYMBOLS

#include <dynalo/symbol_helper.hpp>
#include <dynalo/dynalo.hpp>

#include <spdlog/spdlog.h>
//#include <magic_enum/magic_enum.hpp>

#include "ik_transformer.h"
#include "../env/env_common.h"

#include <core/lib/sig/signal_history.h>

#include <modules/zed/zed_common.h>

//using namespace sl;
using namespace std;

namespace zed {

	const char* TRANSFORMER_NAME = "ik_transformer/v1.0";

	// g_SensorTransformer is stored as a pointer to delay construction until we've got arguments to construct with
	// (Since the ringbuffer has no default constructor, we can't store the actual class without a lot more hassle)
	unique_ptr<IKTransformer> g_IKTransformer;


	// Initialize the signal producer (alloc resources/caches, etc)
	DYNALO_EXPORT bool DYNALO_CALL SignalTransformerInitializeProducer(const sig::SignalProperties& sigProp)

	{	
		
		spdlog::set_default_logger(sigProp.logger);
		spdlog::debug("SignalProducerInitialize Called for Transformer");
		BufferProperties bProp = BufferProperties();
		spdlog::info("zedcam_producer.cpp Json properties: {}", sigProp.jsonConfig);
		bProp.populate_from_json(json::parse(sigProp.jsonConfig)); // Pass in the name of a config file if we want, otherwise it finds a default
		
		g_IKTransformer = unique_ptr<IKTransformer>(new IKTransformer(bProp));
		spdlog::info("SignalProducerInitialize Ended");


		return g_IKTransformer->init();

	}


	bool IKTransformer::init() {

		switch (bufferProperties.bodySignalType) {
		case zed::Zed4SignalType::Body_34_KeypointsPlus:
			return init_local_user_ptr<ZedSkeletonKPRot_34>();
			break;
		default:
			spdlog::debug("IK Transformer: Body Signal Type {} not currently implemented", int(bufferProperties.bodySignalType));
			return false;
			break;
		}


		// BufferProperties has to be populated from the json before this is called
		return true;
	}
	
	int IKTransformer::get_data(uint8_t* mem, sig::time_point& time, sig::SignalMemory & sigMem) {
		int retval = -1;

		switch (bufferProperties.bodySignalType) {
		case zed::Zed4SignalType::Body_34_KeypointsPlus:
			retval = ik_prediction<ZedSkeletonKPRot_34>(mem, time, sigMem);

			break;
		default:
			spdlog::debug("IK Transformer: Body Signal Type {} not currently implemented", int(bufferProperties.bodySignalType));
			retval = -1;
			break;
		}

		return retval;
	}


	void IKTransformer::set_local_idx(uint16_t idx) {
		local_idx = idx;
		spdlog::info("Transformer {}: Informed local user idx is {}", TRANSFORMER_NAME, local_idx);

	}


	void IKTransformer::shutdown() {
		switch (bufferProperties.bodySignalType) {
		case zed::Zed4SignalType::Body_34_KeypointsPlus:
			free_local_user_ptr<ZedSkeletonKPRot_34>();
			break;
		default:
			break;
		}
	}


	DYNALO_EXPORT bool SignalTransformerInitializeConsumer(const sig::SignalProperties& sigProp, const sig::SignalConsumerRuntimeConfig& cfg) {
		spdlog::debug("IK Transformer being initialized as consumer for sig {}", sigProp.signalType);
		//g_IKTransformer->initialize_signal(sigProp, cfg);
		return true;
	}


	// Get signal data and the time of generation. Return number of written bytes
	DYNALO_EXPORT int DYNALO_CALL GetSignalDataTransformer(uint8_t* mem, sig::time_point& time, sig::SignalMemory & sigMem) {		
		
		int rv = g_IKTransformer->get_data(mem, time, sigMem);	

		spdlog::debug("Transformer producing {} bytes", rv);
		return rv;
	}

	// Shutdown the signal producer (free resources/caches, etc)
	
	DYNALO_EXPORT void DYNALO_CALL SignalTransformerShutdownProducer() {
		g_IKTransformer->shutdown();
	}

	DYNALO_EXPORT void DYNALO_CALL SignalTransformerShutdownConsumer() {
	}
	

	DYNALO_EXPORT const char* DYNALO_CALL GetName() {
		return TRANSFORMER_NAME;
	}

	DYNALO_EXPORT void DYNALO_CALL SetLocalUserIdx(uint16_t idx) {
		g_IKTransformer->set_local_idx(idx);
	}

}