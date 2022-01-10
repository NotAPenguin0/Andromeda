#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>


namespace andromeda {
namespace thread {

/**
 * @var using task_id = uint32_t 
 * @brief Unique identifier type to represent tasks.
*/
using task_id = uint32_t;

/**
 * @var using task_function =  std::function<void(uint32_t)>
 * @brief Callable type for tasks. Takes in a single uint32_t parameter with the index of the
 *		  thread it's running on.
*/
using task_function = std::function<void(uint32_t /*thread_index*/)>;

/**
 * @class TaskScheduler 
 * @brief Thread pool over N threads used to schedule asynchronous tasks.
*/
class TaskScheduler {
public:


    /**
     * @brief Initialize the task scheduler.
     * @param num_threads The amount of concurrent threads. Recommended value is the amount of
     *		  available hardware threads given by std::thread::hardware_concurrency().
    */
    TaskScheduler(uint32_t num_threads);

    ~TaskScheduler();

    /**
     * @brief Schedule a task with a number of dependencies.
     * @param function A callable function with the task to execute.
     * @param dependencies A list of task identifiers with the dependencies of this task.
     * @return A unique identifier representing the task.
    */
    task_id schedule(task_function function, std::vector<task_id> dependencies = {});

    /**
     * @brief Check if a task is currently running.
     * @param task The id of the task to check.
     * @return true if the task is currently running in a thread.
    */
    bool is_running(task_id task);

    /**
     * @brief Check if a task is currently pending execution.
     * @param task The id of the task to check.
     * @return true if the task is currently pending execution.
    */
    bool is_pending(task_id task);

    /**
     * @brief Shuts down the task scheduler. Waits for all tasks to be completed.
    */
    void shutdown();

private:
    struct Task {
        task_id id;
        task_function function;
        std::vector<task_id> dependencies;
    };

    // The thread pool
    std::vector<std::thread> threads;
    // All tasks pending execution.
    std::vector<Task> pending_tasks;
    // All tasks currently executing
    std::vector<Task> running_tasks;
    // Mutex protecting the above vectors.
    mutable std::mutex task_mutex;
    // This condition variable will be used to notify the threads that a new task is available.
    std::condition_variable cv;
    // Flag that will indicate to the threads that the pool should be terminated
    std::atomic<bool> terminate;
    // Flag indicating that the thread pool is stopped.
    std::atomic<bool> stopped;

    bool is_running(task_id task, std::unique_lock<std::mutex>& lock);

    bool is_pending(task_id task, std::unique_lock<std::mutex>& lock);
};

} // namespace thread
} // namespace andromeda