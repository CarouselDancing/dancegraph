

#define DYNALO_EXPORT_SYMBOLS

#include <dynalo/symbol_helper.hpp>
#include <dynalo/dynalo.hpp>

#include <spdlog/spdlog.h>
//#include <magic_enum/magic_enum.hpp>

#include "extrapolate.h"
#include "../env/env_common.h"

#include <core/lib/sig/signal_history.h>

#include <modules/zed/zed_common.h>

//using namespace sl;
using namespace std;

namespace zed {

	// g_SensorTransformer is stored as a pointer to delay construction until we've got arguments to construct with
	// (Since the ringbuffer has no default constructor, we can't store the actual class without a lot more hassle)
	unique_ptr<SensorTransformer> g_SensorTransformer;


	// Initialize the signal producer (alloc resources/caches, etc)
	DYNALO_EXPORT bool DYNALO_CALL SignalTransformerInitializeProducer(const sig::SignalProperties& sigProp)

	{	
		
		spdlog::set_default_logger(sigProp.logger);
		spdlog::debug("SignalProducerInitialize Called for Transformer");
		BufferProperties bProp = BufferProperties();
		spdlog::info("zedcam_producer.cpp Json properties: {}", sigProp.jsonConfig);
		bProp.populate_from_json(json::parse(sigProp.jsonConfig)); // Pass in the name of a config file if we want, otherwise it finds a default
		
		g_SensorTransformer = unique_ptr<SensorTransformer>(new SensorTransformer(bProp));
		spdlog::info("SignalProducerInitialize Ended");
		return g_SensorTransformer->init();

	}

	bool SignalTransformer::initialize_signal(const sig::SignalProperties& sigProp, const sig::SignalConsumerRuntimeConfig& cfg) {

		return true;
	}

	bool SensorTransformer::init() {

		// BufferProperties has to be populated from the json before this is called
		return true;
	}
	
	int SensorTransformer::get_data(uint8_t* mem, sig::time_point& time, sig::SignalMemory & sigMem) {
		int retval = 0;

		switch (bufferProperties.bodySignalType) {
		case zed::Zed4SignalType::Body_34_Compact:
			retval = extrapolate_rotation_data<ZedSkeletonCompact_34>(mem, time, sigMem);
			break;
		case zed::Zed4SignalType::Body_38_Compact:
			retval = extrapolate_rotation_data<ZedSkeletonCompact_38>(mem, time, sigMem);
			break;
		case zed::Zed4SignalType::Body_34_Full:
			retval = extrapolate_rotation_data<ZedSkeletonFull_34>(mem, time, sigMem);
			break;
		case zed::Zed4SignalType::Body_38_Full:
			retval = extrapolate_rotation_data<ZedSkeletonFull_38>(mem, time, sigMem);
			break;
		case zed::Zed4SignalType::Body_34_KeypointsPlus:
			retval = extrapolate_kprot_data<ZedSkeletonKPRot_34>(mem, time, sigMem);			
			break;
		case zed::Zed4SignalType::Body_38_KeypointsPlus:
			retval = extrapolate_kprot_data<ZedSkeletonKPRot_38>(mem, time, sigMem);
			break;
		case zed::Zed4SignalType::Body_34_Keypoints:
		case zed::Zed4SignalType::Body_38_Keypoints:				
		default:
			spdlog::debug("Extrapolation Transformer: Body Signal Type {} not currently implemented", int(bufferProperties.bodySignalType));
			retval = 0;
			break;
		}

		return retval;
	}

	void SensorTransformer::shutdown() {
	}


	DYNALO_EXPORT bool SignalTransformerInitializeConsumer(const sig::SignalProperties& sigProp, const sig::SignalConsumerRuntimeConfig& cfg) {
		spdlog::debug("Signal Transformer being initialized as consumer for sig {}", sigProp.signalType);
		g_SensorTransformer->initialize_signal(sigProp, cfg);
		return true;
	}


	// Get signal data and the time of generation. Return number of written bytes
	DYNALO_EXPORT int DYNALO_CALL GetSignalDataTransformer(uint8_t* mem, sig::time_point& time, sig::SignalMemory & sigMem) {		
		
		int rv = g_SensorTransformer->get_data(mem, time, sigMem);	

		spdlog::debug("Transformer producing {} bytes", rv);
		return rv;
	}

	// Shutdown the signal producer (free resources/caches, etc)
	
	DYNALO_EXPORT void DYNALO_CALL SignalTransformerShutdownProducer() {
		g_SensorTransformer->shutdown();
	}

	DYNALO_EXPORT void DYNALO_CALL SignalTransformerShutdownConsumer() {

	}

	const char * TRANSFORMER_NAME = "predict_simple/v1.0";
	
	DYNALO_EXPORT const char * DYNALO_CALL GetName() {
		return TRANSFORMER_NAME;
	}

}