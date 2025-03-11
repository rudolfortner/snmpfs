#pragma once

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/session_api.h>
#include <net-snmp/pdu_api.h>
#include <net-snmp/utilities.h>
#include <net-snmp/varbind_api.h>
#include <net-snmp/snmpv3_api.h>

#include <string>

namespace snmpfs {

	std::string snmp_error_code_name(long int code);
	char snmp_type2char(u_char type);
	void snmp_syslog_err(snmp_session* session);

}	// namespace snmpfs
