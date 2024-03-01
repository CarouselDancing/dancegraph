#include "sync.h"

#include <ntp_client/ntp.h>

namespace net
{
	using namespace CTU::VIN::NTP_client;

	float time_offset_ms_ntp()
	{
		spdlog::info("Running time_offset_ms_ntp, despite everything");
		const int kNumAttempts = 20;
		enum Status s;
		struct Result* result = nullptr;
		HNTP client = Client__create();
		const char* hostname = "130.88.202.49";
		//const char* hostname = "195.113.144.201";

#if 0	// simple average
		double offset_sum = 0;
		int offset_count = 0;
		for (int i = 0; i < kNumAttempts; ++i)
		{
			if (Client__query(client, hostname, &result) == Status::OK)
			{
				offset_sum += result->mtr.offset_ns * 0.000001;
				++offset_count;
			}
		}
		float offset = offset_count > 0 ? float(offset_sum / offset_count) : std::numeric_limits<float>::infinity();
#else	// median
		float offset = std::numeric_limits<float>::infinity();
		std::vector<double> offsets;
		offsets.reserve(kNumAttempts);
		for (int i = 0; i < kNumAttempts; ++i)
		{
			if (Client__query(client, hostname, &result) == Status::OK)
				offsets.push_back(result->mtr.offset_ns * 0.000001);
		}
		if (!offsets.empty())
		{
			std::sort(offsets.begin(), offsets.end());
			offset = offsets[offsets.size()/2];
		}
#endif

		if (result != nullptr)
			Client__free_result(result);
		else
			spdlog::warn("ntp result was null -- could not free");
		if (client != nullptr)
			Client__close(client);
		else
			spdlog::warn("ntp client was null -- could not close");
		return offset;

	}
}