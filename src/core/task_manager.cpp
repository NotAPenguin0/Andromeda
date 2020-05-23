#include <andromeda/core/task_manager.hpp>

namespace andromeda {

TaskManager::TaskManager() {
	ftl::TaskSchedulerInitOptions options;
	options.Behavior = ftl::EmptyQueueBehavior::Sleep;
	scheduler.Init();
}

void TaskManager::launch(ftl::TaskFunction func, void* arg) {
	ftl::Task task{ func, arg };
	scheduler.AddTask(task);
}

}