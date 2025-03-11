#pragma once

#include <mutex>
#include <stdint.h>
#include <thread>
#include <set>

namespace snmpfs {

	/**
	 * Abstract class representing a single Task.
	 * Tasks can either be one time execution SINGLE, or RECURRENT
	 */
	class Task
	{
	public:
		enum Status {
			WAITING,
			RUNNING,
			DONE
		};
		enum Type {
			SINGLE,
			RECURRENT
		};

		Task(Type type);
		virtual ~Task();

		Task::Status getStatus() const;
		Task::Type getType() const;

		uint32_t getInterval() const;
		void setInterval(uint32_t interval);
		uint64_t getLastUpdate() const;

	protected:
		uint32_t interval = 0;

	private:
		// TODO Make those values thread safe ?
		Status status = WAITING;
		const Type type;
		uint64_t lastUpdate = 0;

		void execute();
		virtual void run() = 0;

		friend class TaskManager;
	};

	/**
	* The TaskManagers job is to execute Tasks, expecially when they are RECURRENT
	*/
	class TaskManager
	{
	public:
		TaskManager();
		~TaskManager();

		void addTask(Task* task);
		void removeTask(Task* task);
		const std::set<Task*> getTasks() const;
		size_t size() const;
		bool isIdle() const;

		void start();
		void stop();
		void join();

		static uint64_t now();

	private:
		bool running = false;
		std::thread thread;
		mutable std::mutex taskMutex;
		std::set<Task*> tasks;

		void run();
	};

}	// namespace snmpfs
