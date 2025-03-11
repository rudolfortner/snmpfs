#pragma once

#include "core/taskmanager.h"
#include "core/tqueue.h"
#include "snmpfs.h"
#include "snmp/devicetree.h"

#include <mutex>

namespace snmpfs {

	class DeviceInitTask : public Task
	{
	public:
		DeviceInitTask(snmpFS* snmpfs, std::vector<DeviceConfig> deviceConfigs);

	private:
		snmpFS* snmpfs;
		std::vector<DeviceConfig> deviceConfigs;
		tqueue<Device> deviceQueue;
		std::vector<std::thread> threads;

		uint64_t calcDelay(uint64_t delay) const;
		void run();
		void runSingle();
		void initDevice(Device* device);
	};

	void createNodes(DeviceTree* deviceTree, FileNode* parentNode, const ObjectConfig& config);
	void createTree(DeviceTree* deviceTree, FileNode* parentNode, const ObjectConfig& config);

}	// namespace snmpfs
