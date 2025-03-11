#include "snmp/traphandler.h"

#include "snmp/objectid.h"

namespace snmpfs {

	AbstractTrapHandler::AbstractTrapHandler()
	{

	}

	AbstractTrapHandler::~AbstractTrapHandler()
	{

	}

	std::string AbstractTrapHandler::getName() const
	{
		return "Trap Handler";
	}



	AuthTrapHandler::AuthTrapHandler(const AuthData& auth)
	{
		this->auth = auth;
	}

	AuthTrapHandler::~AuthTrapHandler()
	{

	}

	std::string AuthTrapHandler::getName() const
	{
		return "Auth Handler";
	}

	bool AuthTrapHandler::handle(const TrapData& trap)
	{
		// printf("[AuthHandler] PDU version: %ld\n", trap.pdu->version);
		// printf("[AuthHandler] PDU secName: %s \n", trap.pdu->securityName);

		if(trap.pdu->version == SNMP_VERSION_1 || trap.pdu->version == SNMP_VERSION_2c)
		{
			// CHECK VERSION 1/2c ACCESS
			std::string community((char *) trap.pdu->community, trap.pdu->community_len);
			return community == auth.username;
		}
		else
		{
			// TODO CHECK VERSION 3 ACCESS
			// printf("[AuthHandler] PDU v3\n");
			return false;
		}

		return true;
	}



	DeviceTrapHandler::DeviceTrapHandler(const std::vector<Device*>& devices) : devices(devices)
	{

	}

	DeviceTrapHandler::~DeviceTrapHandler()
	{

	}

	std::string DeviceTrapHandler::getName() const
	{
		return "Device Handler";
	}

	bool DeviceTrapHandler::handle(const TrapData& trap)
	{
		// printf("Handling TrapData for %s\n", trap.address.c_str());

		Device* device = findTrapDevice(devices, trap);
		if(device)
		{
			// printf("Device found\n");
			device->processTrap(trap.pdu);
		}
		else
		{
			syslog(LOG_WARNING, "No device found for trap from %s\n", trap.address.c_str());
			return true;
		}
		return true;
	}

	Device* DeviceTrapHandler::findTrapDevice(const std::vector<Device*>& devices, const TrapData& trap)
	{
		for(Device* dev : devices)
		{
			// TODO Maybe find another method for getting the right device (what if DNS is used?)
			if(dev->config.peername.find(trap.address) != std::string::npos)
			{
				return dev;
			}
		}
		return nullptr;
	}



	LogTrapHandler::LogTrapHandler(const std::vector<Device*>& devices) : devices(devices)
	{

	}

	LogTrapHandler::~LogTrapHandler()
	{

	}

	std::string LogTrapHandler::getName() const
	{
		return "Log Handler";
	}

	bool LogTrapHandler::handle(const TrapData& trap)
	{
		// TODO Maybe also move Device into TrapData ?
		Device* device = DeviceTrapHandler::findTrapDevice(devices, trap);

		if(!device)
		{
			syslog(LOG_WARNING, "No device found for trap from %s\n", trap.address.c_str());
			return true;
		}

		netsnmp_variable_list* var;
		for(var = trap.pdu->variables; var; var = var->next_variable)
		{
			ObjectID oid(var->name, var->name_length);

			// TODO Maybe replace with formatting function used in Device
			size_t bufferSize = 1024;
			char* buffer = (char*) malloc(bufferSize);
			size_t outLen = 0;
			sprint_realloc_by_type((u_char **) &buffer, &bufferSize, &outLen, 0, var, NULL, NULL, NULL);
			std::string data(buffer, outLen);
			free(buffer);

			std::stringstream ss;
			ss << "[TRAP]: ";
			ss << ((std::string) oid);
			ss << " ---> ";
			ss << data;
			device->logNotice(ss.str());
		}

		return true;
	}

}	// namespace snmpfs
