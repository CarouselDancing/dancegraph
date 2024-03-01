#pragma once

#include "message.h"

namespace net
{
	// A helper sync structure. Use it occasionally (from listeners/clients) to identify latency to server
	struct sync_t
	{
		// how many times left to sync
		int times_left = 100;
		// a
		uint64_t accum_ms = 0;
		// number of times we've send/received sync messages
		int accum_num=0;

		uint64_t delay_ms = 0;
		bool synced = false;

		// process response from the server. return if we should send another request!
		bool add(size_t ms)
		{
			accum_ms += ms;
			++accum_num;
			--times_left;
			bool done = times_left == 0;
			
			if (done)
			{
				delay_ms = accum_ms / accum_num;
				// reset values
				accum_ms = 0;
				std::swap(accum_num, times_left);
				synced = true;
			}
			return done;
		}
	};

	float time_offset_ms_ntp();
}