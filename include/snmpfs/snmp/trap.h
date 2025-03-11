#pragma once

#include "snmp_ext.h"
#include <string>

namespace snmpfs {

	struct TrapData {
		std::string address;
		netsnmp_pdu* pdu;
	};

}	// namespace snmpfs
