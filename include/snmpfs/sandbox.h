#pragma once

#include "snmp/device.h"

namespace snmpfs {

	void test();

	void snmp_test();
	void snmpv3_test();
	void snmp_device_test(Device* device);

	void snmp_test_mibs();
	void snmp_test_mibs_table();

	void snmp_device_set(Device* device, std::string objectID, std::string type, std::string value);

	void test_oid();
	void test_table_mib();

	void testTrapHandler();

	void test_snmp_get();
	void test_snmp_set();
	void test_snmp_format();
	void test_snmp_errstat();

	void test_util_trim();

	void test_concurrency();

	void test_writing();

	void test_tqueue();

	class SandboxObject
	{
	public:
		std::string name;
		std::string oid;
	};


	/**
	* @todo write docs
	*/
	class SandboxDevice
	{
	public:

		std::string name;

		// SNMP CONNECTION DATA
		std::string peername;
		std::string community;
		// TODO SNMP3 username, auth, ...

		std::vector<SandboxObject> objects;

		/**
		* Default constructor
		*/
		SandboxDevice();

		/**
		* Destructor
		*/
		~SandboxDevice();


	};

}	// namespace snmpfs
