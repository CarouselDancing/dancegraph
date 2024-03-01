#include <random>
#include <spdlog/spdlog.h>
#include <core/common/task_queue.h>

using namespace std::chrono_literals;
using namespace dancegraph;

TaskQueue adapterTaskList;
TaskQueue nativeTaskList;

void tick(TaskQueue& inputQueue, TaskQueue& outputQueue, const char * label)
{
	auto [numConsumed,numLeft] = inputQueue.TryConsumeTasks();
	if(numLeft >= 0)
		spdlog::info("{0}: consumed {1} left {2}", label, numConsumed, numLeft);
	else
		spdlog::error("{0}: Failed to get the lock to consume tasks", label, numConsumed, numLeft);
	int numTasks = rand() % 5;
	for (int i = 0; i < numTasks; ++i)
	{
		std::this_thread::sleep_for( 2ms ); // a few ms delay between each task appending
		outputQueue.AppendLocalTask([]() {});
	}
	spdlog::info("{0}: appended {1}", label, numTasks);
	auto moved = outputQueue.TryMoveLocalToShared();
	if(moved)
		spdlog::info("{0}: moved local to shared", label);
	else
		spdlog::error("{0}: DID NOT MOVE local to shared", label);
}

void native_tick()
{
	while (true)
	{
		std::this_thread::sleep_for(5ms);  // fast
		tick(adapterTaskList, nativeTaskList, "native");
	}
}

void adapter_tick()
{
	while (true)
	{
		std::this_thread::sleep_for(160ms);  // slow
		tick(nativeTaskList, adapterTaskList, "\t\t\t\tadapter");
	}
}

int main(int argc, char** argv)
{
	std::thread tn(native_tick);
	std::thread ta(adapter_tick);

	tn.join();
	ta.join();
	return 0;
}
