#include <andromeda/thread/scheduler.hpp>

#include <andromeda/util/idgen.hpp>
#include <andromeda/app/log.hpp>

namespace andromeda {
namespace thread {

TaskScheduler::TaskScheduler(uint32_t num_threads) {
	// This function will continuously run on every thread, waiting to accept tasks.
	auto thread_loop_function = [this](uint32_t thread_index) {
		while (true) {
			Task job{};
			{
				// Acquire the mutex
				std::unique_lock lock{ task_mutex };
				// Wait until there is a task available or until the thread pool should be stopped.
				cv.wait(lock, [this]() {
					return !pending_tasks.empty() || terminate;
				});

				// If the terminate flag is set and there is no new task, break out of the loop
				if (pending_tasks.empty() && terminate) break;

				// Find the first task that has all its dependencies completed and execute it.
				for (auto it = pending_tasks.begin(); it != pending_tasks.end(); ++it) {
					bool dependencies_complete = true;
					for (task_id dep : it->dependencies) {
						if (is_running(dep, lock) || is_pending(dep, lock)) {
							dependencies_complete = false;
							break;
						}
					}
					if (dependencies_complete) {
						job = std::move(*it);
						pending_tasks.erase(it);
						running_tasks.push_back(job);
						break;
					}
				}
			}

			// Run the task (if one was found)
			if (job.function) {
				job.function(thread_index);
				// Remove this task from the running task list
				{
					std::unique_lock lock{ task_mutex };
					running_tasks.erase(std::remove_if(running_tasks.begin(), running_tasks.end(),
						[&job](Task const& t) {
							return job.id == t.id;
						}), running_tasks.end());
				}
				// Since the task is complete we want to wake up all threads, because this task might be 
				// a dependency to a bunch of other tasks.
				cv.notify_all();
			}
		}
	};

	threads.reserve(num_threads);
	for (uint32_t i = 0; i < num_threads; ++i) {
		threads.push_back(std::thread{thread_loop_function, i});
	}
}

TaskScheduler::~TaskScheduler() {
	if (!stopped) {
		shutdown();
	}
}

task_id TaskScheduler::schedule(task_function function, std::vector<task_id> dependencies) {
	if (stopped) {
		LOG_FORMAT(LogLevel::Error, "Tried to schedule a task but scheduler is stopped.");
		return -1;
	}

	task_id id = IDGen<Task, task_id>::next();
	{
		// Acquire lock and schedule the task
		std::unique_lock lock{ task_mutex };
		pending_tasks.push_back(Task{ 
			.id = id, 
			.function = std::move(function), 
			.dependencies = std::move(dependencies) 
		});
		// Notify a single thread that a new task is availble
		cv.notify_one();
	}
	return id;
}

bool TaskScheduler::is_running(task_id task) {
	std::unique_lock lock{ task_mutex };
	return is_running(task, lock);
}

bool TaskScheduler::is_pending(task_id task) {
	std::unique_lock lock{ task_mutex };
	return is_pending(task, lock);
}

void TaskScheduler::shutdown() {
	// Note that we don't need a lock here since terminate is atomic
	terminate = true;
	// Notify all threads that the terminate flag is set
	cv.notify_all();
	// Join all threads to wait for tasks to be completed
	for (std::thread& thread : threads) {
		thread.join();
	}
	threads.clear();
	// Set stopped flag
	stopped = true;

	LOG_FORMAT(LogLevel::Info, "Shutting down task scheduler");
}

bool TaskScheduler::is_running(task_id task, std::unique_lock<std::mutex>& lock) {
	auto it = std::find_if(running_tasks.begin(), running_tasks.end(),
		[task](Task const& t) {
			return task == t.id;
		}
	);
	return it != running_tasks.end();
}

bool TaskScheduler::is_pending(task_id task, std::unique_lock<std::mutex>& lock) {
	auto it = std::find_if(pending_tasks.begin(), pending_tasks.end(),
		[task](Task const& t) {
			return task == t.id;
		}
	);
	return it != pending_tasks.end();
}

} // namespace thread
} // namespace andromeda