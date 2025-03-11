#include "core/csv.h"

#include "core/util.h"
#include <stdexcept>

namespace snmpfs {

	csvData csvData::of(const std::string& data, char columnDelimiter, char lineDelimiter)
	{
		csvData csv;

		std::vector<std::string> lines = split(trim(data), lineDelimiter);

		// TODO Support escaped quotes and linebreaks
		for(const std::string& line : lines)
		{
			if(line.empty()) continue;

			std::vector<std::string> cols = split(trim(line), columnDelimiter);
			if(cols.size() == 0) continue;

			for(std::string& col : cols)
				col = trim(col);

			csv.rows.push_back(cols);
		}

		// Check if rows have equal lengths
		for(size_t i = 0; i < csv.rows.size(); i++)
		{
			for(size_t j = 0; j < csv.rows.size(); j++)
			{
				if(csv.rows[i].size() != csv.rows[j].size())
				{
					throw std::runtime_error("CSV rows do not have equal column counts!");
				}
			}
		}

		csv.rowCount = csv.rows.size();
		if(csv.rowCount > 0)
		csv.colCount = csv.rows[0].size();

		return csv;
	}

	bool csvData::empty() const
	{
		return !colCount && !rowCount;
	}

	uint32_t csvData::getColumnCount() const
	{
		return colCount;
	}

	uint32_t csvData::getRowCount() const
	{
		return rowCount;
	}

	const csvData::csvRowData& csvData::getRow(uint32_t row) const
	{
		if(row >= rowCount) throw std::runtime_error("row out of bounds");
		return rows[row];
	}


	const std::string& csvData::get(uint32_t row, uint32_t col) const
	{
		if(row >= rowCount) throw std::runtime_error("row out of bounds");
		if(col >= colCount) throw std::runtime_error("col out of bounds");
		return rows[row][col];
	}


}	// namespace snmpfs
