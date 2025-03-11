#include "configio.h"

#include "core/util.h"
#include "defines.h"
#include <set>

namespace snmpfs {

	ConfigIO::ConfigIO()
	{

	}

	ConfigIO::~ConfigIO()
	{

	}

	std::string ConfigIO::toString(const snmpfsConfig& config)
	{
		std::stringstream ss;

		ss << "Config File: " << config.configPath << std::endl;

		ss << "MIBS (loadSystemMIBs = " << config.loadSystemMIBs <<"):" << std::endl;
		for(const std::filesystem::path& mib : config.mibs)
			ss << mib << std::endl;

		ss << "Templates:" << std::endl;
		for(const TemplateConfig& tmp : config.templates)
		{
			ss << "\tName:    \t"	<< tmp.name		<< std::endl;
			ss << "\tInterval:\t"	<< tmp.interval	<< std::endl;

			for(const ObjectConfig& object : tmp.objects)
			{
				ss << "\t" << "Object: " << std::endl;

				ss << "\t\t" << "Name:			"	<< object.name			<< std::endl;
				ss << "\t\t" << "OID:			"	<< object.rawOID		<< std::endl;
				ss << "\t\t" << "Interval:		"	<< object.interval		<< std::endl;
				ss << "\t\t" << "Type:			"	<< object.type			<< std::endl;
				ss << "\t\t" << "Placeholder:	"	<< object.placeholder	<< std::endl;
				ss << "\t\t" << "Prefix:	"		<< object.placeholder	<< std::endl;

				if(object.type == TABLE)
				{
					ss << "\t\t" << "Columns: " << object.type << std::endl;
					for(const ConfigEntry& column : object.columns)
					{
						ss << "\t\t\t" << "Name:\t"	<< column.name		<< std::endl;
						ss << "\t\t\t" << "OID:\t"	<< column.rawOID	<< std::endl;
					}
				}
			}
		}

		for(const DeviceConfig& device : config.devices)
		{
			ss << "Device:" << std::endl;
			ss << "\t" << "Name:\t\t"	<< device.name			<< std::endl;
			ss << "\t" << "Peername:\t"	<< device.peername		<< std::endl;
			ss << "\t" << "Version:\t"	<< device.auth.version	<< std::endl;

			if(device.auth.version == VERSION_1 || device.auth.version == VERSION_2c)
			{
				ss << "\t" << "Community:\t" << device.auth.username << std::endl;
			}
			else if(device.auth.version == VERSION_3)
			{
				ss << "\t" << "Security Name:\t"	<< device.auth.username			<< std::endl;
				ss << "\t" << "Security Level:\t"	<< device.auth.securityLevel	<< std::endl;

				if(device.auth.securityLevel == authNoPriv || device.auth.securityLevel == authPriv)
					ss << "\t" << "Auth:\t" << device.auth.authAlgorithm << "\t" << device.auth.authPassphrase << std::endl;
				if(device.auth.securityLevel == authPriv)
					ss << "\t" << "Priv:\t" << device.auth.privAlgorithm << "\t" << device.auth.privPassphrase << std::endl;
			}
			ss << "\t" << "Interval:\t" << device.interval << std::endl;

			for(const ObjectConfig& object : device.objects)
			{
				ss << "\t" << "Object: " << std::endl;

				ss << "\t\t" << "Name:    \t"	<< object.name			<< std::endl;
				ss << "\t\t" << "OID:     \t"	<< object.rawOID		<< std::endl;
				ss << "\t\t" << "Interval:\t"	<< object.interval		<< std::endl;
				ss << "\t\t" << "Type:    \t"	<< object.type			<< std::endl;
				ss << "\t\t" << "Prefix:	"	<< object.placeholder	<< std::endl;

				if(object.type == TABLE)
				{
					ss << "\t\t" << "Columns: " << object.type << std::endl;
					for(const ConfigEntry& column : object.columns)
					{
						ss << "\t\t\t" << "Name: "	<< column.name << std::endl;
						ss << "\t\t\t" << "OID: "	<< column.rawOID << std::endl;
					}
				}
			}
		}

		return ss.str();
	}


	bool ConfigIO::read(const std::filesystem::path& configPath, snmpfsConfig& config)
	{
		// Clear provided struct
		config = {};

		// INIT CONFIG
		config.configPath	= configPath;
		config.interval		= DEFAULT_INTERVAL;	// TODO Move somewhere else ?

		// Load XML
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLError err = doc.LoadFile(configPath.c_str());
		if(err != tinyxml2::XMLError::XML_SUCCESS)
		{
			std::string msg = "Could not load XML because " + std::string(tinyxml2::XMLDocument::ErrorIDToName(err));
			printf("%s\n", msg.c_str());
			return false;
		}


		// Check if root is present
		tinyxml2::XMLElement* rootNode = doc.RootElement();
		if(!rootNode)
		{
			printf("No 'snmpfs' root node!\n");
			return false;
		}


		// Check if root has correct name
		std::string rootName = doc.RootElement()->Name();
		if(rootName != "snmpfs")
		{
			// TODO Proper error handling
			printf("Root node with wrong name! Should be snmpfs\n");
			return false;
		}


		// Check interval
		const tinyxml2::XMLAttribute* intervalAttribute	= rootNode->FindAttribute("interval");
		if(intervalAttribute)
		{
			int interval;
			if(intervalAttribute->QueryIntValue(&interval) == tinyxml2::XML_SUCCESS && interval >= 0)
			{
				config.interval = interval;
			}
			else
			{
				printf("'snmpfs' element has invalid value for attribute 'interval'\n");
				return false;
			}
		}


		// Check device
		tinyxml2::XMLElement* deviceElement = rootNode->FirstChildElement("device");
		while(deviceElement != nullptr)
		{
			DeviceConfig deviceConfig = {};
			deviceConfig.interval = config.interval;
			bool valid = true;
			valid &= readDevice(deviceElement, deviceConfig);
			valid &= checkName(config, deviceConfig);
			if(valid) config.devices.push_back(deviceConfig);
			else return false;

			deviceElement = deviceElement->NextSiblingElement("device");
		}


		// Check mibs
		if(rootNode->ChildElementCount("mibs") > 1)
		{
			printf("More than one 'mibs' element found in device %s\n", deviceElement->Name());
			return false;
		}
		tinyxml2::XMLElement* mibsElement = rootNode->FirstChildElement("mibs");
		if(mibsElement)
		{
			const tinyxml2::XMLAttribute* systemAttribute	= mibsElement->FindAttribute("system");
			if(systemAttribute)
			{
				config.loadSystemMIBs = systemAttribute->BoolValue();
			}
			else
			{
				config.loadSystemMIBs = false;
			}

			bool valid = readMIBs(mibsElement, config.mibs);
			if(!valid) return false;
		}
		else
		{
			config.loadSystemMIBs = true;
		}


		// Check template
		tinyxml2::XMLElement* templateElement = rootNode->FirstChildElement("template");
		while(templateElement != nullptr)
		{
			std::string elementName = templateElement->Name();
			if(elementName != "template")
			{
				printf("Found unknown element (%s)\n", templateElement->Name());
				continue;
			}

			TemplateConfig templateConfig = {};
			templateConfig.interval = -1;
			bool valid = true;
			valid &= readTemplate(templateElement, templateConfig);
			valid &= checkName(config, templateConfig);
			if(valid) config.templates.push_back(templateConfig);
			else return false;

			templateElement = templateElement->NextSiblingElement("template");
		}


		// Check traps
		if(rootNode->ChildElementCount("trap") > 1)
		{
			printf("More than one 'trap' element found in snmpfs\n");
			return false;
		}
		tinyxml2::XMLElement* trapElement = rootNode->FirstChildElement("trap");
		if(trapElement)
		{
			bool valid = readTrap(trapElement, config.trap);
			if(!valid) return false;
		}
		else
		{
			config.trap.port = 0;
		}


		// Last step is to replace all placeholders with their corresponding templates
		for(DeviceConfig& device : config.devices)
		{
			bool suc = replaceTemplates(config.templates, device);
			if(!suc) return false;
		}

		// Check if templates introduced duplicate object names
		for(const DeviceConfig& device : config.devices)
		{
			bool suc = checkDuplicates(device);
			if(!suc) return false;
		}

		return true;
	}

	bool ConfigIO::readTemplate(tinyxml2::XMLElement* templateElement, TemplateConfig& config)
	{
		// Check name
		const tinyxml2::XMLAttribute* nameAttribute = templateElement->FindAttribute("name");
		if(!nameAttribute)
		{
			printf("'template' element requires attribute 'name'\n");
			return false;
		}
		config.name = nameAttribute->Value();

		// Check interval
		const tinyxml2::XMLAttribute* intervalAttribute	= templateElement->FindAttribute("interval");
		if(intervalAttribute)
		{
			int interval;
			if(intervalAttribute->QueryIntValue(&interval) == tinyxml2::XML_SUCCESS)
			{
				config.interval = interval;
			}
			else
			{
				printf("'template' element has invalid value for attribute 'interval'\n");
				return false;
			}
		}

		// Read all objects
		bool valid = readObjects(templateElement, config.objects);
		if(!valid)
		{
			printf("Error occured when reading objects of template '%s'\n", config.name.c_str());
			return false;
		}

		// Set interval if template specifies one
		for(ObjectConfig& obj : config.objects)
		{
			if(obj.interval == -1)
			{
				obj.interval = config.interval;
			}
		}

		return true;
	}

	bool ConfigIO::readDevice(tinyxml2::XMLElement* deviceElement, DeviceConfig& config)
	{
		// Check name
		const tinyxml2::XMLAttribute* nameAttribute = deviceElement->FindAttribute("name");
		if(!nameAttribute)
		{
			printf("'device' element requires attribute 'name'\n");
			return false;
		}
		config.name = nameAttribute->Value();

		// Check interval
		const tinyxml2::XMLAttribute* intervalAttribute	= deviceElement->FindAttribute("interval");
		if(intervalAttribute)
		{
			int interval;
			if(intervalAttribute->QueryIntValue(&interval) == tinyxml2::XML_SUCCESS)
			{
				config.interval = interval;
			}
			else
			{
				printf("'device' element has invalid value for attribute 'interval'\n");
				return false;
			}
		}

		// Check snmp
		if(deviceElement->ChildElementCount("snmp") > 1)
		{
			printf("More than one 'snmp' element found in device %s\n", deviceElement->Name());
			return false;
		}
		tinyxml2::XMLElement* snmpElement = deviceElement->FirstChildElement("snmp");
		if(snmpElement)
		{
			bool valid = readSNMP(snmpElement, config);
			if(!valid) return false;
		}
		// TODO if not, offline mode with MIB data ?


		// Check objects
		if(deviceElement->ChildElementCount("objects") > 1)
		{
			printf("More than one 'objects' element found in device %s\n", deviceElement->Name());
			return false;
		}
		tinyxml2::XMLElement* objectsElement = deviceElement->FirstChildElement("objects");
		if(objectsElement)
		{
			bool valid = readObjects(objectsElement, config.objects);
			if(!valid)
			{
				printf("Error occured when reading objects of device '%s'\n", config.name.c_str());
				return false;
			}
		}

		return true;
	}

	bool ConfigIO::readAuth(tinyxml2::XMLElement* element, AuthData& auth)
	{
		// Check version
		const tinyxml2::XMLAttribute* version = element->FindAttribute("version");
		if(!version)
		{
			printf("element requires attribute 'version'\n");
			return false;
		}
		std::string versionString = version->Value();
		if(versionString == "1")
		{
			auth.version = VERSION_1;
		}
		else if(versionString == "2c")
		{
			auth.version = VERSION_2c;
		}
		else if(versionString == "3")
		{
			auth.version = VERSION_3;
		}
		else
		{
			printf("snmp version %s is not supported/inexistant\n", versionString.c_str());
			return false;
		}

		if(auth.version == VERSION_1 || auth.version == VERSION_2c)
		{
			// Check community
			const tinyxml2::XMLAttribute* community = element->FindAttribute("community");
			if(!community)
			{
				printf("element requires attribute 'community'\n");
				return false;
			}
			auth.username = community->Value();
		}
		else
		{
			// Check username
			const tinyxml2::XMLAttribute* username = element->FindAttribute("username");
			if(!username)
			{
				printf("element requires attribute 'username'\n");
				return false;
			}
			auth.username = username->Value();

			// Check auth
			const tinyxml2::XMLAttribute* securityLevel = element->FindAttribute("securityLevel");
			if(!securityLevel)
			{
				printf("element requires attribute 'securityLevel'\n");
				return false;
			}
			std::string securityLevelName = securityLevel->Value();
			if(securityLevelName == "noAuthNoPriv")
			{
				auth.securityLevel = noAuthNoPriv;
			}
			else if(securityLevelName == "authNoPriv")
			{
				auth.securityLevel = authNoPriv;
			}
			else if(securityLevelName == "authPriv")
			{
				auth.securityLevel = authPriv;
			}
			else
			{
				printf("element has unknown 'securityLevel' value of '%s'\n", securityLevelName.c_str());
				return false;
			}

			// CHECK auth
			if(auth.securityLevel != noAuthNoPriv)
			{
				// Check authAlgorithm
				const tinyxml2::XMLAttribute* authAlgorithm = element->FindAttribute("authAlgorithm");
				if(!authAlgorithm)
				{
					printf("element requires attribute 'authAlgorithm'\n");
					return false;
				}
				std::string authAlgorithmName = authAlgorithm->Value();
				if(authAlgorithmName == "MD5")
				{
					auth.authAlgorithm = MD5;
				}
				else if(authAlgorithmName == "SHA")
				{
					auth.authAlgorithm = SHA;
				}
				else
				{
					printf("element has unknown 'authAlgorithm' value of '%s'\n", authAlgorithmName.c_str());
					return false;
				}

				// Check authPassphrase
				const tinyxml2::XMLAttribute* authPassphrase = element->FindAttribute("authPassphrase");
				if(!authPassphrase)
				{
					printf("element requires attribute 'authPassphrase'\n");
					return false;
				}
				auth.authPassphrase = authPassphrase->Value();
			}

			// CHECK priv
			if(auth.securityLevel == authPriv)
			{
				// Check privAlgorithm
				const tinyxml2::XMLAttribute* privAlgorithm = element->FindAttribute("privAlgorithm");
				if(!privAlgorithm)
				{
					printf("element requires attribute 'privAlgorithm'\n");
					return false;
				}
				std::string privAlgorithmName = privAlgorithm->Value();
				if(privAlgorithmName == "AES")
				{
					auth.privAlgorithm = AES;
				}
				else if(privAlgorithmName == "DES")
				{
					auth.privAlgorithm = DES;
				}
				else
				{
					printf("element has unknown 'privAlgorithm' value of '%s'\n", privAlgorithmName.c_str());
					return false;
				}

				// Check privPassphrase
				const tinyxml2::XMLAttribute* privPassphrase = element->FindAttribute("privPassphrase");
				if(!privPassphrase)
				{
					printf("element requires attribute 'privPassphrase'\n");
					return false;
				}
				auth.privPassphrase = privPassphrase->Value();
			}
		}

		return true;
	}


	bool ConfigIO::readSNMP(tinyxml2::XMLElement* snmpElement, DeviceConfig& config)
	{
		// Check peername
		const tinyxml2::XMLAttribute* peername = snmpElement->FindAttribute("peername");
		if(!peername)
		{
			printf("snmp element requires attribute 'peername'\n");
			return false;
		}
		config.peername = peername->Value();

		bool validAuth = readAuth(snmpElement, config.auth);
		if(!validAuth)
		{
			printf("snmp element has errors in the authentication attributes\n");
			return false;
		}

		return true;
	}

	bool ConfigIO::readMIBs(tinyxml2::XMLElement* mibsElement, std::vector<std::filesystem::path>& mibs)
	{
		// Check for mib elements
		tinyxml2::XMLElement* mibElement = mibsElement->FirstChildElement("mib");
		while(mibElement != nullptr)
		{
			const tinyxml2::XMLAttribute* pathAttribute	= mibElement->FindAttribute("path");
			if(!pathAttribute)
			{
				printf("mib element requires attribute 'path'\n");
				return false;
			}

			std::filesystem::path path = pathAttribute->Value();
			mibs.emplace_back(path);

			mibElement = mibElement->NextSiblingElement("mib");
		}

		return true;
	}


	bool ConfigIO::readObjects(tinyxml2::XMLElement* objectsElement, std::vector<ObjectConfig>& objects)
	{
		// Check for object elements
		tinyxml2::XMLElement* objectElement = objectsElement->FirstChildElement();
		while(objectElement != nullptr)
		{

			ObjectConfig objectConfig = {};
			objectConfig.interval = -1;	// In order to replace them later if not specified by XML
			bool valid = true;
			valid &= readObject(objectElement, objectConfig);
			valid &= checkName(objects, objectConfig);
			if(valid) objects.push_back(objectConfig);
			else return false;

			objectElement = objectElement->NextSiblingElement();
		}

		return true;
	}

	bool ConfigIO::readObject(tinyxml2::XMLElement* objectElement, ObjectConfig& config)
	{
		std::string elementName = objectElement->Name();
		bool requiresName	= false;
		bool requiresOID	= false;

		if(elementName == "scalar")
		{
			config.type		= ObjectType::SCALAR;
			requiresName	= false;
			requiresOID		= true;
		}
		else if(elementName == "table")
		{
			config.type		= ObjectType::TABLE;
			requiresName	= false;
			requiresOID		= true;
		}
		else if(elementName == "tree")
		{
			config.type		= ObjectType::TREE;
			requiresName	= true;
			requiresOID		= true;
		}
		else if(elementName == "reuse")
		{
			config.type		= ObjectType::REUSE;
			requiresName	= true;
			requiresOID		= false;
		}
		else
		{
			printf("Found unknown object element '%s'\n", elementName.c_str());
			return false;
		}

		const tinyxml2::XMLAttribute* nameAttribute	= objectElement->FindAttribute("name");

		if(nameAttribute)
		{
			config.name	= nameAttribute->Value();
		}
		else if(requiresName)
		{
			printf("'%s' element requires attribute 'name'\n", elementName.c_str());
			return false;
		}

		const tinyxml2::XMLAttribute* oidAttribute	= objectElement->FindAttribute("oid");
		if(oidAttribute)
		{
			config.rawOID	= oidAttribute->Value();

			// TODO Maybe in the future automatically add the 0 to OID ?
			if(config.type == SCALAR && !endsWith(config.rawOID, ".0"))
			{
				printf("'%s' element '%s' requires attribute 'oid' ending with '.0'\n", elementName.c_str(), config.name.c_str());
				return false;
			}
		}
		else if(requiresOID)
		{
			printf("'%s' element requires attribute 'oid'\n", elementName.c_str());
			return false;
		}

		const tinyxml2::XMLAttribute* intervalAttribute	= objectElement->FindAttribute("interval");
		if(intervalAttribute)
		{
			int interval;
			if(intervalAttribute->QueryIntValue(&interval) == tinyxml2::XML_SUCCESS)
			{
				config.interval = interval;
			}
			else
			{
				printf("'%s' element has invalid value for attribute 'interval'\n", elementName.c_str());
				return false;
			}
		}

		const tinyxml2::XMLAttribute* placeholderAttribute	= objectElement->FindAttribute("placeholder");
		if(placeholderAttribute)
		{
			bool placeholder;
			if(placeholderAttribute->QueryBoolValue(&placeholder) == tinyxml2::XML_SUCCESS)
			{
				config.placeholder = placeholder;
			}
			else
			{
				printf("'%s' element has invalid value for attribute 'placeholder'\n", elementName.c_str());
				return false;
			}
		}


		// Check prefix
		const tinyxml2::XMLAttribute* prefixAttribute = objectElement->FindAttribute("prefix");
		if(prefixAttribute)
		{
			bool prefix;
			if(prefixAttribute->QueryBoolValue(&prefix) == tinyxml2::XML_SUCCESS)
			{
				config.prefix = prefix;
			}
			else
			{
				printf("'template' element has invalid value for attribute 'prefix'\n");
				return false;
			}
		}

		// Check table columns
		if(config.type == TABLE)
		{
			bool suc = readTable(objectElement, config);
			if(!suc)
			{
				printf("Error reading table columns!\n");
				return false;
			}
		}

		return true;
	}

	bool ConfigIO::readTable(tinyxml2::XMLElement* tableElement, ObjectConfig& config)
	{
		// Check for column elements
		tinyxml2::XMLElement* columnElement = tableElement->FirstChildElement();
		while(columnElement != nullptr)
		{
			ConfigEntry column;

			std::string elementName = columnElement->Name();
			if(elementName != "column")
			{
				printf("'table' element should only contain column elements\n");
				return false;
			}

			const tinyxml2::XMLAttribute* nameAttribute	= columnElement->FindAttribute("name");
			if(!nameAttribute)
			{
				printf("'column' element requires attribute 'name'\n");
				return false;
			}
			column.name = nameAttribute->Value();

			const tinyxml2::XMLAttribute* oidAttribute	= columnElement->FindAttribute("oid");
			if(!oidAttribute)
			{
				printf("'column' element requires attribute 'oid'\n");
				return false;
			}
			column.rawOID	= oidAttribute->Value();

			config.columns.push_back(column);
			columnElement = columnElement->NextSiblingElement();
		}


		return true;
	}

	bool ConfigIO::readTrap(tinyxml2::XMLElement* trapElement, TrapConfig& config)
	{
		bool validAuth = readAuth(trapElement, config.auth);
		if(!validAuth)
		{
			printf("trap element has errors in the authentication attributes\n");
			return false;
		}

		const tinyxml2::XMLAttribute* portAttribute	= trapElement->FindAttribute("port");
		if(!portAttribute)
		{
			printf("'trap' element requires attribute 'port'\n");
			return false;
		}
		config.port	= portAttribute->IntValue();

		// TODO other stuff concerning TrapHandlers

		return true;
	}


	bool ConfigIO::replaceTemplates(const std::vector<TemplateConfig>& templates, DeviceConfig& device)
	{
		// while loop to allow nested templates
		bool foundReuse = true;
		size_t levels = 0;
		while(foundReuse && levels < 32)
		{
			foundReuse = false;
			levels++;

			for(size_t i = 0; i < device.objects.size(); i++)
			{
				const ObjectConfig& placeholder = device.objects[i];
				if(placeholder.type != REUSE) continue;
				foundReuse = true;
				int32_t interval	= placeholder.interval;
				bool prefix			= placeholder.prefix;

				// Search template
				const TemplateConfig* tmplt = nullptr;
				for(const TemplateConfig& search : templates)
				{
					if(search.name == placeholder.name)
					{
						tmplt = &search;
						break;
					}
				}

				if(!tmplt)
				{
					printf("template '%s' was not found\n", placeholder.name.c_str());
					return false;
				}

				// Remove template object
				device.objects.erase(device.objects.begin() + i);

				// Add objects from template
				for(size_t t = 0; t < tmplt->objects.size(); t++)
				{
					ObjectConfig copy = tmplt->objects[t];
					if(interval != -1)
					{
						copy.interval = interval;
					}
					if(prefix)
					{
						copy.name = tmplt->name + "_" + copy.name;
					}
					device.objects.insert(device.objects.begin() + i + t, copy);
				}
			}
		}


		// Set unset interval values with device interval
		for(ObjectConfig& obj : device.objects)
		{
			if(obj.interval == -1)
			{
				obj.interval = device.interval;
			}
		}

		return true;
	}




	bool ConfigIO::checkDuplicates(const DeviceConfig& device)
	{
		std::set<std::string> names;
		for(const ObjectConfig& obj : device.objects)
		{
			if(names.contains(obj.name))
			{
				printf("Device '%s' has duplicated object '%s'! Please check device and templates used!\n", device.name.c_str(), obj.name.c_str());
				return false;
			}
			names.insert(obj.name);
		}
		return true;
	}


	bool snmpfs::ConfigIO::checkName(const std::vector<snmpfs::ObjectConfig>& objects, const ObjectConfig& object)
{
		for(const ObjectConfig& obj : objects)
		{
			if(obj.name == object.name)
			{
				printf("Object with name '%s' already exists!\n", object.name.c_str());
				return false;
			}
		}
		return true;
	}

	bool ConfigIO::checkName(const snmpfsConfig& snmpfsConfig, const DeviceConfig& device)
	{
		for(const DeviceConfig& dev : snmpfsConfig.devices)
		{
			if(dev.name == device.name)
			{
				printf("Device with name '%s' already exists!\n", device.name.c_str());
				return false;
			}
		}
		return true;
	}

	bool ConfigIO::checkName(const snmpfsConfig& snmpfsConfig, const TemplateConfig& tmpt)
	{
		for(const TemplateConfig& tmp : snmpfsConfig.templates)
		{
			if(tmp.name == tmpt.name)
			{
				printf("Template with name '%s' already exists!\n", tmpt.name.c_str());
				return false;
			}
		}
		return true;
	}





}	// namespace snmpfs
