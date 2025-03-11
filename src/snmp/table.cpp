#include "snmp/table.h"

#include "core/csv.h"
#include "core/util.h"
#include "snmp/device.h"

#include <algorithm>
#include <sstream>

namespace snmpfs {

	// TODO 'T' is not a Proper SNMP Type, acts as placeholder for Table
	Table::Table(Device* device, ObjectID id) : Object(device, id, 'T')
	{

	}

	Table::~Table()
	{
		// Free all rowData
		for(const auto& [colID, colData] : cells)
		{
			for(const auto& [rowID, cell] : colData)
			{
				delete cell;
			}
		}
	}

	void Table::addColumn(std::string name, ObjectID oid)
	{
		columns.push_back({name, oid});
	}

	ObjectID Table::getColumnOID(const std::string& name) const
	{
		for(const TableColumn& col : columns)
		{
			if(col.name == name)
			{
				return col.oid;
			}
		}
		return ObjectID();
	}

	void Table::reverseColumns()
	{
		std::reverse(columns.begin(), columns.end());
	}

	bool Table::isReadable() const
	{
		bool readable = false;
		for(const TableColumn& column : columns)
			readable |= column.oid.isReadable();
		return readable;
	}

	bool Table::isWritable() const
	{
		bool writable = false;
		for(const TableColumn& column : columns)
			writable |= column.oid.isWritable();
		return writable;
	}




	std::string Table::getData() const
	{
		// std::unique_lock<std::mutex> lock(tableLock);
		std::stringstream ss;

		// WRITE TABLE HEADER
		for(size_t i = 0; i < columns.size(); i++)
		{
			const TableColumn& col = columns[i];
			ss << col.name;

			if(i < columns.size() - 1)	ss << colSeparator;
			// else						ss << rowSeparator;
		}
		ss << rowSeparator;

		const std::set<std::string> rowSet = getRowIDs();
		const std::vector<std::string> rows(rowSet.begin(), rowSet.end());
		for(size_t r = 0; r < rows.size(); r++)
		{
			const std::string& rowID = rows[r];
			for(size_t c = 0; c < columns.size(); c++)
			{
				const TableColumn& col = columns[c];
				if(!cells.contains(col.oid))
				{
					ss << "";
				}
				else if(!cells.at(col.oid).contains(rowID))
				{
					ss << "";
				}
				else
				{
					Object* cell = cells.at(col.oid).at(rowID);
					ss << cell->getData();
				}

				if(c < columns.size() - 1)	ss << colSeparator;
				// else						ss << rowSeparator;
			}
			if(r < rows.size() - 1)			ss << rowSeparator;
		}

		return ss.str();
	}



	bool Table::updateData(const ObjectID& oid, const std::string& data)
	{
		// printf("[Table] UPDATING CELL DATA FOR %s (%s)\n", ((std::string) oid).c_str(), data.c_str());
		for(auto& [colID, colData] : cells)
		{
			if(!colID.isAncestorOf(oid)) continue;

			for(auto& [rowID, cell] : colData)
			{
				if(cell->getID() == oid)
				{
					return cell->updateData(oid, data);
				}
			}
		}
		throw std::runtime_error("NO CELL FOUND!");
	}

	bool Table::dump() const
	{
		printf("---------- TABLE DUMP ----------\n");
		printf("%s\n", getData().c_str());
		printf("--------------------------------\n");
		return true;
	}

	bool Table::update()
	{
		// std::unique_lock<std::mutex> lock(tableLock);
		// printf("[Table] Update %s from Device\n", ((std::string) id).c_str());
		if(columns.size() <= 0) return false;


		std::set<std::string> rowIDs;
		bool somethingChanged = false;
		for(size_t c = 0; c < columns.size(); c++)
		{
			const TableColumn& col = columns[c];
			ObjectID currentOID	= col.oid;
			ObjectData currentData;
			while(device->next(currentOID, currentData))
			{
				if(!col.oid.isAncestorOf(currentOID)) break;

				std::string rowID = makeRowID(col.oid, currentOID);
				rowIDs.emplace(rowID);

				if(!cells[col.oid].contains(rowID))
				{
					// NEW ROW
					Object* cell = new Object(device, currentData.id, currentData.type);
					cells[col.oid][rowID] = cell;
				}

				// UPDATE DATA
				somethingChanged |= updateData(currentOID, currentData.data);
			}
		}

		// Find rows that are not there anymore
		std::vector<std::string> forRemoval;
		for(const auto& [colID, colData] : cells)
		{
			for(const auto& [rowID, cell] : colData)
			{
				if(std::find(rowIDs.begin(), rowIDs.end(), rowID) == rowIDs.end())
				{
					forRemoval.push_back(rowID);
				}
			}
		}

		// Delete rows that are not there anymore
		for(const std::string& rowID : forRemoval)
		{
			removeRow(rowID);
			somethingChanged |= true;
		}

		if(somethingChanged) notifyChanged();
		notifyUpdated();

		return true;
	}


	bool Table::updateData(const std::string& data)
	{
		// std::unique_lock<std::mutex> lock(tableLock);
		// printf("[Table] Update %s with:\n%s\n", ((std::string) id).c_str(), data.c_str());

		try
		{
			bool allSuccess = true;
			csvData csv = csvData::of(data, colSeparator, rowSeparator);

			// Build helping structure to quickly map column ID to ObjectID
			std::vector<ObjectID> columnOIDs;
			for(const std::string& name : csv.getRow(0))
			{
				ObjectID id = getColumnOID(name);
				if(id == ObjectID()) throw std::runtime_error("No OID found for column " + name);
				columnOIDs.emplace_back(id);
			}

			// Build helping structure to quickly map rowID to OID
			std::set<std::string> rowIDset = getRowIDs();
			std::vector row2oid(rowIDset.begin(), rowIDset.end());


			for(size_t row = 1; row < csv.getRowCount(); row++)
			{
				std::string rowID = row2oid[row - 1];
				for(uint32_t column = 0; column < csv.getColumnCount(); column++)
				{
					const std::string& cellData = csv.get(row, column);
					const ObjectID colID = columnOIDs[column];
					const ObjectID cellID(((std::string) colID) + rowID);

					Object* obj = cells[colID][rowID];
					if(obj)
					{
						// printf("Updating %s with %s\n", ((std::string) obj->getID()).c_str(), cellData.c_str());
						bool suc = obj->updateData(cellData);
						allSuccess &= suc;
					}
					else
					{
						throw std::runtime_error("Object NOT found in Table");
					}
				}
			}

			// Required because updateData is only called when ObjectNode is flushed and file was modifed
			notifyChanged();
			if(!allSuccess) return false;
		}
		catch(const std::exception& e)
		{
			std::stringstream err;
			err << "Can't update table ";
			err << ((std::string) id);
			err << " due to ";
			err << e.what();

			device->logErr(err.str());
			notifyChanged(true);
			return false;
		}

		return true;
	}

	void Table::removeRow(std::string rowID)
	{
		for(auto& [colID, colData] : cells)
		{
			if(colData.contains(rowID))
			{
				delete colData[rowID];
				colData.erase(rowID);
			}
		}
	}


	std::set<std::string> Table::getRowIDs() const
	{
		std::set<std::string> ids;

		for(const auto& [colID, rowData] : cells)
		{
			for(const auto& [rowID, cell] : rowData)
			{
				ids.emplace(rowID);
			}
		}

		return ids;
	}


	std::string Table::makeRowID(const ObjectID& columnID, const ObjectID& cellID)
	{
		std::stringstream ss;

		for(size_t i = columnID.length(); i < cellID.length(); i++)
		{
			ss << "." << cellID[i];
		}

		return ss.str();
	}


}	// namespace snmpfs
