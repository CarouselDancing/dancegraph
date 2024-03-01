#include "signal_config.h"

#include <filesystem>
#include <spdlog/spdlog.h>

// Get the signal properties, to know what data to expect
DYNALO_EXPORT void DYNALO_CALL GetSignalProperties(sig::SignalProperties& sigProp)
{
	sigProp.set_all_sizes(sizeof(int));

	sigProp.toString = [](std::string& text, const uint8_t* mem, int size, sig::SignalStage stage) {
		// convert mem to an integer and convert to a string
		text = std::to_string(*(int32_t*)mem);
	};


}

namespace sig
{
	bool SignalLibraryConfig::init(const char* sharedLibraryPath)
	{
		if (!std::filesystem::exists(sharedLibraryPath))
		{
			spdlog::error("Signal library path \"{}\" does not exist", sharedLibraryPath);
			return false;
		}
		auto actual_path = get_unique_shared_library_path_for_loading(sharedLibraryPath);
		library = std::make_unique<dynalo::library>(actual_path);
		if (library)
		{
			fnGetSignalProperties = library->get_function<void(SignalProperties&)>("GetSignalProperties");
		}
		return library != nullptr;
	}
}