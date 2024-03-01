#if 0

#include <ipc/ringbuffer.h>


class LatencyAdapter {
	bool read_ipc();

protected:
	ipc::DNRingbufferReader ipcReader;
};

#endif