#pragma once

#include "config.h"

#include <filesystem>
#include <tinyxml2.h>
#include <sstream>
#include <string>
#include <vector>

namespace snmpfs {

	/**
	* Class for loading the configuration from the XML file into our internal representations defined in config.h
	*/
	class ConfigIO
	{
	public:
		/**
		* Default constructor
		*/
		ConfigIO();

		/**
		* Destructor
		*/
		~ConfigIO();

		/**
		 * Converts a given configuration into a string representation
		 */
		static std::string toString(const snmpfsConfig& config);

		/**
		 * Loads the configuration from the given path into the internal representation
		 */
		static bool read(const std::filesystem::path& configPath, snmpfsConfig& config);

	private:
		static bool readTemplate(tinyxml2::XMLElement* templateElement, TemplateConfig& config);
		static bool readDevice(tinyxml2::XMLElement* deviceElement, DeviceConfig& config);

		static bool readAuth(tinyxml2::XMLElement* element, AuthData& auth);
		static bool readSNMP(tinyxml2::XMLElement* snmpElement, DeviceConfig& config);
		static bool readMIBs(tinyxml2::XMLElement* mibsElement, std::vector<std::filesystem::path>& mibs);
		static bool readObjects(tinyxml2::XMLElement* objectsElement, std::vector<ObjectConfig>& objects);
		static bool readObject(tinyxml2::XMLElement* objectElement, ObjectConfig& config);
		static bool readTable(tinyxml2::XMLElement* tableElement, ObjectConfig& config);
		static bool readTrap(tinyxml2::XMLElement* trapElement, TrapConfig& config);

		static bool replaceTemplates(const std::vector<TemplateConfig>& templates, DeviceConfig& device);

		static bool checkDuplicates(const DeviceConfig& device);
		static bool checkName(const std::vector<ObjectConfig>& objects, const ObjectConfig& object);
		static bool checkName(const snmpfsConfig& snmpfsConfig, const DeviceConfig& device);
		static bool checkName(const snmpfsConfig& snmpfsConfig, const TemplateConfig& tmpt);
	};

}	// namespace snmpfs
