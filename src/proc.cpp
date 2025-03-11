#include "proc.h"

#include "fuse/procfile.h"
#include <algorithm>

namespace snmpfs {

	void ProcBase::registerObserver(ProcObserver* observer)
	{
		std::unique_lock<std::mutex> lock(mutex);
		observers.emplace_back(observer);
	}

	void ProcBase::unregisterObserver(ProcObserver* observer)
	{
		std::unique_lock<std::mutex> lock(mutex);
		observers.erase(std::remove(observers.begin(), observers.end(), observer), observers.end());
	}

	void ProcBase::fireChanged() const
	{
		std::unique_lock<std::mutex> lock(mutex);
		for(ProcObserver* obs : observers)
		{
			obs->updateProc();
		}
	}

	void addProcFile(FileNode* parent, const std::string name, ProcBase* base)
	{
		if(!parent)			throw std::runtime_error("argument error");
		if(!base)			throw std::runtime_error("argument error");
		if(name.empty())	throw std::runtime_error("argument error");

		ProcFile* file = new ProcFile(name, base);
		parent->addChild(file);
	}

	FileNode* createProcFS(ProcData* data)
	{
		FileNode* proc = new FileNode("proc");

		// SNMPFS
		addProcFile(proc, "snmpfs_creation",		&data->snmpfsCreation);
		addProcFile(proc, "snmpfs_last_update",		&data->snmpfsLastUpdate);
		addProcFile(proc, "snmpfs_pid",				&data->snmpfsPID);
		addProcFile(proc, "snmpfs_uid",				&data->snmpfsUID);
		addProcFile(proc, "snmpfs_gid",				&data->snmpfsGID);
		addProcFile(proc, "snmpfs_version",			&data->snmpfsVersion);


		// SNMP Counters
		addProcFile(proc, "snmp_request_count",		&data->snmpRequestCount);
		addProcFile(proc, "snmp_request_timeouts",	&data->snmpRequestTimeouts);
		addProcFile(proc, "snmp_request_errors",	&data->snmpRequestErrors);

		// SNMP Timestamps
		addProcFile(proc, "snmp_last_request",		&data->snmpLastRequest);

		return proc;
	}

}	// namespace snmpfs
