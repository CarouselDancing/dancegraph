#include "zed_signal_config.h"

#include <array>

#include <spdlog/fmt/fmt.h>
#include <modules/zed/zed_common.h>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

using namespace zed;
using json = nlohmann::json;




template <>
struct fmt::formatter<vec3> : fmt::formatter<std::string> {
	auto format(const vec3& a, format_context& ctx) {
		return formatter<std::string>::format(fmt::format("({},{},{})", a.x,a.y,a.z), ctx);
	}
};

template <>
struct fmt::formatter<quat> : fmt::formatter<std::string> {
	auto format(const quat& a, format_context& ctx) {
		return formatter<std::string>::format(fmt::format("({},{},{},{})", a.x, a.y, a.z, a.w), ctx);
	}
};

template <>
struct fmt::formatter<quant_quat> : fmt::formatter<std::string> {
	auto format(const quant_quat& a, format_context& ctx) {
		return formatter<std::string>::format(fmt::format("({},{},{})", a.x, a.y, a.z), ctx);
	}
};


template <>
struct fmt::formatter<xform> : fmt::formatter<std::string> {
	auto format(const xform& a, format_context& ctx) {
		return formatter<std::string>::format(fmt::format("(pos={}, ori={})", a.pos, a.ori), ctx);
	}
};

template <>
struct fmt::formatter<xform_compact> : fmt::formatter<std::string> {
	auto format(const xform_compact& a, format_context& ctx) {
		return formatter<std::string>::format(fmt::format("(pos={}, ori={})", a.pos, a.ori), ctx);
	}
};
template <>
struct fmt::formatter<xform_quant> : fmt::formatter<std::string> {
	auto format(const xform_quant& a, format_context& ctx) {
		return formatter<std::string>::format(fmt::format("(pos={}, ori={})", a.pos, a.ori), ctx);
	}
};



template <class T, int N>
struct fmt::formatter<std::array<T, N>> : fmt::formatter<std::string> {
	auto format(const std::array<T, N>& a, format_context& ctx) {
		std::string s = "(";
		for (size_t i = 0; i < N; ++i)
			s += fmt::format("[{}]={},", i, a[i]);
		return formatter<std::string>::format(s + ")", ctx);
	}
};


template <>
struct fmt::formatter<Zed4Header> : fmt::formatter<std::string> {
	auto format(const Zed4Header& a, format_context& ctx) {
		return formatter<std::string>::format(fmt::format("([t={},{},{}] num_skeletons={})", a.grab_delta, a.tracking_delta, a.processing_delta, a.num_skeletons), ctx);
	}
};

/*
template <>
struct fmt::formatter<ZedSkeleton> : fmt::formatter<std::string> {
	auto format(const ZedSkeleton& a, format_context& ctx) {
		return formatter<std::string>::format(fmt::format("(id={}, root_transform={}, bone_data=TBD)", a.id, a.root_transform), ctx);
	}
};
*/

/* TODO: These should be writable with one piece of code! */


template <>
struct fmt::formatter<ZedSkeletonCompact_34> : fmt::formatter<std::string> {
	auto format(const ZedSkeletonCompact_34& a, format_context& ctx) {
		return formatter<std::string>::format(fmt::format("(id={}, root_transform={}, bone_rotations={})", a.id, a.root_transform, a.bone_rotations), ctx);
	}
};

template <>
struct fmt::formatter<ZedSkeletonCompact_38> : fmt::formatter<std::string> {
	auto format(const ZedSkeletonCompact_38& a, format_context&  ctx) {
		return formatter<std::string>::format(fmt::format("(id={}, root_transform={}, bone_rotations={})", a.id, a.root_transform, a.bone_rotations), ctx);
	}
};

template <>
struct fmt::formatter<ZedSkeletonFull_34> : fmt::formatter<std::string> {
	auto format(const ZedSkeletonFull_34& a, format_context& ctx) {
		return formatter<std::string>::format(fmt::format("(id={}, root_transform={}, bone_rotations={})", a.id, a.root_transform, a.bone_rotations), ctx);
	}
};

template <>
struct fmt::formatter<ZedSkeletonFull_38> : fmt::formatter<std::string> {
	auto format(const ZedSkeletonFull_38& a, format_context& ctx) {
		return formatter<std::string>::format(fmt::format("(id={}, root_transform={}, bone_rotations={})", a.id, a.root_transform, a.bone_rotations), ctx);
	}
};



BufferProperties bufferProperties;


// Get the signal properties, to know what data to expect
DYNALO_EXPORT void DYNALO_CALL GetSignalProperties(sig::SignalProperties& sigProp)
{
	spdlog::info("Zedcam Signal Property JSON:{}", sigProp.jsonConfig);

	json optstext = nlohmann::json::parse(sigProp.jsonConfig);

	// We only want to read the max body count from the config file once
	if (!bufferProperties.populated)
		//bufferProperties.populate_from_json(sigProp.jsonConfig);
		bufferProperties.populate_from_json(optstext);


	sigProp.keepState = true;

	int num_skeletons;


	switch (bufferProperties.bodySignalType) {

	case zed::Zed4SignalType::Body_38_Compact:
		sigProp.set_all_sizes(ZedBodies<ZedSkeletonCompact_38>::size(bufferProperties.maxBodyCount)); break;
	case zed::Zed4SignalType::Body_34_Full:
		sigProp.set_all_sizes(ZedBodies<ZedSkeletonFull_34>::size(bufferProperties.maxBodyCount)); break;
	case zed::Zed4SignalType::Body_38_Full:
		sigProp.set_all_sizes(ZedBodies<ZedSkeletonFull_38>::size(bufferProperties.maxBodyCount)); break;
	case zed::Zed4SignalType::Body_38_KeypointsPlus:
		sigProp.set_all_sizes(ZedBodies<ZedSkeletonKPRot_38>::size(bufferProperties.maxBodyCount)); break;
	case zed::Zed4SignalType::Body_34_KeypointsPlus:
		sigProp.set_all_sizes(ZedBodies<ZedSkeletonKPRot_34>::size(bufferProperties.maxBodyCount)); break;

	case zed::Zed4SignalType::Body_34_Compact: default:
		spdlog::info("Setting size with base {} + metadata {} = {}", ZedBodies<ZedSkeletonCompact_34>::size(bufferProperties.maxBodyCount), sizeof(sig::SignalMetadata),
			ZedBodies<ZedSkeletonCompact_34>::size(bufferProperties.maxBodyCount) + sizeof(sig::SignalMetadata));
		sigProp.set_all_sizes(ZedBodies<ZedSkeletonCompact_34>::size(bufferProperties.maxBodyCount)); break;
	}


	// TODO : Make the templates work properly with fmt::formatter

	/*
	sigProp.toString = [](std::string& text, const uint8_t* mem, int size, sig::SignalStage stage) {
		const auto& header = *(Zed4Header*)mem;

		text = fmt::format("header={}, data=", header);

		const ZedSkeleton* data = (ZedSkeleton*)&header.num_skeletons;
		for (int i = 0; i < header.num_skeletons; i++) {
			text += fmt::format("[{}]={}, ", i, data[i]);
		}
	};
}

*/

	sigProp.toString = [](std::string& text, const uint8_t* mem, int size, sig::SignalStage stage) {
		const auto& header = *(Zed4Header*)mem;

		text = fmt::format("header={}, data=", header);


		switch (bufferProperties.bodySignalType) {
		case zed::Zed4SignalType::Body_38_Compact: {
			const ZedSkeletonCompact_38* data1 = (ZedSkeletonCompact_38*)&header.skeletons;
			for (int i = 0; i < header.num_skeletons; i++)
				text += fmt::format("[{}]={}, ", i, data1[i]);
			break;
		}

		case zed::Zed4SignalType::Body_34_Full: {
			const ZedSkeletonFull_34* data2 = (ZedSkeletonFull_34*)&header.skeletons;
			for (int i = 0; i < header.num_skeletons; i++)
				text += fmt::format("[{}]={}, ", i, data2[i]);
			break;
		}

		case zed::Zed4SignalType::Body_38_Full: {
			const ZedSkeletonFull_38* data3 = (ZedSkeletonFull_38*)&header.skeletons;
			for (int i = 0; i < header.num_skeletons; i++)
				text += fmt::format("[{}]={}, ", i, data3[i]);
			break;
		}

		case zed::Zed4SignalType::Body_34_Compact: default: {
			const ZedSkeletonCompact_34* data4 = (ZedSkeletonCompact_34*)&header.skeletons;
			for (int i = 0; i < header.num_skeletons; i++)
				text += fmt::format("[{}]={}, ", i, data4[i]);
			break;

		}
		}
	};
}
