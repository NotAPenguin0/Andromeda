#include <andromeda/core/task_manager.hpp>

#include <andromeda/util/log.hpp>

namespace andromeda {

struct TaskLaunchStub {
	
};

TaskManager::TaskManager() {

}

void TaskManager::free_if_idle() {
	if (running_tasks == 0) {
		++ticks_with_no_tasks;
	}

	if (!idle && ticks_with_no_tasks >= idle_timer_max) {
		io::log("Task manager idle after {} ticks without running tasks. Freeing scheduler.", ticks_with_no_tasks);
		idle = true;
		deinit();
	}
}

void TaskManager::init() {
	scheduler = std::make_unique<ftl::TaskScheduler>();
	ftl::TaskSchedulerInitOptions options;
	options.Behavior = ftl::EmptyQueueBehavior::Sleep;
	scheduler->Init();
}

void TaskManager::deinit() {
	scheduler.reset(nullptr);
}

void TaskManager::on_task_start() {
	++running_tasks;
}

void TaskManager::on_task_end() {
	--running_tasks;
}

}