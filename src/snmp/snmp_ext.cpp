#include "snmp/snmp_ext.h"

#include <stdexcept>

#define ADD_CASE(X)	case X: return #X

namespace snmpfs {

	std::string snmp_error_code_name(long int code)
	{
		switch(code)
		{
			ADD_CASE(SNMP_ERR_NOERROR);
			ADD_CASE(SNMP_ERR_TOOBIG);
			ADD_CASE(SNMP_ERR_NOSUCHNAME);
			ADD_CASE(SNMP_ERR_BADVALUE);
			ADD_CASE(SNMP_ERR_READONLY);
			ADD_CASE(SNMP_ERR_GENERR);

			ADD_CASE(SNMP_ERR_NOACCESS);
			ADD_CASE(SNMP_ERR_WRONGTYPE);
			ADD_CASE(SNMP_ERR_WRONGLENGTH);
			ADD_CASE(SNMP_ERR_WRONGENCODING);
			ADD_CASE(SNMP_ERR_WRONGVALUE);
			ADD_CASE(SNMP_ERR_NOCREATION);
			ADD_CASE(SNMP_ERR_INCONSISTENTVALUE);
			ADD_CASE(SNMP_ERR_RESOURCEUNAVAILABLE);
			ADD_CASE(SNMP_ERR_COMMITFAILED);
			ADD_CASE(SNMP_ERR_UNDOFAILED);
			ADD_CASE(SNMP_ERR_AUTHORIZATIONERROR);
			ADD_CASE(SNMP_ERR_NOTWRITABLE);
			ADD_CASE(SNMP_ERR_INCONSISTENTNAME);

			ADD_CASE(SNMPERR_RANGE);
			ADD_CASE(SNMPERR_VALUE);
			ADD_CASE(SNMPERR_VAR_TYPE);
			ADD_CASE(SNMPERR_BAD_NAME);

			default:	return "UNKNOWN CODE " + std::to_string(code);
		}
	}

	char snmp_type2char(u_char type)
	{
		switch(type)
		{
			case ASN_INTEGER:		return 'i';
			case ASN_UINTEGER:		return 'u';
			case ASN_TIMETICKS:		return 't';
			case ASN_IPADDRESS:		return 'a';
			case ASN_OBJECT_ID:		return 'o';

			case ASN_OCTET_STR:		return 's';
			case ASN_BIT_STR:		throw std::runtime_error("ASN_BIT_STR");

			case ASN_UNSIGNED64:	return 'U';
			case ASN_INTEGER64:		return 'I';
			case ASN_FLOAT:			return 'F';
			case ASN_DOUBLE:		return 'D';

			case SNMP_NOSUCHOBJECT:	return '\0';

			default:	return '=';
		}
	}



	void snmp_syslog_err(snmp_session* session)
	{
		char* error;
		snmp_error(session, NULL, NULL, &error);
		syslog(LOG_ERR, "%s", error);
		delete error;
	}


}	// namespace snmpfs
