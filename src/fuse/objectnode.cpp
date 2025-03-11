#include "fuse/objectnode.h"

#include <sys/stat.h>

namespace snmpfs {

	ObjectNode::ObjectNode(std::string name, Object* object) : FileNode(name), object(object)
	{
		object->registerObserver(this);

		lastAccess = {};
		lastChange = {};
		lastUpdate = {};

		length	= 0;
		data	= nullptr;
	}

	ObjectNode::~ObjectNode()
	{
		object->unregisterObserver(this);
		if(data) free(data);
	}


	int ObjectNode::open(bool trunc)
	{
		if(trunc)	return truncate(0);
		else		return 0;
	}

	int ObjectNode::read(char* buf, size_t size, off_t offset)
	{
		if(offset >= length)
			return 0;

		if(length - offset < size)
			size = length - offset;

		memcpy(buf, data + offset, size);
		timespec_get(&lastAccess, TIME_UTC);

		return size;
	}

	int ObjectNode::write(const char* buf, size_t size, off_t offset)
	{
		if(offset + size > length)
		{
			length	= offset + size;
			data	= (char*) realloc(data, length);
		}

		memcpy(data + offset, buf, size);
		modified = true;

		return size;
	}

	int ObjectNode::flush()
	{
		if(!modified) return 0;
		bool success = object->updateData(std::string(data, length));
		return success ? 0 : -EIO;
	}

	int ObjectNode::release()
	{
		return 0;
	}

	int ObjectNode::truncate(off_t size)
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
			modified= true;
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
		modified= true;

		return 0;
	}

	uint64_t ObjectNode::getMode() const
	{
		uint64_t mode = 0;

		// Regular File
		mode |= S_IFREG;

		// Read by Owner and Group
		if(object->isReadable())
			mode |= S_IRUSR | S_IRGRP;

		// Write by Owner and Group
		if(object->isWritable())
			mode |= S_IWUSR | S_IWGRP;

		return mode;
	}

	uint64_t ObjectNode::getLinkCount() const
	{
		return 1;
	}

	uint64_t ObjectNode::getSize() const
	{
		return length;
	}

	timespec ObjectNode::getTimeAccess() const
	{
		return lastAccess;
	}

	timespec ObjectNode::getTimeModification() const
	{
		return lastChange;
	}

	timespec ObjectNode::getTimeStatusChange() const
	{
		return lastUpdate;
	}

	void ObjectNode::changed(bool restore)
	{
		std::string data = object->getData();

		truncate(data.size());
		write(data.c_str(), data.size(), 0);
		modified = false;

		if(!restore)
		{
			// ONLY UPDATE lastChange TIMESTAMP FOR NEW DATA (we do not restore)
			timespec_get(&lastChange, TIME_UTC);
		}
	}

	void ObjectNode::updated()
	{
		timespec_get(&lastUpdate, TIME_UTC);
	}


}	// namespace snmpfs




