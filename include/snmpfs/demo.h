#pragma once

#include "config.h"
#include "fuse/filenode.h"

namespace snmpfs {

	class Demo {
	public:
		static void addObject(DeviceConfig& device, std::string name, std::string oid, ObjectType type);
		static void addObjects(DeviceConfig& device);
		static FileNode* createTree();
		static snmpfsConfig createConfig();
	};

}	// namespace snmpfs
