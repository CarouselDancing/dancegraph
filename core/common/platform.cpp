#include "platform.h"

#if defined(_WIN32)
#include <windows.h>
#include <libloaderapi.h>
#endif

#include <filesystem>

namespace common
{
	std::string GetExecutablePath()
	{
#if defined(_WIN32)
		char buf[_MAX_PATH];
		GetModuleFileNameA(NULL, buf, sizeof(buf));
		return std::string(buf);
#elif defined(__linux__)
		return std::filesystem::canonical("/proc/self/exe").string();
#endif
	}
}