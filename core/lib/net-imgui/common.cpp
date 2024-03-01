#include "common.h"

#include <imgui/imgui.h>
#include <spdlog/spdlog.h>
#include <magic_enum/magic_enum.hpp>

namespace imgui
{
	void GlobalLogLevel()
	{
		auto level = spdlog::get_level();
		if (Enum<spdlog::level::level_enum>("Global log level", level))
			spdlog::set_level(level);
	}
}