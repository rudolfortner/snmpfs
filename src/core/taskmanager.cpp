#include "core/taskmanager.h"

namespace snmpfs {

	Task::Task(Type type) : type(type)
	{

	}

	Task::~Task()
	{

	}

	Task::Status Task::getStatus() const
	{
		return status;
	}

	Task::Type Task::getType() const
	{
		return type;
	}

	uint32_t Task::getInterval() const
	{
		return interval;
	}

	void Task::setInterval(uint32_t interval)
	{
		this->interval = interval;
	}

	uint64_t Task::getLastUpdate() const
	{
		return lastUpdate;
	}

	void Task::execute()
	{
		run();
		lastUpdate = TaskManager::now();
		if(type == SINGLE)			status = DONE;
		else if(type == RECURRENT)	status = WAITING;
		else std::runtime_error("Undefined");
	}



	TaskManager::TaskManager()
	{

	}

	TaskManager::~TaskManager()
	{
		std::unique_lock<std::mutex> lock(taskMutex);

		// DELETE ALL TASKS LEFT NOBODY CARED UNTIL NOW
		for(Task* task : tasks)
		{
			delete task;
		}
		tasks.clear();
	}

	void TaskManager::addTask(Task* task)
	{
		std::unique_lock<std::mutex> lock(taskMutex);
		tasks.insert(task);
	}

	void TaskManager::removeTask(Task* task)
	{
		std::unique_lock<std::mutex> lock(taskMutex);
		tasks.erase(task);
	}

	const std::set<Task*> TaskManager::getTasks() const
	{
		std::unique_lock<std::mutex> lock(taskMutex);
		return tasks;
	}

	size_t TaskManager::size() const
	{
		std::unique_lock<std::mutex> lock(taskMutex);
		return tasks.size();
	}

	bool TaskManager::isIdle() const
	{
		std::unique_lock<std::mutex> lock(taskMutex);
		for(const Task* task : tasks)
		{
			if(task->status == Task::RUNNING)
				return false;
		}
		return true;
	}



	void TaskManager::start()
	{
		if(running)
			throw std::runtime_error("TaskManager already running!");
		running	= true;
		thread	= std::thread(&TaskManager::run, this);
	}

	void TaskManager::stop()
	{
		if(!running)
			throw std::runtime_error("TaskManager not running!");
		running	= false;
	}

	void TaskManager::join()
	{
		thread.join();
		// TODO Don't know if this solution is best... DeviceInitTask blocks very long...
		while(!isIdle())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}


	void TaskManager::run()
	{
		while(running)
		{
			taskMutex.lock();
			for(Task* task : tasks)
			{
				if(task->status != Task::WAITING)
					continue;

				uint64_t nextUpdate = task->lastUpdate + 1000 * task->interval;
				if(nextUpdate <= now())
				{
					task->status = Task::RUNNING;
					// use detached thread instead of std::async to avoid blocking destructor of future
					std::thread(&Task::execute, task).detach();
				}
			}
			taskMutex.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

	uint64_t TaskManager::now()
	{
		auto start = std::chrono::system_clock::now();
		auto epoch = start.time_since_epoch();
		auto milli = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();
		return milli;
	}

}	// namespace snmpfs
