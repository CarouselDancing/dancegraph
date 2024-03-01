#include "message.h"

#include <spdlog/spdlog.h>

#ifdef _WIN32
#include <Windows.h>
#include <Iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#endif

namespace net
{
	namespace msg
	{
		void _fallback(std::array<char, 8>& value) {
			std::random_device rd;
			uint32_t* uval = (uint32_t*)value.data(); // 8 bytes == 2 uints
			uval[0] = rd();
			uval[1] = rd();
		}

		void ClientID::populate(int portStart) {
#if 1
			PIP_ADAPTER_INFO AdapterInfo;
			DWORD dwBufLen = sizeof(IP_ADAPTER_INFO);

			// last 2 bytes are the port
			*(uint16_t*)&value[6] = *(const uint16_t*)&portStart;

			AdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));

			if (AdapterInfo == nullptr) {
				spdlog::error("Error allocating memory needed to call GetAdaptersinfo");
				_fallback(value);
			}

			// Make an initial call to GetAdaptersInfo to get the necessary size into the dwBufLen variable
			if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == ERROR_BUFFER_OVERFLOW) {
				free(AdapterInfo);
				AdapterInfo = (IP_ADAPTER_INFO*)malloc(dwBufLen);
				if (AdapterInfo == nullptr) {
					spdlog::error("Error allocating memory needed to call GetAdaptersinfo");
					_fallback(value);

				}
			}

			if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == NO_ERROR) {
				// Contains pointer to current adapter info

				// It's a linked list, but we only want one address

				if (AdapterInfo->AddressLength < 6) {
					spdlog::error("Error: addresslength of adapterinfo is too short");
					free(AdapterInfo);
					_fallback(value);
				}
				for (int i = 0; i < 6; i++)
					value[i] = AdapterInfo->Address[i];
				free(AdapterInfo);
			}
#else 
			_fallback(value);
#endif 
		}
	}
}