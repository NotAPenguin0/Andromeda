#ifndef ANDROMEDA_TASK_MANAGER_HPP_
#define ANDROMEDA_TASK_MANAGER_HPP_

#include <ftl/task_scheduler.h>
#include <memory>
#include <functional>

#include <andromeda/util/log.hpp>

namespace andromeda {

class TaskManager;

template<typename T>
struct TaskFunctionType {
	using type = std::function<void(ftl::TaskScheduler*, T)>;
};

template<>
struct TaskFunctionType<void> {
	using type = std::function<void(ftl::TaskScheduler*)>;
};

template<typename T>
using TaskFunction = typename TaskFunctionType<T>::type;

namespace detail {

template<typename T>
struct TaskLaunchStub {
	TaskFunction<T> func;
	T arg;
	TaskManager& manager;
};

template<>
struct TaskLaunchStub<void> {
	TaskFunction<void> func;
	TaskManager& manager;
};

template<typename T>
void task_func(ftl::TaskScheduler* scheduler, void* arg);
template<>
void task_func<void>(ftl::TaskScheduler* scheduler, void* arg); 
template<typename T>
void do_launch(TaskLaunchStub<T>* launch_stub);

} // namespace detail 

class TaskManager {
public:
	TaskManager();

	template<typename T>
	void launch(TaskFunction<T> func, T arg) {
		if (idle) {
			io::log("Resuming idle task manager because a new task was launched. Total idle time was {} ticks.", ticks_with_no_tasks);
			idle = false;
			init();
		}

		ticks_with_no_tasks = 0;

		detail::TaskLaunchStub<T>* launch_stub = new detail::TaskLaunchStub<T>{ std::move(func), std::move(arg), *this };
		detail::do_launch(launch_stub);
	}

	void launch(TaskFunction<void> func) {
		if (idle) {
			io::log("Resuming idle task manager because a new task was launched. Total idle time was {} ticks.", ticks_with_no_tasks);
			idle = false;
			init();
		}

		ticks_with_no_tasks = 0;

		detail::TaskLaunchStub<void>* launch_stub = new detail::TaskLaunchStub<void>{ std::move(func), *this };
		detail::do_launch(launch_stub);
	}

	ftl::TaskScheduler& get_scheduler() { return *scheduler; }

	void free_if_idle();

private:
	void init();
	void deinit();

	void on_task_start();
	void on_task_end();

	template<typename T>
	friend void detail::task_func(ftl::TaskScheduler* scheduler, void* arg);

	std::unique_ptr<ftl::TaskScheduler> scheduler;
	std::atomic<size_t> running_tasks = 0;

	size_t ticks_with_no_tasks = 0;
	static constexpr size_t idle_timer_max = 240;
	bool idle = true;
};

namespace detail {

template<typename T>
void task_func(ftl::TaskScheduler* scheduler, void* arg) {
	auto original_launch_stub = (TaskLaunchStub<T>*)(arg);
	original_launch_stub->manager.on_task_start();
	original_launch_stub->func(scheduler, original_launch_stub->arg);
	original_launch_stub->manager.on_task_end();

	// Cleanup the launch data
	delete original_launch_stub;
}

template<>
inline void task_func<void>(ftl::TaskScheduler* scheduler, void* arg) {
	auto original_launch_stub = (TaskLaunchStub<void>*)(arg);
	original_launch_stub->manager.on_task_start();
	original_launch_stub->func(scheduler);
	original_launch_stub->manager.on_task_end();

	// Cleanup the launch data
	delete original_launch_stub;
}

template<typename T>
void do_launch(TaskLaunchStub<T>* launch_stub) {
	ftl::Task task{ task_func<T>, (void*)launch_stub };
	launch_stub->manager.get_scheduler().AddTask(task);
}

}

}

#endif