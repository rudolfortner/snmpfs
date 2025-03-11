#pragma once

#include "proc.h"

#include <filesystem>

namespace snmpfs {

	/**
	 * Structure for holding program parameters when parsing them via FUSE
	 */
	struct snmpfsParams {
		char* configPath = NULL;
	};

	/**
	 * Required for handling program parameters like -h and -v
	 */
	enum {
		SNMPFS_OPT_KEY_HELP,
		SNMPFS_OPT_KEY_VERSION
	};

	class Device;
	class FileNode;
	class TaskManager;
	class VirtualLogger;
	class TrapReceiver;
	struct snmpfsConfig;

	/**
	 * Holds the filesystem data after initialization
	 */
	struct snmpFS {
		std::mutex mutex;
		bool active = false;
		std::vector<Device*> devices;
		FileNode* root = NULL;

		// Proc Data
		ProcData proc;
		TaskManager taskManager;
		TrapReceiver* trapReceiver = NULL;
	};

	snmpFS* createFS(const snmpfsConfig& config);
	void destroyFS(snmpFS* snmpfs);

}	// namespace snmpfs
