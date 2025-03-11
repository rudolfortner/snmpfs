#pragma once

#include "snmp_ext.h"

#include <string>
#include <vector>

namespace snmpfs {

	/**
	* @todo write docs
	*/
	class ObjectID
	{
	public:
		/**
		* Default constructor
		*/
		ObjectID();
		ObjectID(std::string raw);
		ObjectID(const char* raw);
		ObjectID(oid* name, size_t name_length);
		ObjectID(std::vector<oid> oids);

		/**
		* Destructor
		*/
		~ObjectID();

		operator oid*() const;
		operator size_t() const;
		operator std::string() const;

		oid back() const;
		size_t length() const;

		bool operator==(const ObjectID& other) const { return oids == other.oids; }
		bool operator<(const ObjectID& other) const { return oids < other.oids; }
		oid operator[](uint64_t index) const { return oids[index]; }

		ObjectID getParentID() const;
		ObjectID getSubOID(uint64_t subID) const;
		bool isAncestorOf(const ObjectID& descendantOID) const;
		bool isParentOf(const ObjectID& childOID) const;

		static tree* toMIB(const ObjectID& id);
		tree* getMIB() const { return mib; }
		bool hasMIB() const { return mibAvailable; }

		bool isReadable() const { return readable; }
		void setReadable(bool readable) { this->readable = readable; }
		bool isWritable() const { return writable; }
		void setWritable(bool writable) { this->writable = writable; }

	private:

		std::vector<oid> oids;

		tree* mib;
		bool mibAvailable	= false;
		bool readable		= true;
		bool writable		= true;

		void clear();
		void set(std::string raw);
		void set(const char* raw);
		void set(oid* name, size_t name_length);
		void set(std::vector<oid> oids);

		void updateInfo();
	};

}	// namespace snmpfs
