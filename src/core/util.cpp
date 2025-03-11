#include "core/util.h"

#include <sstream>

namespace snmpfs {

	bool endsWith(const std::string& str, const std::string& ending)
	{
		if(ending.size() > str.size())
			return false;

		return str.compare(str.size() - ending.size(), ending.size(), ending) == 0;
	}

	std::string replace(std::string str, const std::string& search, const std::string& replace)
	{
		size_t index = 0;
		while(true)
		{
			index = str.find(search, index);
			if(index == std::string::npos) break;

			str.replace(index, search.size(), replace);
			index += replace.size();
		}
		return str;
	}

	std::vector<std::string> split(const std::string& str, char del)
	{
		std::vector<std::string> split;

		std::string tmp;
		std::stringstream ss(str);
		while(!ss.eof())
		{
			std::getline(ss, tmp, del);
			split.emplace_back(tmp);
		}

		return split;
	}

	std::string trim(const std::string& str)
	{
		size_t start	= 0;
		size_t end		= str.size() - 1;

		// Find beginning
		while(std::isspace(str[start]) && start < str.size())
			start++;

		// Find end
		while(std::isspace(str[end]) && end > 0)
			end--;

		return str.substr(start, end - start + 1);
	}


}	// namespace snmpfs
