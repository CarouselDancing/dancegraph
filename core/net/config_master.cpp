#include "config_master.h"

#include <filesystem>

#include <spdlog/spdlog.h>

#include <core/common/platform.h>
#include <core/common/utility.h>

#ifdef _WIN32
constexpr char kSharedLibraryExtension[] = "dll";
#endif

#ifdef linux
constexpr char kSharedLibraryExtension[] = "so";
#endif

namespace fs = std::filesystem;

namespace net
{
	namespace cfg
	{
		static Root root;

		const std::string& DanceGraphAppDataPath()
		{
			static std::string danceGraphAppdataPath;
			if (danceGraphAppdataPath.empty())
			{
				if (const char* appdata_path = std::getenv("LOCALAPPDATA"))
					danceGraphAppdataPath = std::string(appdata_path) + "/DanceGraph";
				else
					danceGraphAppdataPath = "~/.config/DanceGraph";
				if (!std::filesystem::exists(danceGraphAppdataPath))
					spdlog::error("DanceGraph data path does not exist in {}", danceGraphAppdataPath);
			}
			return danceGraphAppdataPath;
		}

		bool Root::load()
		{
			auto& root = const_cast<Root&>(instance());
			// First try the repository directory
			auto resources_path = fs::path(__FILE__).parent_path() / ".."/"resources";
			// Then try the local app data directory
			if (!fs::is_directory(resources_path))	
				resources_path = DanceGraphAppDataPath();
			if (!fs::is_directory(resources_path))
			{
				spdlog::error("Cannot file resources path at {}", resources_path.string());
				return false;
			}
			auto text = dancenet::readTextFile((resources_path / "dancegraph.json").string());
			if (text.empty())
			{
				spdlog::error("/resources/dancegraph.json is empty or doesn't exist");
				return false;
			}

			try
			{
				auto j = nlohmann::json::parse(text);
				if (j.empty())
				{
					spdlog::error("/resources/dancegraph.json contains nothing");
					return false;
				}
				root = j;
			}
			catch (nlohmann::json::exception e) {
				spdlog::info("dancegraph.json loading failed: {}", e.what());
				return false;
			}
			
			return true;
		}

		std::string Root::make_library_filename(const std::string& name) const
		{
			return DanceGraphAppDataPath() + "/" + dll_folder + "/" + name + "." + kSharedLibraryExtension;
		}
	}
}