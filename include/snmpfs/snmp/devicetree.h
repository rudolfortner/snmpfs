#pragma once

#include "snmp/device.h"
#include "snmp/objectid.h"

namespace snmpfs {

	/**
	* DeviceTree is used to check what Objects are available on the Device
	*/
	class DeviceTree
	{
	private:
		DeviceTree();

	public:
		~DeviceTree();

	public:
		static DeviceTree* fromConfig(Device* device);
		static DeviceTree* fromDevice(Device* device);


		Device* getDevice() const;
		bool contains(const ObjectID& id);
		DeviceTree* get(const ObjectID& id);
		void put(const ObjectID& id, const ObjectData& data);

		DeviceTree* getChild(oid id) const;
		DeviceTree* findOID(const ObjectID& id) const;
		const std::vector<DeviceTree*> getChildren() const;
		const std::vector<ObjectID> getChildOIDs() const;
		std::string print() const;
		size_t level() const;
		size_t size() const;


		ObjectData getObjectData() const;
		ObjectID getOID() const;
		char getType() const;
		std::string getData() const;

	private:
		DeviceTree* addChild(oid id);
		void printRec(std::stringstream& ss) const;

	private:
		Device* device;
		DeviceTree* parent;
		std::vector<DeviceTree*> childs;

		std::vector<oid> nodeID;
		ObjectData data;
	};

}	// namespace snmpfs
