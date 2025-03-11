#pragma once

#include "fuse/filenode.h"
#include "snmp/object.h"

namespace snmpfs {

	/**
	* The ObjectNode is the interface between FUSE and an Object
	*/
	class ObjectNode : public FileNode, public ObjectObserver
	{
	public:
		ObjectNode(std::string name, Object* object);
		~ObjectNode();

		// CALLS FOR ACCESSING DATA
		int open(bool trunc);
		int read(char* buf, size_t size, off_t offset);
		int write(const char* buf, size_t size, off_t offset);
		int flush();
		int release();
		int truncate(off_t size);

		// CALLS FOR ATTRIBUTES
		virtual uint64_t getMode() const;
		virtual uint64_t getLinkCount() const;
		virtual uint64_t getSize() const;

		virtual timespec getTimeAccess() const;
		virtual timespec getTimeModification() const;
		virtual timespec getTimeStatusChange() const;



	private:
		Object* object;

		bool modified = false;
		timespec lastAccess, lastChange, lastUpdate;
		uint64_t length;
		char* data;

		// OBJECTOBSERVER
		void changed(bool restore = false);
		void updated();
	};

}	// namespace snmpfs
