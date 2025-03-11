#include "snmp/devicetree.h"

#include "snmp/table.h"

namespace snmpfs {

	DeviceTree::DeviceTree()
	{
		parent = nullptr;
	}

	DeviceTree::~DeviceTree()
	{
		// Free all childs nodes (when called on root the whole tree is freed recursively)
		for(DeviceTree* child : childs)
			delete child;
	}

	Device * DeviceTree::getDevice() const
	{
		return device;
	}


	/**
	 * Walks only a subset of the Device depending on the configuration.
	 * Used to make initialization faster
	 */
	DeviceTree* DeviceTree::fromConfig(Device* device)
	{
		if(device == nullptr)
			throw std::runtime_error("Can't build DeviceTree from nullptr");

		DeviceTree* root = new DeviceTree();
		root->device = device;

		// OIDs could be used multiple times -> avoid walking them twice
		std::set<ObjectID> oids;
		for(const ObjectConfig& objectConfig : device->getConfig().objects)
		{
			ObjectID oid(objectConfig.rawOID);
			if(objectConfig.type == SCALAR)
			{
				oids.emplace(oid.getParentID());
			}
			else
			{
				oids.emplace(oid);
			}
		}

		for(const ObjectID& oid : oids)
		{
			for(const ObjectData& data : device->walkSubtree(oid))
			{
				root->put(data.id, data);
			}
		}

		return root;
	}

	/**
	 * Walks the whole Device to check what objects it reveals
	 */
	DeviceTree* DeviceTree::fromDevice(Device* device)
	{
		if(device == nullptr)
			throw std::runtime_error("Can't build DeviceTree from nullptr");

		DeviceTree* root = new DeviceTree();
		root->device = device;

		std::vector<ObjectData> data = device->walk();

		// Add found objects to DeviceTree
		for(const ObjectData& data : data)
			root->put(data.id, data);

		return root;
	}

	bool DeviceTree::contains(const ObjectID& id)
	{
		// TODO Maybe replace with return get(...);
		DeviceTree* current = this;
		for(size_t i = 0; i < id.length(); i++)
		{
			current = current->getChild(id[i]);
			if(!current) return false;
		}

		return true;
	}

	DeviceTree* DeviceTree::get(const ObjectID& id)
	{
		DeviceTree* current = this;
		for(size_t i = 0; i < id.length(); i++)
		{
			current = current->getChild(id[i]);
			if(!current) return nullptr;
		}

		return current;
	}

	void DeviceTree::put(const ObjectID& id, const ObjectData& data)
	{
		if(id.length() < level())
		{
			printf("Tree ID: %zu\n", level());
			printf("New  ID: %zu\n", id.length());
			throw std::runtime_error("ObjectID inserted too low");
		}

		if(level() < id.length())
		{
			DeviceTree* child = addChild(id[level()]);
			child->put(id, data);
		}

		if(id == ObjectID(id))
			this->data = data;
	}



	const std::vector<DeviceTree *> DeviceTree::getChildren() const
	{
		return childs;
	}

	const std::vector<ObjectID> DeviceTree::getChildOIDs() const
	{
		std::vector<ObjectID> childOIDs;

		for(DeviceTree* child : childs)
		{
			childOIDs.push_back(child->getOID());
		}

		return childOIDs;
	}


	std::string DeviceTree::print() const
	{
		std::stringstream ss;
		printRec(ss);
		return ss.str();
	}

	void DeviceTree::printRec(std::stringstream& ss) const
	{
		// INLINE NAME OF THIS NODE
		for(size_t i = 0; i < level(); i++)
			ss << "-";

		// PRINT ID
		for(size_t i = 0; i < nodeID.size(); i++)
			ss << "." << std::to_string(nodeID[i]);
		if(level() == 0)
			ss << "ROOT";

		// PRINT DATA
		ss << " (" << data.type << ")";
		ss << " " << data.data;

		ss << std::endl;


		// PRINT CHILDS
		for(DeviceTree* child : childs)
			child->printRec(ss);
	}


	size_t DeviceTree::level() const
	{
		return nodeID.size();
	}

	size_t DeviceTree::size() const
	{
		return childs.size();
	}


	DeviceTree* DeviceTree::addChild(oid id)
	{
		DeviceTree* child = getChild(id);
		if(child) return child;

		child = new DeviceTree();
		child->parent = this;
		child->nodeID = std::vector(this->nodeID);
		child->nodeID.push_back(id);
		childs.push_back(child);
		return child;
	}

	DeviceTree* DeviceTree::getChild(oid id) const
	{
		for(DeviceTree* child : childs)
		{
			if(child->nodeID[level()] == id) return child;
		}
		return nullptr;
	}

	DeviceTree* DeviceTree::findOID(const ObjectID& id) const
	{
		// Check if we found correct Node
		if(id.length() == level() && id == ObjectID(nodeID))
		{
			// printf("FOUND %s!\n", ((std::string) ObjectID(nodeID)).c_str());
			// printf("with data %s!\n", ((std::string) getOID()).c_str());
			return (DeviceTree*) this;
		}

		// Check all digits down to this level and move up in tree
		for(size_t lev = 0; lev < level(); lev++)
		{
			if(id[lev] != nodeID[lev])
			{
				// printf("Moving Up in DeviceTree\n");
				return parent->findOID(id);
			}
		}

		// Check all childs
		for(DeviceTree* child : childs)
		{
			// printf("Searching Child %s\n", ((std::string) ObjectID(child->nodeID)).c_str());
			if(child->nodeID[level()] == id[level()])
			{
				return child->findOID(id);
			}
		}

		return nullptr;
	}

	ObjectData DeviceTree::getObjectData() const
	{
		return data;
	}

	ObjectID DeviceTree::getOID() const
	{
		return ObjectID(nodeID);
	}

	char DeviceTree::getType() const
	{
		return data.type;
	}

	std::string DeviceTree::getData() const
	{
		return data.data;
	}

}	// namespace snmpfs
