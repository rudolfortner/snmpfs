#include "deviceinit.h"

#include "snmp/table.h"

#include <algorithm>
#include <assert.h>

namespace snmpfs {

	DeviceInitTask::DeviceInitTask(snmpFS* snmpfs, std::vector<DeviceConfig> deviceConfigs) : Task(SINGLE), snmpfs(snmpfs), deviceConfigs(deviceConfigs)
	{

	}


	uint64_t DeviceInitTask::calcDelay(uint64_t delay) const
	{
		static uint64_t minDelay = 1000;
		static uint64_t maxDelay = 5 * 60 * 1000;

		if(delay <  minDelay)	return minDelay;
		if(delay >= maxDelay)	return maxDelay;
		return 2 * delay;
	}

	void DeviceInitTask::run()
	{
		// CREATE ALL DEVICES
		for(const DeviceConfig& config : deviceConfigs)
		{
			Device* device = new Device(snmpfs, config);
			device->initSNMP();
			deviceQueue.push(device);
		}

		// CREATE THREADS FOR PARALLEL INITIALIZATION
		// TODO How many threads? One for each device?
		for(size_t i = 0; i < 8; i++)
			threads.emplace_back(&DeviceInitTask::runSingle, this);

		// WAIT FOR ALL THREADS
		for(std::thread& thread : threads)
			thread.join();

		// DELETE ALL DEVICES STILL IN QUEUE (during shutdown process)
		while(true)
		{
			Device* device = deviceQueue.pop();
			if(!device) break;
			device->cleanupSNMP();
			delete device;
		}
	}

	void DeviceInitTask::runSingle()
	{
		while(true)
		{
			// STOP CONDITION 1
			snmpfs->mutex.lock();
			bool active = snmpfs->active;
			snmpfs->mutex.unlock();
			if(!active)	break;

			// GET NEXT DEVICE
			Device* device;
			uint64_t delay;
			while(deviceQueue.waiting())
			{
				snmpfs->mutex.lock();
				bool active = snmpfs->active;
				snmpfs->mutex.unlock();
				if(!active)	break;
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
			deviceQueue.next(&device, &delay);

			// STOP CONDITION 2
			if(!device)
				break;

			// SKIP IF OFFLINE
			Device::Status status = device->checkStatus();
			switch(status)
			{
				case Device::ONLINE:
					initDevice(device);
					break;

				case Device::INACCESSIBLE:
					device->logErr("Inaccessible! Please check credentials!");
					device->cleanupSNMP();
					delete device; device = nullptr;
					break;

				case Device::OFFLINE:
					delay = calcDelay(delay);
					std::stringstream ss;
					ss << "Waiting ";
					ss << (delay / 1000) << "s";
					ss << " for device to become online..";
					device->logInfo(ss.str());
					deviceQueue.pushIn(device, delay);
					break;
			}
		}
	}


	void DeviceInitTask::initDevice(Device* device)
	{
		assert(device);
		auto initStart = std::chrono::high_resolution_clock::now();

		// RETRIEVE DEVICETREE
		DeviceTree* deviceTree = DeviceTree::fromConfig(device);
		// DeviceTree* deviceTree = DeviceTree::fromDevice(device);
		// auto treeEnd	= std::chrono::high_resolution_clock::now();
		// auto treeMillis	= std::chrono::duration_cast<std::chrono::milliseconds>(treeEnd - initStart).count();
		// printf("[%s] DeviceTree created in %fs\n", device->name.c_str(), treeMillis / 1000.0);

		// CREATE FILESYSTEM FOR DEVICE
		// Acts as organizational unit (not the same as Device)
		FileNode* deviceNode = new FileNode(device->getName());
		for(const ObjectConfig& objectConfig : device->getConfig().objects)
		{
			createNodes(deviceTree, deviceNode, objectConfig);
		}

		// DELETE DEVICETREE
		delete deviceTree;

		// TODO Check if I want the device folder to disappear or keep it with empty files
		// if(!device->active())
		{
			// TODO Free recursively
			// 			delete device;
			// 			delete deviceNode;
			// 			continue;
		}

		// ADD DEVICE TO FILESYSTEM
		snmpfs->mutex.lock();
		snmpfs->devices.push_back(device);
		insertFileNodeByPath("/", deviceNode, snmpfs->root);	// TODO insert by actual path?
		snmpfs->mutex.unlock();

		auto initEnd	= std::chrono::high_resolution_clock::now();
		auto initMillis	= std::chrono::duration_cast<std::chrono::milliseconds>(initEnd - initStart).count();
		device->logInfo("DeviceInit finished after " + std::to_string(initMillis/1000.0) + "s");
	}





	void loadTableCustom(Table* table, const ObjectConfig& config)
	{
		for(const ConfigEntry& ce : config.columns)
		{
			table->addColumn(ce.name, ObjectID(ce.rawOID));
		}
	}

	void loadTableMIB(Table* table)
	{
		const ObjectID& oid = table->getID();

		// if no columns specified, load according to MIB
		tree* tr = oid.getMIB();
		tree* entry = tr->child_list;

		if(entry)
		{
			for(tree* col = entry->child_list; col; col = col->next_peer)
			{
				table->addColumn(col->label, oid.getSubOID(entry->subid).getSubOID(col->subid));
			}
		}
		table->reverseColumns();
	}

	void loadTableWalk(Table* table, const DeviceTree* deviceTree, const ObjectID& oid, const ObjectConfig* config)
	{
		// Try to load Table structure from walk data (DeviceTree)
		DeviceTree* tb = deviceTree->findOID(oid);
		if(!tb)
		{	deviceTree->getDevice()->logWarn("No Table in DeviceTree for " + config->name);
			return;
		}

		DeviceTree* et = tb->getChild(1);
		if(!et)
		{	deviceTree->getDevice()->logWarn("No Entry in DeviceTree for " + config->name);
			return;
		}

		for(ObjectID col : et->getChildOIDs())
		{
			table->addColumn(col, col);
		}
	}


	Table* createTable(DeviceTree* deviceTree, const ObjectID& oid, const ObjectConfig* config)
	{
		Table* table = new Table(deviceTree->getDevice(), oid);

		if(config && config->columns.size() > 0)
		{
			// If columns are specified, only use them
			loadTableCustom(table, *config);
		}
		else if(oid.hasMIB())
		{
			loadTableMIB(table);
		}
		else if(deviceTree)	// TODO Check if tree is there ?
		{
			loadTableWalk(table, deviceTree, oid, config);
		}
		else
		{
			throw std::runtime_error("Could not create table");
		}

		return table;
	}

	void createNodes(DeviceTree* deviceTree, FileNode* parentNode, const ObjectConfig& config)
	{
		ObjectID oid(config.rawOID);

		if(config.type == SCALAR || config.type == TABLE)
		{
			if(!deviceTree)
			{
				return;
			}

			// CHECK IF OID IS AVAILABLE ON DEVICE ?
			// IF NOT AVAIL RETURN AND ADD NOTHING
			DeviceTree* objTree = deviceTree->get(oid);
			if(config.type == SCALAR && !objTree)
			{
				return;
			}

			// CHECK IF OBJECT FOR THIS OID AND INTERVAL ALREADY EXISTS
			Device* device = deviceTree->getDevice();
			Object* obj = device->lookupObject(oid, config.interval);
			// if(obj) printf("Object already there %s\n", ((std::string) obj->getID()).c_str());

			if(!obj)
			{
				// CREATE A BRAND NEW OBJECT FOR THIS OID
				if(config.type == SCALAR)
				{
					obj = new Object(device, oid, objTree->getObjectData());
				}
				else if(config.type == TABLE)
				{
					obj = createTable(deviceTree, oid, &config);
					// TODO Load inital data from tree ?
					obj->update();
				}
				device->registerObject(obj, config.interval);
			}

			if(!obj) return;

			ObjectNode* node = new ObjectNode(config.name, obj);
			parentNode->addChild(node);
			obj->notifyChanged();
			obj->notifyUpdated();
		}
		else if(config.type == TREE)
		{
			createTree(deviceTree, parentNode, config);
		}
		else
		{
			throw std::runtime_error("No such type");
		}
	}

	void createTreeFromMIB(DeviceTree* deviceTree, FileNode* parentNode, const ObjectConfig& config, const ObjectID& oid)
	{
		tree* tr = oid.getMIB();
		// printf("Creating tree from MIB for %s (%s)\n", ((std::string) oid).c_str(), tr->label);

		std::string name = tr->label;
		std::transform(name.begin(), name.end(), name.begin(), tolower);

		if(name.ends_with("table"))
		{
			ObjectConfig tableConfig;
			tableConfig.name	= config.name;
			tableConfig.rawOID	= (std::string) oid;
			tableConfig.type	= TABLE;
			tableConfig.interval= config.interval;

			createNodes(deviceTree, parentNode, tableConfig);
		}
		else if(tr->child_list)
		{
			// FOUND FOLDER NODE
			FileNode* node = config.placeholder ? parentNode : new FileNode(config.name);

			for(tree* child = tr->child_list; child; child = child->next_peer)
			{
				ObjectID childID = oid.getSubOID(child->subid);
				ObjectConfig childConfig;
				childConfig.name	= child->label;
				childConfig.rawOID	= (std::string) childID;
				childConfig.type	= TREE;
				childConfig.interval= config.interval;
				createNodes(deviceTree, node, childConfig);
			}

			// Avoid loads of empty directories
			if(node->children.size() == 0)
			{
				delete node;
				return;
			}

			if(!config.placeholder) parentNode->addChild(node);
		}
		else
		{
			ObjectConfig childConfig;
			childConfig.name	= config.name;
			childConfig.rawOID	= (std::string) oid.getSubOID(0);
			childConfig.type	= SCALAR;
			childConfig.interval= config.interval;

			createNodes(deviceTree, parentNode, childConfig);
		}
	}

	void createTreeFromDeviceTree(DeviceTree* deviceTree, FileNode* parentNode, const ObjectConfig& config, const ObjectID& oid)
	{
		DeviceTree* subTree = deviceTree->findOID(oid);
		if(!subTree) return;
		DeviceTree* chld = subTree->getChild(0);

		if(chld && chld->getOID().back() == 0 && chld->size() == 0)
		{
			ObjectConfig childConfig;
			childConfig.name	= "F" + (std::string) chld->getOID();
			childConfig.rawOID	= (std::string) chld->getOID();
			childConfig.type	= SCALAR;
			childConfig.interval= config.interval;

			createNodes(deviceTree, parentNode, childConfig);
		}
		else if(false)
		{
			// TABLE
			// TODO How to identify a table in DeviceTree ?
			// TODO Check oid that is used here
			throw std::runtime_error("NOT IMPLEMENTED");
			ObjectConfig childConfig;
			childConfig.name	= "T" + (std::string) chld->getOID();
			childConfig.rawOID	= (std::string) chld->getOID();
			childConfig.type	= TABLE;
			childConfig.interval= config.interval;

			createNodes(deviceTree, parentNode, childConfig);
		}
		else
		{
			// FOLDER
			FileNode* node = config.placeholder ? parentNode : new FileNode(config.name);

			for(const ObjectID& childID : subTree->getChildOIDs())
			{
				ObjectConfig childConfig;
				childConfig.name	= "D" + (std::string) childID;
				childConfig.rawOID	= (std::string) childID;
				childConfig.type	= TREE;
				childConfig.interval= config.interval;
				createNodes(deviceTree, node, childConfig);
			}

			// Avoid loads of empty directories
			if(node->children.size() == 0)
			{
				delete node;
				return;
			}

			if(!config.placeholder) parentNode->addChild(node);
		}

		return;
	}

	void createTree(DeviceTree* deviceTree, FileNode* parentNode, const ObjectConfig& config)
	{
		ObjectID oid(config.rawOID);

		if(oid.hasMIB())	createTreeFromMIB(deviceTree, parentNode, config, oid);
		else				createTreeFromDeviceTree(deviceTree, parentNode, config, oid);
	}

}	// namespace snmpfs

