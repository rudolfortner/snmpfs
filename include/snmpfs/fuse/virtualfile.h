#pragma once

#include "filenode.h"

namespace snmpfs {

	/**
	* As the name suggests, data is only stored in memory and is purely virtual.
	*/
	class VirtualFile : public FileNode
	{
	public:
		VirtualFile(std::string name);
		~VirtualFile();

		// CALLS FOR ACCESSING DATA
		int read(char* buf, size_t size, off_t offset);
		int write(const char* buf, size_t size, off_t offset);
		int truncate(off_t size);

		// CALLS FOR ATTRIBUTES
		virtual uint64_t getMode() const;
		virtual uint64_t getSize() const;

		virtual timespec getTimeAccess() const;
		virtual timespec getTimeModification() const;
		virtual timespec getTimeStatusChange() const;

	private:
		timespec lastChange, lastAccess;
		uint64_t length;
		char* data;
	};

}	// namespace snmpfs
