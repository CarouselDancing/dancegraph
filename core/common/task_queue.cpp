#include "task_queue.h"

namespace dancegraph
{
	// It's safe to work on this list. It's not being used by the other thread
	void TaskQueue::AppendLocalTask(task_t task)
	{
		localTasks.push_back(task);
	}

	// Lock 
	bool TaskQueue::TryMoveLocalToShared()
	{
		bool moved = false;
		if (mutex.try_lock()) // this can return false, even if nobody else has acquired the mutex
		{
			tasks.insert(tasks.end(), localTasks.begin(), localTasks.end());
			mutex.unlock();
			localTasks.resize(0);
			moved = true;
		}
		return moved;
	}

	// return then number of actually consumed tasks
	std::pair<int,int> TaskQueue::TryConsumeTasks(int maxTasksToConsume)
	{
		int consumed = 0;
		int left = -1;
		if (mutex.try_lock()) // this can return false, even if nobody else has acquired the mutex
		{
			// consume the tasks
			for (auto task : tasks)
			{
				task();
				if (++consumed >= maxTasksToConsume)
					break;
			}
			// remove what's consumed
			if (consumed > 0)
				tasks.erase(tasks.begin(), tasks.begin() + consumed);
			left = int(tasks.size());
			mutex.unlock();
		}
		return std::make_pair(consumed,left);
	}
}