#pragma once

#include <chrono>
#include <mutex>
#include <stdint.h>
#include <vector>

/**
 * Timed Queue
 * Values are added to the queue with a specified time value at which they expire.
 * Until this time is reached they can't be retrieved from the queue.
 */
template <typename T>
class tqueue {

private:
	struct tqueue_entry {
		uint64_t delay;
		uint64_t time;
		T* value;
	};

public:
	tqueue()
	{

	}

	bool empty()
	{
		std::unique_lock<std::mutex> lock(mutex);
		return entries.empty();
	}

	T* awaitNext()
	{
		T* nextT;
		uint64_t delay;
		next(&nextT, &delay);
		return nextT;
	}

	bool awaitNext(T** valuePtr, uint64_t* delayPtr)
	{
		while(true)
		{
			std::unique_lock<std::mutex> lock(mutex);

			if(entries.empty())
			{
				*valuePtr = nullptr;
				*delayPtr = 0;
				return false;
			}

			const tqueue_entry& entry = entries[0];
			if(entry.time <= now())
			{
				*delayPtr = entry.delay;
				*valuePtr = entry.value;
				entries.erase(entries.begin());
				return true;
			}

			lock.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

	T* next()
	{
		T* nextT;
		uint64_t delay;
		next(&nextT, &delay);
		return nextT;
	}

	bool next(T** valuePtr, uint64_t* delayPtr)
	{
		std::unique_lock<std::mutex> lock(mutex);

		if(entries.empty())
		{
			*valuePtr = nullptr;
			*delayPtr = 0;
			return false;
		}

		const tqueue_entry& entry = entries[0];
		if(entry.time <= now())
		{
			*delayPtr = entry.delay;
			*valuePtr = entry.value;
			entries.erase(entries.begin());
			return true;
		}
		else
		{
			*delayPtr = 0;
			*valuePtr = nullptr;
			return false;
		}
	}

	T* pop()
	{
		T* nextT;
		uint64_t delay;
		pop(&nextT, &delay);
		return nextT;
	}

	bool pop(T** valuePtr, uint64_t* delayPtr)
	{
		std::unique_lock<std::mutex> lock(mutex);

		if(entries.empty())
		{
			*valuePtr = nullptr;
			*delayPtr = 0;
			return false;
		}

		const tqueue_entry& entry = entries[0];
		*delayPtr = entry.delay;
		*valuePtr = entry.value;
		entries.erase(entries.begin());
		return true;
	}

	bool waiting() const
	{
		std::unique_lock<std::mutex> lock(mutex);
		if(entries.empty())
			return false;

		const tqueue_entry& entry = entries[0];
		return entry.time > now();
	}

	void push(T* value)
	{
		std::unique_lock<std::mutex> lock(mutex);
		entries.emplace_back(0, now(), value);
		sort();
	}

	void pushAt(T* value, uint64_t time)
	{
		std::unique_lock<std::mutex> lock(mutex);
		entries.emplace_back(time - now(), time, value);
		sort();
	}

	void pushIn(T* value, uint64_t delay)
	{
		std::unique_lock<std::mutex> lock(mutex);
		entries.emplace_back(delay, now() + delay, value);
		sort();
	}

	void print()
	{
		std::unique_lock<std::mutex> lock(mutex);

		for(const tqueue_entry& entry : entries)
		{
			printf("%16lu: %p\n", entry.time, (void*) entry.value);
		}
	}

	static uint64_t now()
	{
		auto start = std::chrono::system_clock::now();
		auto epoch = start.time_since_epoch();
		auto milli = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();
		return milli;
	}

private:
	mutable std::mutex mutex;
	std::vector<tqueue_entry> entries;

	static bool compare(const tqueue_entry& e0, const tqueue_entry& e1)
	{
		return e0.time < e1.time;
	}

	void sort()
	{
		std::sort(entries.begin(), entries.end(), compare);
	}

};
