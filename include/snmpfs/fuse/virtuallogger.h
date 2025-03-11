#pragma once

#include "filenode.h"

#include <list>
#include <mutex>

namespace snmpfs {

	/**
	* @todo write docs
	*/
	class VirtualLogger : public FileNode
	{
	public:
		[[deprecated("Use syslog(...) instead")]]
		VirtualLogger();
		VirtualLogger(std::string name);

		// Logger interface
		void clear();
		void print(std::string msg);
		void printLine(std::string msg);

		// FileNode interface
		virtual int read(char* buf, size_t size, off_t offset);
		virtual int write(const char* buf, size_t size, off_t offset);
		virtual int truncate(off_t size);

		// CALLS FOR ATTRIBUTES
		virtual uint64_t getMode() const;
		virtual uint64_t getSize() const;

		virtual timespec getTimeAccess() const;
		virtual timespec getTimeModification() const;
		virtual timespec getTimeStatusChange() const;

	private:
		uint64_t maxBufferSize = 1024 * 1024;
		mutable std::recursive_mutex mutex;
		std::list<std::string> messages;

		timespec lastChange, lastAccess;
		void updateTime();
		void shrink(uint64_t size);
	};

}	// namespace snmpfs
