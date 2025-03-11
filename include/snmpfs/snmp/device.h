#pragma once

#include "config.h"
#include "core/taskmanager.h"
#include "object.h"
#include "fuse/objectnode.h"
#include "fuse/virtuallogger.h"
#include "proc.h"
#include "snmp/snmp_ext.h"
#include "snmpfs.h"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace snmpfs {


	class UpdateTask;

	/**
	* Represents a SNMP enabled Device inside our program.
	* It provides common functions like get, set and utils for formatting variables.
	* Also it groups Objects belonging to this Device.
	*/
	class Device
	{
	public:

		enum Status {
			ONLINE,
			INACCESSIBLE,
			OFFLINE
		};

		/**
		* Default constructor
		*/
		Device(snmpFS* snmpfs, const DeviceConfig& config);

		/**
		* Destructor
		*/
		~Device();

		bool initSNMP();
		void cleanupSNMP();

		// LOGGING API
		void logInfo	(const std::string& msg) const;
		void logNotice	(const std::string& msg) const;
		void logWarn	(const std::string& msg) const;
		void logErr		(const std::string& msg) const;

		// OBJECTS API
		void freeObjects();
		Object* lookupObject(ObjectID oid, uint32_t interval) const;
		void registerObject(Object* obj, uint32_t interval);
		void unregisterObject(Object* obj);

		// SNMP API
		bool formatVariable(netsnmp_variable_list* var, std::string& data) const;
		bool probe(ObjectID oid) const;
		bool next(ObjectID& oid) const;
		bool next(ObjectID& oid, ObjectData& data) const;
		ObjectData get(const ObjectID& id) const;
		ObjectData set(ObjectID oid, char type, std::string data) const;
		std::vector<ObjectData> walk() const;
		std::vector<ObjectData> walkSubtree(const ObjectID& oid) const;

		// bool active() const { return counterRequests == counterTimeouts; }
		Status checkStatus() const;
		void update();

		const DeviceConfig& getConfig() const { return config; }
		std::string getName() const { return name; }
		const std::map<uint32_t, UpdateTask> getTasks() const { return tasks; }

	private:
		// ESSENTIAL INFO
		snmpFS* snmpfs;
		const std::string name;
		const DeviceConfig config;

		// SNMP
		mutable std::mutex snmpMutex;
		void* snmpHandle;

		// TASKS CONTAINING ALL OBJECTS
		std::map<uint32_t, UpdateTask> tasks;
		bool updateObjects(const std::map<ObjectID, Object*>& objects);

		// SNMP API
		bool checkStatus(int status, const std::string& op);
		void printResponse(netsnmp_pdu* response) const;
		std::vector<ObjectData> processResponse(netsnmp_pdu* response) const;
		bool processTrap(netsnmp_pdu* response);
		bool sendPDU(netsnmp_pdu* pdu, netsnmp_pdu** response, const std::string& op) const;

		friend class UpdateTask;
		friend class DeviceTrapHandler;
	};

	/**
	 * UpdateTask reponsible do update a Device's Objects in regular intervals
	 */
	class UpdateTask : public Task
	{
	public:
		UpdateTask();
		size_t getSize() const { return objects.size(); }
	private:
		void run();
		Device* device;
		std::map<ObjectID, Object*> objects;

		friend class Device;
	};

}	// namespace snmpfs
