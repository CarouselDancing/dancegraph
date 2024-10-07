
#if 0
#include "ipc/ringbuffer.h"
#include "latency_adapter.h"


using namespace ipc;

bool LatencyAdapter::read_ipc() {
	ipc::RBError err = ipcReader.read((void*)signalData);

	return (err == ipc::RBError::SUCCESS);
}
#endif
