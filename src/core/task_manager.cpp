#include <andromeda/core/task_manager.hpp>

#include <andromeda/util/log.hpp>

namespace andromeda {

struct TaskLaunchStub {
	
};

TaskManager::TaskManager() {

}

void TaskManager::check_task_status() {
	if (waiting_tasks.empty()) return;
	std::lock_guard lock(waiting_mutex);

	for (auto it = waiting_tasks.begin(); it != waiting_tasks.end(); ++it) {
		auto const& waiting_task = *it;
		if (waiting_task.poll_func() == TaskStatus::Completed) {
			waiting_task.cleanup_func();
			it = waiting_tasks.erase(it);
			if (it == waiting_tasks.end()) break;
		}
	}
}

void TaskManager::wait_task(std::function<TaskStatus()> status_callback, std::function<void()> cleanup_callback) {
	std::lock_guard lock(waiting_mutex);
	waiting_tasks.push_back({ status_callback, cleanup_callback });
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

void TaskManager::resume_if_idle() {
	if (idle) {
		io::log("Resuming idle task manager because a new task was launched. Total idle time was {} ticks.", ticks_with_no_tasks);
		idle = false;
		init();
	}

	ticks_with_no_tasks = 0;
}

}