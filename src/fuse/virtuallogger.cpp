#include "fuse/virtuallogger.h"

#include <cstring>
#include <iostream>
#include <sys/stat.h>
#include <sstream>

namespace snmpfs {

	VirtualLogger::VirtualLogger()
	{
		updateTime();
	}

	VirtualLogger::VirtualLogger(std::string name) : FileNode(name)
	{
		updateTime();
	}

	void VirtualLogger::clear()
	{
		std::unique_lock lock(mutex);
		messages.clear();
	}


	void VirtualLogger::print(std::string msg)
	{
		std::unique_lock lock(mutex);
		messages.push_back(msg);
		shrink(maxBufferSize);
		updateTime();
	}

	void VirtualLogger::printLine(std::string msg)
	{
		// TIME
		auto now	= std::chrono::system_clock::now();
		auto time	= std::chrono::system_clock::to_time_t(now);
		auto millis	= std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;
		auto tm		= std::gmtime(&time);

		// FORMAT
		std::stringstream ss;
		ss << "[";
		ss << std::put_time(tm, "%Y-%m-%d %H:%M:%S.");
		ss << std::setfill('0') << std::setw(3) << millis;
		ss << "] ";
		ss << msg;
		ss << std::endl;

		// PRINT
		print(ss.str());
	}


	void VirtualLogger::updateTime()
	{
		timespec_get(&lastAccess, TIME_UTC);
		timespec_get(&lastChange, TIME_UTC);
	}

	void VirtualLogger::shrink(uint64_t size)
	{
		std::unique_lock lock(mutex);
		while(getSize() > size)
		{
			messages.erase(messages.begin());
		}
	}



	int VirtualLogger::read(char* buf, size_t size, off_t offset)
	{
		uint64_t bytesPassed	= 0;
		uint64_t bytesRead		= 0;

		std::unique_lock lock(mutex);
		for(const std::string& msg : messages)
		{
			for(size_t i = 0; i < msg.size(); i++)
			{
				bytesPassed++;
				if(bytesPassed < offset+1) continue;
				if(bytesPassed > offset + size) break;
				buf[bytesRead++] = msg[i];
			}
			if(bytesPassed > offset + size) break;
		}

		timespec_get(&lastAccess, TIME_UTC);
		return bytesRead;
	}

	int VirtualLogger::write(const char* buf, size_t size, off_t offset)
	{
		return -EACCES;
	}

	int VirtualLogger::truncate(off_t size)
	{
		return -EACCES;
	}

	uint64_t VirtualLogger::getMode() const
	{
		uint64_t mode = 0;
		mode |= S_IFREG;							// Regular File
		mode |= S_IRUSR | S_IRGRP | S_IROTH;		// Read by Owner and Group and Others
		return mode;
	}

	uint64_t VirtualLogger::getSize() const
	{
		uint64_t size = 0;
		std::unique_lock lock(mutex);
		for(const std::string& msg : messages)
		{
			size += msg.size();
		}
		return size;
	}

	timespec VirtualLogger::getTimeAccess() const
	{
		return lastAccess;
	}

	timespec VirtualLogger::getTimeModification() const
	{
		return lastChange;
	}

	timespec VirtualLogger::getTimeStatusChange() const
	{
		return lastChange;
	}

}	// namespace snmpfs
