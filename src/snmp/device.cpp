#include "snmp/device.h"

#include "core/util.h"
#include <algorithm>
#include <assert.h>
#include <cmath>

namespace snmpfs {

	Device::Device(snmpFS* snmpfs, const DeviceConfig& config) : snmpfs(snmpfs), name(config.name), config(config)
	{
		assert(snmpfs);
	}

	Device::~Device()
	{

	}

	bool Device::initSNMP()
	{
		logInfo("Initializing SNMP");

		// SETUP SESSION
		netsnmp_session snmpSession;
		snmp_sess_init(&snmpSession);

		snmpSession.peername	= (char*) config.peername.c_str();
		snmpSession.retries	= 3;

		// SET SNMP VERSION
		switch(config.auth.version)
		{
			case VERSION_1:		snmpSession.version = SNMP_VERSION_1;	break;
			case VERSION_2c:	snmpSession.version = SNMP_VERSION_2c;	break;
			case VERSION_3:		snmpSession.version = SNMP_VERSION_3;	break;
			default:			throw std::runtime_error("Unknown DeviceVersion");
		}

		// SET COMMUNITY
		if(config.auth.version == VERSION_1 || config.auth.version == VERSION_2c)
		{
			// Version 1 and 2c
			snmpSession.community		= (unsigned char*) config.auth.username.c_str();
			snmpSession.community_len	= config.auth.username.size();
		}

		// SETUP SNMPv3
		if(config.auth.version == VERSION_3)
		{
			// Version 3
			snmpSession.securityName	= (char*) config.auth.username.c_str();
			snmpSession.securityNameLen	= config.auth.username.size();

			switch(config.auth.securityLevel)
			{
				case noAuthNoPriv:	snmpSession.securityLevel = SNMP_SEC_LEVEL_NOAUTH;		break;
				case authNoPriv:	snmpSession.securityLevel = SNMP_SEC_LEVEL_AUTHNOPRIV;	break;
				case authPriv:		snmpSession.securityLevel = SNMP_SEC_LEVEL_AUTHPRIV;	break;
				default:			throw std::runtime_error("Unknown DeviceSecurityLevel");
			}

			if(config.auth.securityLevel == authNoPriv || config.auth.securityLevel == authPriv)
			{
				switch(config.auth.authAlgorithm)
				{
					case MD5:
						snmpSession.securityAuthProto		= usmHMACMD5AuthProtocol;
						snmpSession.securityAuthProtoLen	= sizeof(usmHMACSHA1AuthProtocol) / sizeof(oid);
						break;
					case SHA:
						snmpSession.securityAuthProto		= usmHMACSHA1AuthProtocol;
						snmpSession.securityAuthProtoLen	= sizeof(usmHMACSHA1AuthProtocol) / sizeof(oid);
						break;
					default: throw std::runtime_error("Unknown DeviceAuthAlgorithm");
				}
				snmpSession.securityAuthKeyLen = USM_AUTH_KU_LEN;

				int res = generate_Ku(snmpSession.securityAuthProto, snmpSession.securityAuthProtoLen,
									(const u_char*) config.auth.authPassphrase.c_str(), config.auth.authPassphrase.size(),
									snmpSession.securityAuthKey, &snmpSession.securityAuthKeyLen);

				if(res != SNMPERR_SUCCESS)
				{
					logErr("Error running generate_Ku\n");
					return false;
				}
			}

			if(config.auth.securityLevel == authPriv)
			{
				switch(config.auth.privAlgorithm)
				{
					case AES:
						snmpSession.securityPrivProto		= usmAESPrivProtocol;
						snmpSession.securityPrivProtoLen	= sizeof(usmAESPrivProtocol) / sizeof(oid);
						break;
					case DES:
						snmpSession.securityPrivProto		= usmDESPrivProtocol;
						snmpSession.securityPrivProtoLen	= sizeof(usmDESPrivProtocol) / sizeof(oid);
						break;
					default: throw std::runtime_error("Unknown DevicePrivAlgorithm");
				}
				snmpSession.securityPrivKeyLen = USM_PRIV_KU_LEN;

				int res = generate_Ku(snmpSession.securityAuthProto, snmpSession.securityAuthProtoLen,
									(const u_char*) config.auth.privPassphrase.c_str(), config.auth.privPassphrase.size(),
									snmpSession.securityPrivKey, &snmpSession.securityPrivKeyLen);

				if(res != SNMPERR_SUCCESS)
				{
					logErr("Error running generate_Ku\n");
					return false;
				}
			}
		}

		// CONNECT
		logInfo("Opening SNMP session");
		snmpHandle = snmp_sess_open(&snmpSession);
		if(snmpHandle == NULL)
		{
			char* err;
			snmp_error(&snmpSession, NULL, NULL, &err);
			std::string error = err;
			delete err;

			logErr("SNMP Session could not be created: " + error);
			return false;
		}

		return true;
	}

	void Device::cleanupSNMP()
	{
		logInfo("Closing SNMP session");
		snmp_sess_close(snmpHandle);
		snmpHandle = NULL;
	}


	void Device::freeObjects()
	{
		for(auto& [interval, task] : tasks)
		{
			for(const auto& [id, obj] : task.objects)
			{
				delete obj;
			}
			snmpfs->taskManager.removeTask(&task);
		}
		tasks.clear();
	}

	Object* Device::lookupObject(ObjectID oid, uint32_t interval) const
	{
		if(tasks.contains(interval))
		{
			const UpdateTask& task = tasks.at(interval);
			if(task.objects.contains(oid))
			{
				return task.objects.at(oid);
			}
		}
		return nullptr;
	}

	void Device::registerObject(Object* obj, uint32_t interval)
	{
		UpdateTask& task = tasks[interval];
		task.device = this;
		task.setInterval(interval);
		task.objects[obj->getID()] = obj;
		snmpfs->taskManager.addTask(&task);
	}

	void Device::unregisterObject(Object* obj)
	{
		for(auto& [interval, task] : tasks)
		{
			task.objects.erase(obj->getID());
		}
	}




	bool Device::formatVariable(netsnmp_variable_list* var, std::string& data) const
	{
		// TODO Move buffer outside of loop to minimize heap allocations
		// TODO Buffer has to be at least as large as biggest string that is received, otherwise we get an error
		size_t bufferSize = 256;
		if(var->type == ASN_OCTET_STR) bufferSize = 64 * 1024;
		char* buffer = (char*) malloc(bufferSize);
		size_t outLen = 0;

		// TODO Check if thread safe ?
		netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_QUICK_PRINT, true);
		netsnmp_ds_set_int(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_OID_OUTPUT_FORMAT, NETSNMP_OID_OUTPUT_NUMERIC);

		// TODO Could additional infos be used? subtree->enums, subtree->hint, units
		bool suc = sprint_realloc_by_type((u_char **) &buffer, &bufferSize, &outLen, 0, var, NULL, NULL, NULL);
		data.assign(buffer, outLen);
		free(buffer);

		// DEBUG
	#if DEBUG_FORMAT_VAR
		ObjectID id(var->name, var->name_length);
		printf("VALUE for %s\n", ((std::string) id).c_str());
		printf("VALUE printed %s\n", suc ? "TRUE" : "FALSE");
		printf("VALUE len is %zu\n", outLen);
		printf("VALUE str is %s\n", data.c_str());
	#endif
		if(!suc) return suc;

		// Strings are shipped in quotes
		if(data.starts_with('\"') && data.ends_with('\"'))
			data = data.substr(1, data.length() - 2);

		// Quotes in strings are escaped...
		data = replace(data, "\\\"", "\"");

		return true;
	}


	bool Device::probe(ObjectID oid) const
	{
		if(!snmpHandle)
			std::runtime_error("Device is not connected");

		netsnmp_pdu* pdu;
		netsnmp_pdu* response;

		pdu = snmp_pdu_create(SNMP_MSG_GET);
		snmp_add_null_var(pdu, oid, oid);

		if(!sendPDU(pdu, &response, "Probe")) return false;

		assert(response);
		bool stat = response->errstat == SNMP_ERR_NOERROR;
		snmp_free_pdu(response);

		return stat;
	}

	bool Device::next(ObjectID& oid) const
	{
		ObjectData data;
		return next(oid, data);
	}

	bool Device::next(ObjectID& oid, ObjectData& data) const
	{
		if(!snmpHandle)
			std::runtime_error("Device is not connected");

		netsnmp_pdu* pdu		= nullptr;
		netsnmp_pdu* response	= nullptr;

		pdu = snmp_pdu_create(SNMP_MSG_GETNEXT);
		snmp_add_null_var(pdu, oid, oid);

		if(!sendPDU(pdu, &response, "GetNext")) return false;
		assert(response);

		netsnmp_variable_list* var = response->variables;
		if( var->type == SNMP_ENDOFMIBVIEW		||
			var->type == SNMP_NOSUCHOBJECT		||
			var->type == SNMP_NOSUCHINSTANCE	||
			var->type == ASN_NULL)
		{
			snmp_free_pdu(response);
			return false;
		}

		oid = ObjectID(var->name, var->name_length);

		data.id		= oid;
		data.type	= snmp_type2char(var->type);
		formatVariable(var, data.data);

		snmp_free_pdu(response);
		return true;
	}


	ObjectData Device::get(const ObjectID& id) const
	{
		netsnmp_pdu* pdu = snmp_pdu_create(SNMP_MSG_GET);
		snmp_add_null_var(pdu, id, id);

		netsnmp_pdu* response;
		if(!sendPDU(pdu, &response, "GET")) return {};

		assert(response);
		std::vector<ObjectData> res = processResponse(response);
		snmp_free_pdu(response);
		return res[0];
	}

	ObjectData Device::set(ObjectID oid, char type, std::string data) const
	{
		if(!snmpHandle)
			std::runtime_error("Device is not connected");

		// logErr("Trying to set " + ((std::string) oid) + " with type " + std::to_string(type) + " to " + data);

		netsnmp_pdu* pdu;
		netsnmp_pdu* response;

		pdu = snmp_pdu_create(SNMP_MSG_SET);
		int suc_add = snmp_add_var(pdu, oid, oid, type, data.c_str());
		if(suc_add != 0)
		{
			// WE ASSUME THAT DATA IS ALWAYS SET CORRECTLY
			// SO WHEN THIS ERROR HAPPENS IT IS REALLY NOT ALLOWED
			// THIS ERROR HAPPENS E.G. WHEN AN INACCESSIBLE OBJECT IS SET
			ObjectData ret;
			ret.id		= oid;
			ret.type	= type;
			ret.data	= data;
			ret.valid	= false;
			ret.error	= suc_add;
			snmp_free_pdu(pdu);
			return ret;
		}

		if(!sendPDU(pdu, &response, "SET")) return {};

		assert(response);
		std::vector<ObjectData> res = processResponse(response);
		assert(res.size() == 1);
		snmp_free_pdu(response);
		return res[0];
	}

	std::vector<ObjectData> Device::walk() const
	{
		if(!snmpHandle)
			std::runtime_error("Device is not connected");

		ObjectData data;
		ObjectID oid(".");
		std::vector<ObjectData> objects;

		while(true)
		{
			bool success = next(oid, data);
			if(!success) break;
			objects.emplace_back(data);
		}

		return objects;
	}

	std::vector<ObjectData> Device::walkSubtree(const ObjectID& oid) const
	{
		if(!snmpHandle)
			std::runtime_error("Device is not connected");


		ObjectData currentData;
		ObjectID currentOID = oid;
		std::vector<ObjectData> objects;

		while(next(currentOID, currentData))
		{
			if(!oid.isAncestorOf(currentOID)) break;
			objects.emplace_back(currentData);
		}

		return objects;
	}




	Device::Status Device::checkStatus() const
	{
		ObjectID oid(".");
		netsnmp_pdu* res;
		netsnmp_pdu* pdu = snmp_pdu_create(SNMP_MSG_GETNEXT);
		snmp_add_null_var(pdu, oid, oid);

		snmpMutex.lock();
		int status = snmp_sess_synch_response(snmpHandle, pdu, &res);
		snmpMutex.unlock();

		if(res) snmp_free_pdu(res);

		// TODO Also return INACCESSIBLE ?
		switch(status)
		{
			case STAT_SUCCESS:	return Device::Status::ONLINE;
			case STAT_TIMEOUT:
			case STAT_ERROR:	return Device::Status::OFFLINE;
			default: throw std::runtime_error("Undefined status code");
		}
	}



	void Device::update()
	{
		for(const auto& [interval, task] : tasks)
		{
			updateObjects(task.objects);
		}
	}

	bool Device::updateObjects(const std::map<ObjectID, Object*>& objects)
	{
		if(!snmpHandle)
			std::runtime_error("Device is not connected");

		bool suc = true;
		for(auto& [oid, obj] : objects)
		{
			suc &= obj->update();
		}

		return suc;
	}


	bool Device::sendPDU(netsnmp_pdu* pdu, netsnmp_pdu** response, const std::string& op) const
	{
		snmpMutex.lock();
		int status = snmp_sess_synch_response(snmpHandle, pdu, response);
		snmpMutex.unlock();

		snmpfs->proc.snmpRequestCount.inc();
		snmpfs->proc.snmpLastRequest.setNow();

		if(status == STAT_ERROR)
		{
			logErr("Operation " + op + " failed");
			snmpfs->proc.snmpRequestErrors.inc();
			return false;
		}

		if(status == STAT_TIMEOUT)
		{
			logErr("Operation " + op + " failed (no response from device)");
			snmpfs->proc.snmpRequestTimeouts.inc();
			return false;
		}

		assert(status == STAT_SUCCESS);
		return true;
	}

	void Device::printResponse(netsnmp_pdu* response) const
	{
		// TODO Should this be logged via logger ?
		printf("|--------------------------------------------------\n");
		printf("| Response for Device: %s\n", config.peername.c_str());
		printf("| errstat:\t%ld\t%s\n", response->errstat, snmp_errstring(response->errstat));
		printf("| errindex:\t%ld\n", response->errindex);
		printf("|--------------------------------------------------\n");


		size_t idx = 1;
		for(netsnmp_variable_list* var = response->variables; var; var = var->next_variable)
		{
			std::string data;
			ObjectID name(var->name, var->name_length);
			bool suc = formatVariable(var, data);
			if(!suc) data = "NIL";

			printf("| %2zu | %s | '%s'\n", idx++, ((std::string) name).c_str(), data.c_str());
		}

		printf("|--------------------------------------------------\n");
		fflush(stdout);

		throw std::runtime_error("Response contains error!");
	}

	std::vector<ObjectData> Device::processResponse(netsnmp_pdu* response) const
	{
		if(!response) throw std::runtime_error("Response is NULL");

		std::vector<ObjectData> entries;

		uint32_t index = 0;
		netsnmp_variable_list* var;
		for(var = response->variables; var; var = var->next_variable)
		{
			index++;

			ObjectData data = {};
			data.id		= ObjectID(var->name, var->name_length);
			data.type	= snmp_type2char(var->type);
			bool suc = formatVariable(var, data.data);

			if(!suc)
			{
				throw std::runtime_error("Can't format variable");
			}

			// HANDLE ERRORS
			if(response->errstat != SNMP_ERR_NOERROR && response->errindex >= index)
			{
				data.error = response->errstat;
				data.valid = false;
			}
			else
			{
				data.valid = true;
			}

			entries.emplace_back(data);
		}

		return entries;
	}

	bool Device::processTrap(netsnmp_pdu* response)
	{
		std::vector<ObjectData> objs = processResponse(response);

		for(const ObjectData& obj : objs)
		{
			if(!obj.valid) continue;
			for(const auto& [interval, task] : tasks)
			{
				if(task.objects.contains(obj.id))
				{
					task.objects.at(obj.id)->updateData(obj.id, obj.data);
				}
			}
		}
		return true;
	}


	void Device::logInfo(const std::string& msg) const
	{
		syslog(LOG_INFO, "[%s] %s", name.c_str(), msg.c_str());
	}

	void Device::logNotice(const std::string& msg) const
	{
		syslog(LOG_NOTICE, "[%s] %s", name.c_str(), msg.c_str());
	}

	void Device::logWarn(const std::string& msg) const
	{
		syslog(LOG_WARNING, "[%s] %s", name.c_str(), msg.c_str());
	}

	void Device::logErr (const std::string& msg) const
	{
		syslog(LOG_ERR, "[%s] %s", name.c_str(), msg.c_str());
	}




	UpdateTask::UpdateTask() : Task(RECURRENT)
	{

	}

	void UpdateTask::run()
	{
		device->updateObjects(objects);
		device->snmpfs->proc.snmpfsLastUpdate.setNow();
	}

}	// namespace snmpfs

