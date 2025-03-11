#pragma once

#include <stdint.h>
#include <string>
#include <vector>

namespace snmpfs {


	/**
	 * Class used for loading CSV data from a string
	 * It is very basic and might not support all possible variants like quotes in strings, etc
	 */
	class csvData {
		using csvRowData = std::vector<std::string>;

	public:
		static csvData of(const std::string& data, char columnDelimiter = ',', char lineDelimiter = '\n');

		bool empty() const;
		uint32_t getColumnCount() const;
		uint32_t getRowCount() const;

		const csvRowData& getRow(uint32_t row) const;
		const std::string& get(uint32_t row, uint32_t col) const;


	private:
		uint32_t colCount, rowCount;
		std::vector<csvRowData> rows;

	};

}	// namespace snmpfs
