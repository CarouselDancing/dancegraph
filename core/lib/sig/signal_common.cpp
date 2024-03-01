#include "signal_common.h"

#include <cassert>
#include <cstring>

#include <spdlog/spdlog.h>

#include <iostream>

#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>

namespace sig
{
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SignalIpcProperties, name, numEntries);

	time_point time_now()
	{
		return time_point::clock::now();
	}

	std::string time_point_as_string(const sig::time_point& tp) {
#if 0

		std::time_t t = sig::time_point::clock::to_time_t(tp);
		std::string ts = std::ctime(&t);
		ts.resize(ts.size() - 1);
#else
		// Convert the current time to microseconds since the epoch
		auto microsecondsSinceEpoch = std::chrono::time_point_cast<std::chrono::microseconds>(tp).time_since_epoch().count();

		// Convert microseconds to milliseconds, seconds, minutes, hours, days, etc.
		auto milliseconds = microsecondsSinceEpoch / 1000;
		auto seconds = milliseconds / 1000;
		auto minutes = seconds / 60;
		auto hours = minutes / 60;
		auto days = hours / 24;

		// Extract individual components
		auto micros = microsecondsSinceEpoch % 1000;
		auto millis = milliseconds % 1000;
		auto secs = seconds % 60;
		auto mins = minutes % 60;
		auto hrs = hours % 24;

		// Get current date
		auto currentTimeStruct = std::chrono::system_clock::to_time_t(tp);
		auto tmStruct = *std::localtime(&currentTimeStruct);
		// Print in standard format: year/month/day hour:minute:second.millisecond.microsecond
		std::stringstream ss;
		ss << std::put_time(&tmStruct, "%Y/%m/%d %H:%M:%S.") << std::setw(3) << std::setfill('0') << millis << std::setw(3) << std::setfill('0') << micros;
		auto ts = ss.str();

#endif
		return ts;
	}

	double duration_sec(const sig::time_point& t0, const sig::time_point& t1)
	{
		return std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0).count();
	}

	int SignalProperties::data_xform_memcpy(const uint8_t* src, int srcSize, uint8_t* dst, int dstMaxSize, const void* userData)
	{
		assert(srcSize <= dstMaxSize);
		memcpy(dst, src, srcSize);
		return srcSize;
	};

	void SignalProperties::set_all_sizes(int datasize)
	{		
		spdlog::warn("SET ALL SIZES CALLED WITH SIZE: {}", datasize);
		producerFormMaxSize = datasize;
		consumerFormMaxSize = datasize;
		networkFormMaxSize = datasize;
		stateFormMaxSize = keepState ? datasize : 0;

		consumerFormMaxSizeWithHeader = datasize + sizeof(SignalMetadata);
		networkFormMaxSizeWithHeader = datasize + sizeof(SignalMetadata);

	}


	std::string makeHex(const uint8_t* buffer, size_t size) {
		fmt::memory_buffer output;
		for (size_t i = 0; i < size; ++i) {
			fmt::format_to(fmt::appender(output), "{:02X} ", static_cast<unsigned char>(buffer[i]));
		}
		return fmt::format("{}", output.data());
	}

	void SignalProperties::to_string_default(std::string& text, const uint8_t* buffer, int size, SignalStage stage)
	{
		text = fmt::format("[stage {}, size {}]: ", magic_enum::enum_name(stage), size, makeHex(buffer, size));
	}


	std::string get_unique_shared_library_path_for_loading(const std::string& path)
	{
		static std::unordered_map<std::string, int> loaded_files;
		auto it = loaded_files.find(path);
		if (it == loaded_files.end())
		{
			loaded_files[path] = 1;
			return path;
		}
		else
		{
			namespace fs = std::filesystem;
			auto& count = it->second;
			auto idx = path.rfind('.');
			auto path_out = path;
			path_out.insert(idx, fmt::format("-{}", count++));
			const auto copyOptions = fs::copy_options::update_existing
				| fs::copy_options::overwrite_existing;
			std::error_code ec;
			std::filesystem::copy(path, path_out, copyOptions, ec);
			if (ec)
				spdlog::error("Error copying a shared library to a unique new path: {}", ec.message());
			return path_out;
		}
	}
}
