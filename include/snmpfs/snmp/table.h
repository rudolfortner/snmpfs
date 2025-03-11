#pragma once

#include "object.h"
#include <map>
#include <mutex>
#include <set>

namespace snmpfs {

	class TableColumn
	{
	public:
		std::string name;
		ObjectID oid;
	};

	/**
	* @todo write docs
	*/
	class Table : public Object
	{
	public:
		/**
		* Default constructor
		*/
		Table(Device* device, ObjectID id);

		/**
		* Destructor
		*/
		~Table();

		void addColumn(std::string name, ObjectID oid);
		ObjectID getColumnOID(const std::string& name) const;
		void reverseColumns();

		bool isReadable() const;
		bool isWritable() const;

		virtual std::string getData() const;									// GET data represented as string
		virtual bool update();													// UPDATE data from snmp GET
		virtual bool updateData(const std::string& data);						// UPDATE data on Device
		virtual bool updateData(const ObjectID& oid, const std::string& data);	// UPDATE data from string (does not update remote data)

		bool dump() const;

	private:
		char colSeparator	= ',';
		char rowSeparator	= '\n';

		// mutable std::mutex tableLock;
		std::vector<TableColumn> columns;
		std::map<ObjectID, std::map<std::string, Object*>> cells;

		std::set<std::string> getRowIDs() const;
		void removeRow(std::string rowID);
		static std::string makeRowID(const ObjectID& columnID, const ObjectID& cellID);
	};

}	// namespace snmpfs
