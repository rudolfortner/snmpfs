#include "snmp/objectid.h"

#include <sstream>
#include <stdexcept>

namespace snmpfs {

	ObjectID::ObjectID()
	{
		set(".");
	}

	ObjectID::ObjectID(std::string raw)
	{
		set(raw);
	}

	ObjectID::ObjectID(const char* raw)
	{
		set(raw);
	}

	ObjectID::ObjectID(oid* name, size_t name_length)
	{
		set(name, name_length);
	}

	ObjectID::ObjectID(std::vector<oid> oids)
	{
		set(oids);
	}



	ObjectID::~ObjectID()
	{

	}


	ObjectID::operator oid*() const
	{
		if(oids.size() == 0)
			return NULL;
		return (oid*) &oids[0];
	}

	ObjectID::operator size_t() const
	{
		return oids.size();
	}

	ObjectID::operator std::string() const
	{
		std::stringstream ss;

		for(size_t i = 0; i < oids.size(); i++)
		{
			ss << "." << oids[i];
		}

		return ss.str();
	}

	oid ObjectID::back() const
	{
		if(oids.size() <= 0)
			throw std::runtime_error("ObjectID.back() on empty oid");

		return oids.back();
	}

	size_t ObjectID::length() const
	{
		return oids.size();
	}



	ObjectID ObjectID::getParentID() const
	{
		std::vector<oid> parentOIDs;
		for(size_t i = 0; i < oids.size() - 1; i++)
		{
			parentOIDs.push_back(oids[i]);
		}

		return ObjectID(parentOIDs);
	}

	ObjectID ObjectID::getSubOID(uint64_t subID) const
	{
		std::vector<oid> newOIDs = oids;
		newOIDs.push_back(subID);
		return ObjectID(newOIDs);
	}


	bool ObjectID::isAncestorOf(const ObjectID& descendantOID) const
	{
		for(size_t i = 0; i < length(); i++)
		{
			if(oids[i] != descendantOID.oids[i])
			{
				return false;
			}
		}

		return true;
	}

	bool ObjectID::isParentOf(const ObjectID& childOID) const
	{
		if(childOID.length() != length() + 1)
			return false;

		for(size_t i = 0; i < length(); i++)
		{
			if(oids[i] != childOID.oids[i])
			{
				return false;
			}
		}

		return true;
	}

	tree* ObjectID::toMIB(const ObjectID& id)
	{
		// Retrieve MIB tree node via SNMP
		tree* tr = get_tree(id, id, get_tree_head());
		if(!tr)
		{
			return nullptr;
		}

		// Retrieve OID of tree node we found
		tree* current = tr;
		std::vector<oid> ids;
		while(current)
		{
			ids.insert(ids.begin(), current->subid);
			current = current->parent;
		}

		// Check if they are equal (then table entry was found)
		if(ids == id.oids)
			return tr;

		// Check if they are equal (then scalar entry was found)
		// This has to be done since we always store the OID with a trailing .0
		// But MIB entries only contain the number without it
		if(id.back() == 0 && ids == id.getParentID().oids)
			return tr;

		return nullptr;
	}



	void ObjectID::clear()
	{
		oids.clear();
	}

	void ObjectID::set(std::string raw)
	{
		set(raw.c_str());
	}

	void ObjectID::set(const char* raw)
	{
		oid name[MAX_OID_LEN];
		size_t name_length = MAX_OID_LEN;	// MUST BE SET TO MAX_OID_LEN

		oid* res = snmp_parse_oid(raw, name, &name_length);
		if(!res) throw std::runtime_error("Could not parse OID from " + std::string(raw));

		set(name, name_length);
	}

	void ObjectID::set(oid* name, size_t name_length)
	{
		clear();
		for(size_t i = 0; i < name_length; i++)
		{
			oids.push_back(name[i]);
		}
		updateInfo();
	}

	void ObjectID::set(std::vector<oid> oids)
	{
		this->oids = oids;
		updateInfo();
	}



	void ObjectID::updateInfo()
	{
		mib = toMIB(*this);

		if(mib && find_module(mib->modid))
		{
			// FOUND MIB INFO
			mibAvailable	= true;
			switch(mib->access)
			{
				case MIB_ACCESS_READONLY:
					readable = true;
					writable = false;
					break;
				case MIB_ACCESS_READWRITE:
					readable = true;
					writable = true;
					break;
				case MIB_ACCESS_WRITEONLY:
					readable = false;
					writable = true;
					break;
				case MIB_ACCESS_NOACCESS:
					readable = false;
					writable = false;
					break;
				default:
					// TODO Why is sysUptime returning with this access value ?
					readable = true;
					writable = true;
					break;
			}
		}
		else
		{
			// DEFAULT INFO IF NO MIB PRESENT
			mibAvailable	= false;
			readable		= true;
			writable		= true;
		}
	}

}	// namespace snmpfs

