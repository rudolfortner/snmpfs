#pragma once

#include <filesystem>
#include <stdint.h>
#include <string>
#include <vector>

namespace snmpfs {

	enum DeviceVersion {
		VERSION_1,
		VERSION_2c,
		VERSION_3
	};

	enum DeviceSecurityLevel {
		noAuthNoPriv,
		authNoPriv,
		authPriv
	};

	enum DeviceAuthAlgorithm {
		MD5,
		SHA
	};

	enum DevicePrivAlgorithm {
		AES,
		DES
	};

	enum ObjectType {
		SCALAR,
		TABLE,
		TREE,
		REUSE
	};


	/**
	 * Holds authentication data required for SNMP
	 * Either used for sending requests (GET, SET, ...) or for authenticating incoming traps
	 */
	struct AuthData {
		DeviceVersion version;				///< protocol version used

		std::string username;				///< community for v1/v2c or username for v3 respectively

		DeviceSecurityLevel securityLevel;	///< noAuthNoPriv/noauth OR authNoPriv/auth OR authPriv/priv
		DeviceAuthAlgorithm authAlgorithm;	///< e.g. SHA-256
		std::string authPassphrase;			///< Password for authentication
		DevicePrivAlgorithm privAlgorithm;	///< e.g. AES
		std::string privPassphrase;			///< Password for privacy (encryption)
	};

	struct ConfigEntry {
		std::string name;					///< name in filesystem
		std::string rawOID;					///< unparsed OID
		int32_t interval;					///< update interval in seconds
	};

	struct ObjectConfig : ConfigEntry {
		// Fields from ConfigEntry
		ObjectType type		= SCALAR;
		std::vector<ConfigEntry> columns;	///< TABLE ONLY
		bool prefix			= false;		///< REUSE ONLY
		bool placeholder	= false;		///< TREE ONLY
	};

	struct TemplateConfig {
		std::string name;
		int32_t interval;

		// OBJECTS
		std::vector<ObjectConfig> objects;
	};


	struct TrapConfig {
		AuthData auth;
		uint16_t port;

		// TODO Enable certain handlers (LogHandler, DeviceHandler, AlertHandler, ...)
	};

	struct DeviceConfig {
		// GENERAL
		std::string name;				///< name in filesystem
		std::string peername;			///< name or address of default peer (may include transport specifier and/or port number)
		int32_t interval;
		// TODO custom path

		AuthData auth;					///< authentication data

		// OBJECTS
		std::vector<ObjectConfig> objects;
	};

	struct snmpfsConfig {
		std::filesystem::path configPath;
		std::filesystem::path mountPoint;

		int32_t interval;
		bool loadSystemMIBs;

		std::vector<DeviceConfig> devices;
		std::vector<std::filesystem::path> mibs;
		std::vector<TemplateConfig> templates;
		TrapConfig trap;
	};

}	// namespace snmpfs
