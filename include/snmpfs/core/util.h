#pragma once

#include <string>
#include <vector>

namespace snmpfs {

	bool endsWith(const std::string& str, const std::string& ending);
	std::string replace(std::string str, const std::string& search, const std::string& replace);
	std::vector<std::string> split(const std::string& str, char del);
	std::string trim(const std::string& str);

}	// namespace snmpfs
