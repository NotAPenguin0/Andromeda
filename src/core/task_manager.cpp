#include <andromeda/core/task_manager.hpp>

namespace andromeda {

TaskManager::TaskManager() {
	scheduler.Init();
}

void TaskManager::launch(ftl::TaskFunction func, void* arg) {
	ftl::Task task{ func, arg };
	scheduler.AddTask(task);
}

}