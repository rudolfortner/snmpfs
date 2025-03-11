#pragma once

#include "config.h"
#include "fuse/virtuallogger.h"
#include "snmp/device.h"
#include "snmp/snmp_ext.h"
#include "trap.h"

#include <string>
#include <vector>

namespace snmpfs {

	/**
	* Abstract base class to implement all kinds of "Handlers"
	*/
	class AbstractTrapHandler
	{
	public:
		/**
		* Default constructor
		*/
		AbstractTrapHandler();

		/**
		 * Destructor
		 */
		virtual ~AbstractTrapHandler();

		virtual std::string getName() const;
		virtual bool handle(const TrapData& trap) = 0;
	};

	/**
	* AuthTrapHandler used to check if a trap from an authorized entity is received
	*/
	class AuthTrapHandler : public AbstractTrapHandler
	{
	public:
		/**
		* Default constructor
		*/
		AuthTrapHandler(const AuthData& auth);

		/**
		* Destructor
		*/
		~AuthTrapHandler();

		std::string getName() const;
		bool handle(const TrapData& trap);

	private:
		AuthData auth;
	};

	/**
	* DeviceTrapHandler used to update Device Objects
	*/
	class DeviceTrapHandler : public AbstractTrapHandler
	{
	public:
		/**
		* Default constructor
		*/
		DeviceTrapHandler(const std::vector<Device*>& devices);

		/**
		* Destructor
		*/
		~DeviceTrapHandler();

		std::string getName() const;
		bool handle(const TrapData& trap);

		static Device* findTrapDevice(const std::vector<Device*>& devices, const TrapData& trap);

	private:
		const std::vector<Device*>& devices;
	};

	/**
	* LogTrapHandler used to Log received traps in an virtual file
	* TODO Maybe enable logging to persistent storage as well ?
	*/
	class LogTrapHandler : public AbstractTrapHandler
	{
	public:
		/**
		* Default constructor
		*/
		LogTrapHandler(const std::vector<Device*>& devices);

		/**
		* Destructor
		*/
		~LogTrapHandler();

		std::string getName() const;
		bool handle(const TrapData& trap);

	private:
		const std::vector<Device*>& devices;
	};

}	// namespace snmpfs
