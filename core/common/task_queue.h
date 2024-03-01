#pragma once

#include <functional>
#include <mutex>
#include <vector>

namespace dancegraph
{
	/*
		Very simple tasks queue.
		WARNING: Tasks execute non-thread-safe code
			a native task that is consumed by the adapter should not access shared memory from the native plugin
			an adapter task that is consumed by the native code should not access shared memory from the adapter
	 
		Usage:
	 
			producer tick()
				// try and consume any tasks provided by the media framework, e.g. manipulating env signals.
				//		if we fail, try later
				adapterTaskList.TryConsumeTasks()
				...
				// append any tasks to a local task list, e.g. local HW signals or remote env signals
				nativeTaskList.AppendLocal( task )
				...
				// Try to move the local tasks to the shared task list
				//		If we fail, try later
				nativeTaskList.TryMoveLocalToShared()

			media framework tick()

				// try and consume any tasks provided by the native plugin, e.g. syncing remote env signals, or using local HW signals
				//		If we fail, try later
				nativeTaskList.TryConsumeTasks()
				...
				// append any tasks to a local task line, e.g. manipulating env signals
				adapterTaskList.AppendLocal( task )
				...
				// Try to move the local tasks to the shared task list
				//		If we fail, try later
				adapterTaskList.TryMoveLocalToShared()

		See tasks_queue_test.cpp for a simple example
	*/


	using task_t = std::function<void()>;

	class TaskQueue
	{
	public:
		static constexpr int kMaxTasksToConsume = 10;

		void AppendLocalTask(task_t task);
		// return then number of actually consumed tasks, and what's left
		std::pair<int,int> TryConsumeTasks(int maxTasksToConsume = kMaxTasksToConsume);
		bool TryMoveLocalToShared();
		void Clear() { tasks.clear(); localTasks.clear(); }
	private:
		std::mutex mutex;
		std::vector<task_t> tasks;
		std::vector<task_t> localTasks;
	};
}