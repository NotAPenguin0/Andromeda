#ifndef ANDROMEDA_TASK_MANAGER_HPP_
#define ANDROMEDA_TASK_MANAGER_HPP_

#include <ftl/task_scheduler.h>
#include <functional>

namespace andromeda {

class TaskManager {
public:
	TaskManager();

	void launch(ftl::TaskFunction func, void* arg = nullptr);
	ftl::TaskScheduler& get_scheduler() { return scheduler; }
private:
	ftl::TaskScheduler scheduler;
};

}

#endif