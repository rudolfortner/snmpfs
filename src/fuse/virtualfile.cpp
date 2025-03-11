#include "fuse/virtualfile.h"

#include <cstring>
#include <sys/stat.h>

namespace snmpfs {

	VirtualFile::VirtualFile(std::string name) : FileNode(name)
	{
		lastAccess	= {};
		lastChange	= {};

		length	= 0;
		data	= NULL;
	}

	VirtualFile::~VirtualFile()
	{
		if(data) free(data);
	}

	int VirtualFile::read(char* buf, size_t size, off_t offset)
	{
		if(offset >= length)
			return 0;

		if(length - offset < size)
			size = length - offset;

		memcpy(buf, data + offset, size);

		timespec_get(&lastAccess, TIME_UTC);

		return size;
	}

	int VirtualFile::write(const char* buf, size_t size, off_t offset)
	{
		if(offset + size > length)
		{
			length	= offset + size;
			data	= (char*) realloc(data, length);
		}

		memcpy(data + offset, buf, size);

		timespec_get(&lastAccess, TIME_UTC);
		timespec_get(&lastChange, TIME_UTC);

		return size;
	}

	int VirtualFile::truncate(off_t size)
	{
		// NO CHANGE
		if(size == length)
			return 0;

		// DELETE BUFFER
		if(size == 0)
		{
			free(data);
			length	= 0;
			data	= nullptr;
		}

		// CHANGE BUFFER SIZE
		char* newData = (char*) realloc(data, size);
		if(!newData && size)
			return -ENOMEM;

		// SET EXTENSION TO NULL
		if(size > length)
			memset(newData + length, 0, size - length);

		// RELINK DATA
		length	= size;
		data	= newData;

		return 0;
	}

	uint64_t VirtualFile::getMode() const
	{
		return S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
	}

	uint64_t VirtualFile::getSize() const
	{
		return length;
	}

	timespec VirtualFile::getTimeAccess() const
	{
		return lastAccess;
	}

	timespec VirtualFile::getTimeModification() const
	{
		return lastChange;
	}

	timespec VirtualFile::getTimeStatusChange() const
	{
		return lastChange;
	}

}	// namespace snmpfs
