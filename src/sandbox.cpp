#include "sandbox.h"

#include "core/tqueue.h"
#include "core/util.h"
#include "demo.h"
#include "deviceinit.h"
#include "snmp/devicetree.h"
#include "snmp/snmp_ext.h"
#include "snmp/objectid.h"
#include <cassert>

namespace snmpfs {


	void test()
	{
		// 	init_snmp("snmpfs");
		// 	snmpv3_test();
		// 	test_oid();
		// 	testTrapHandler();
		// 	return;


		ObjectID sys("iso.3.6.1.2.1.1");
		printf("Has MIB: %s\n", sys.hasMIB() ? "YES" : "NO");
		printf("MIB: %p\n", (void*) sys.getMIB());

		ObjectID root("iso");
		printf("Has MIB: %s\n", root.hasMIB() ? "YES" : "NO");
		printf("MIB: %p\n", (void*) root.getMIB());
		for(tree* child = root.getMIB()->child_list; child; child = child->next_peer)
		{
			printf("--%s\n", child->label);
		}

		exit(0);
	}

	/*
	static void testDeviceTree()
	{
		DeviceConfig deviceConfig;
		deviceConfig.name			= "ubuntu";
		deviceConfig.peername		= "192.168.56.203";
		deviceConfig.auth.version	= VERSION_1;
		deviceConfig.auth.username	= "public";

		Device* device = new Device();
		device->name		= deviceConfig.name;
		device->config		= deviceConfig;
		device->initSNMP();

		Demo::addObject(deviceConfig, "hostname",		"iso.3.6.1.2.1.1.5.0",		SCALAR);	// STRING
		Demo::addObject(deviceConfig, "uptime",			"iso.3.6.1.2.1.25.1.1.0",	SCALAR);	// TIMETICKS
		Demo::addObject(deviceConfig, "oid-example",	"iso.3.6.1.2.1.1.2.0",		SCALAR);	// OID
		Demo::addObject(deviceConfig, "hex-string",		"iso.3.6.1.2.1.25.1.2.0",	SCALAR);	// HEX-STRING
		Demo::addObject(deviceConfig, "integer",		"iso.3.6.1.2.1.25.1.3.0",	SCALAR);	// INTEGER
		Demo::addObject(deviceConfig, "gauge",			"iso.3.6.1.2.1.25.1.5.0",	SCALAR);	// GAUGE32

		// 	Demo::addObject(deviceConfig, "interfaces",		"iso.3.6.1.2.1.2.2",		TABLE);		// TABLE
		Demo::addObject(deviceConfig, "system",			"iso.3.6.1.2.1.1",			TREE);		// SUBTREE
		//Demo::addObject(deviceConfig, "all",			"iso",						TREE);		// SUBTREE

		DeviceTree* deviceTree = DeviceTree::fromDevice(device);
		printf("%s\n", deviceTree->print().c_str());

		FileNode* deviceNode = new FileNode(deviceConfig.name);
		for(const ObjectConfig& objectConfig : deviceConfig.objects)
		{
			createNodes(deviceTree, deviceNode, objectConfig);
		}

		printf("----- FILE TREE -----\n");
		printf("%s\n", deviceNode->printFileTree().c_str());

		exit(0);
	}
	*/

	void snmp_test()
	{
		// 	netsenmp_session session;
		netsnmp_session session;
		snmp_sess_init(&session);

		session.version = 1;
		session.retries	= 3;

		std::string peername	= "192.168.2.29";
		session.peername		= (char*) peername.c_str();

		std::string community = "public";
		session.community		= (unsigned char*) community.c_str();
		session.community_len	= community.size();
		// TODO Authentication for version 3

		void* handle = snmp_sess_open(&session);
		if(handle == NULL)
		{
			printf("ERROR SESSION\n");
			snmp_sess_perror("snmpfs", &session);
			return;
		}

		netsnmp_pdu* pdu;
		pdu = snmp_pdu_create(SNMP_MSG_GET);

		std::string uptimeOID = "iso.3.6.1.2.1.25.1.1.0";
		// 	snmp_pdu_add_variable(pdu, up
		oid name[MAX_OID_LEN];
		size_t name_length = MAX_OID_LEN;	// MUST BE SET TO MAX_OID_LEN
		oid* res = snmp_parse_oid(uptimeOID.c_str(), name, &name_length);
		if(!res) printf("COULD NOT PARSE OID\n");
		if(!res) snmp_perror(uptimeOID.c_str());

		struct tree* tp;
		tp = get_tree(name, name_length, get_tree_head());
		if(tp == NULL)
		{
			printf("CANNOT FIND TREE FOR OID\n");
		}
		else
		{
			print_objid(name, name_length);
			print_description(name, name_length, 1000000);
			print_mib_tree(stdout, tp, 1000000);

			print_mib_tree(stdout, get_tree_head(), 1000000);
		}

		printf("Got OID: ");
		for(size_t i = 0; i < name_length; i++) printf(".%lu", name[i]);
		printf("\n");

		// 	snmp_pdu_add_variable(pdu, o, 9, ASN_NULL, 0, 0);
		snmp_add_null_var(pdu, name, name_length);

		netsnmp_pdu* response;
		int status = snmp_sess_synch_response(handle, pdu, &response);
		if(status != STAT_SUCCESS) printf("ERROR\n");

		for(netsnmp_variable_list* vars = response->variables; vars; vars = vars->next_variable)
		{
			print_variable(vars->name, vars->name_length, vars);
			printf("Type: %d\n", vars->type);
			printf("Data-Len: %zu\n", vars->val_len);
			printf("Data Integer:\t%ld\n", *vars->val.integer);
			printf("Data String:\t%hhu\n", *vars->val.string);
			printf("Data OBJID:\t%ld\n", *vars->val.objid);

			ASN_GAUGE;
			ASN_INTEGER;
			ASN_INTEGER64;

			switch(vars->type)
			{
				case ASN_NULL:
					printf("ASN_NULL");
					break;
				case ASN_INTEGER:
					printf("ASN_INTEGER");
					break;
				case ASN_INTEGER64:
					printf("ASN_INTEGER64");
					break;
				case ASN_TIMETICKS:
					printf("ASN_TIMETICKS");
					break;
			}
			printf("\n");

		}

		// 	snmp_free_pdu(pdu);	// synch does free it for us

		snmp_sess_close(handle);
	}

	void snmpv3_test()
	{


		char username[] = "admin";
		std::string peername	= "192.168.56.202";
		std::string authPassphrase	= "123456789";
		std::string privPassphrase	= "987654321";


		// 	netsenmp_session session;
	// 	init_snmp("snmpfs");
		netsnmp_session session0;
		snmp_sess_init(&session0);
		session0.version = SNMP_VERSION_3;
		session0.retries	= 3;

		session0.peername		= (char*) peername.c_str();

		// Version 3
		session0.securityName	= username;
		session0.securityNameLen	= strlen(username);
		// 		session.securityModel	= SNMP_SEC_MODEL_USM;

		session0.securityLevel = SNMP_SEC_LEVEL_AUTHPRIV;

		// auth
		printf("setting auth\n");
		session0.securityAuthProto		= usmHMACSHA1AuthProtocol;
		session0.securityAuthProtoLen	= sizeof(usmHMACSHA1AuthProtocol) / sizeof(oid);
		session0.securityAuthKeyLen = USM_AUTH_KU_LEN;

		int res0 = generate_Ku(session0.securityAuthProto, session0.securityAuthProtoLen,
							(const u_char*) authPassphrase.c_str(), authPassphrase.size(),
							session0.securityAuthKey, &session0.securityAuthKeyLen);
		if(res0 != SNMPERR_SUCCESS)
			std::runtime_error("ERROR");
		printf("AuthKey: %s\n", session0.securityAuthKey);

		// priv
		printf("setting priv\n");
		session0.securityPrivProto		= usmDESPrivProtocol;
		session0.securityPrivProtoLen	= sizeof(usmDESPrivProtocol) / sizeof(oid);
		session0.securityPrivKeyLen = USM_PRIV_KU_LEN;

		int res1 = generate_Ku(session0.securityAuthProto, session0.securityAuthProtoLen,
							(const u_char*) privPassphrase.c_str(), privPassphrase.size(),
							session0.securityPrivKey, &session0.securityPrivKeyLen);
		if(res1 != SNMPERR_SUCCESS)
			std::runtime_error("ERROR");
		printf("PrivKey: %s\n", session0.securityPrivKey);



		void* handle0 = snmp_sess_open(&session0);
		if(handle0 == NULL)
		{
			printf("ERROR SESSION\n");
			snmp_sess_perror("snmpfs", &session0);
			return;
		}

		netsnmp_pdu* pdu;
		pdu = snmp_pdu_create(SNMP_MSG_GET);

		std::string uptimeOID = "iso.3.6.1.2.1.25.1.1.0";
		// 	snmp_pdu_add_variable(pdu, up
		oid name[MAX_OID_LEN];
		size_t name_length = MAX_OID_LEN;	// MUST BE SET TO MAX_OID_LEN
		oid* res = snmp_parse_oid(uptimeOID.c_str(), name, &name_length);
		if(!res) printf("COULD NOT PARSE OID\n");
		if(!res) snmp_perror(uptimeOID.c_str());


		// 	snmp_pdu_add_variable(pdu, o, 9, ASN_NULL, 0, 0);
		snmp_add_null_var(pdu, name, name_length);

		netsnmp_pdu* response;
		int status = snmp_sess_synch_response(handle0, pdu, &response);
		if(status != STAT_SUCCESS) printf("status != STAT_SUCCESS\n");
		if(status != STAT_SUCCESS) return;

		for(netsnmp_variable_list* vars = response->variables; vars; vars = vars->next_variable)
		{
			print_variable(vars->name, vars->name_length, vars);
			printf("Type: %d\n", vars->type);
			printf("Data-Len: %zu\n", vars->val_len);
			printf("Data Integer:\t%ld\n", *vars->val.integer);
			printf("Data String:\t%hhu\n", *vars->val.string);
			printf("Data OBJID:\t%ld\n", *vars->val.objid);

			switch(vars->type)
			{
				case ASN_NULL:
					printf("ASN_NULL");
					break;
				case ASN_INTEGER:
					printf("ASN_INTEGER");
					break;
				case ASN_INTEGER64:
					printf("ASN_INTEGER64");
					break;
				case ASN_TIMETICKS:
					printf("ASN_TIMETICKS");
					break;
			}
			printf("\n");
		}

		// 	snmp_free_pdu(pdu);	// synch does free it for us

		snmp_sess_close(handle0);
	}

	void snmp_device_test(SandboxDevice* device)
	{
		// 	netsenmp_session session;
		netsnmp_session session;
		snmp_sess_init(&session);

		session.version = 1;
		session.retries	= 3;

		session.peername		= (char*) device->peername.c_str();
		session.community		= (unsigned char*) device->community.c_str();
		session.community_len	= device->community.size();
		// TODO Authentication for version 3

		void* handle = snmp_sess_open(&session);
		if(handle == NULL)
		{
			printf("ERROR SESSION\n");
			snmp_sess_perror("snmpfs", &session);
			return;
		}

		netsnmp_pdu* pdu;
		pdu = snmp_pdu_create(SNMP_MSG_GET);

		for(const SandboxObject& obj : device->objects)
		{
			oid name[MAX_OID_LEN];
			size_t name_length = MAX_OID_LEN;	// MUST BE SET TO MAX_OID_LEN
			oid* res = snmp_parse_oid(obj.oid.c_str(), name, &name_length);
			if(!res) snmp_perror(obj.oid.c_str());

			snmp_add_null_var(pdu, name, name_length);

		}

		netsnmp_pdu* response;
		int status = snmp_sess_synch_response(handle, pdu, &response);
		if(status != STAT_SUCCESS) printf("ERROR\n");

		for(netsnmp_variable_list* vars = response->variables; vars; vars = vars->next_variable)
		{
			print_variable(vars->name, vars->name_length, vars);
			printf("Type: %d\n", vars->type);
			printf("Data-Len: %zu\n", vars->val_len);
			printf("Data Integer:\t%ld\n", *vars->val.integer);
			printf("Data String:\t%hhu\n", *vars->val.string);
			printf("Data OBJID:\t%ld\n", *vars->val.objid);

			switch(vars->type)
			{
				case ASN_NULL:
					printf("ASN_NULL");
					break;
				case ASN_BOOLEAN:
					printf("ASN_BOOLEAN");
					break;
				case ASN_INTEGER:
					printf("ASN_INTEGER");
					break;
				case ASN_INTEGER64:
					printf("ASN_INTEGER64");
					break;
				case ASN_GAUGE:
					printf("ASN_GAUGE");
					break;
				case ASN_TIMETICKS:
					printf("ASN_TIMETICKS");
					break;

				case ASN_BIT_STR:
					printf("ASN_BIT_STR");
					break;
				case ASN_OCTET_STR:
					printf("ASN_OCTET_STR");
					{
						std::string str((char*)vars->val.string, vars->val_len);
						printf("GOT String: %s\n", str.c_str());
					}
					break;

				default:
					printf("UNKNOWN TYPE");
			}
			printf("\n");
			printf("\n");

			struct tree* tp;
			tp = get_tree(vars->name, vars->name_length, get_tree_head());
			if(tp == NULL)
			{
				printf("CANNOT FIND TREE FOR OID\n");
			}
			else
			{
				printf("Label:\t%s\n", tp->label);
				printf("Type:\t%d\n", tp->type);
				printf("Access:\t%d\n", tp->access);
				printf("Status:\t%d\n", tp->status);

				printf("Hint:\t%s\n", tp->hint);
				printf("Units:\t%s\n", tp->units);
				printf("Description:\t%s\n", tp->description);

				// 			print_objid(vars->name, vars->name_length);
				// 			print_description(vars->name, vars->name_length, 1000000);
				// 			print_mib_tree(stdout, tp, 1000000);

				// 			print_mib_tree(stdout, get_tree_head(), 1000000);
			}
			printf("\n");
			printf("--------------------");
			printf("\n");
		}

		// 	snmp_free_pdu(pdu);	// synch does free it for us

		snmp_sess_close(handle);
	}


	void snmp_test_mibs()
	{
	// 	add_mibdir("/usr/share/snmp/mibs/");
		// 	add_mibfile("UCD-SNMP-MIB", NULL);
		// 	init_mib();
		// 	add_mibfile("/usr/share/snmp/mibs-old/UCD-SNMP-MIB.txt", NULL);
	// 	read_mib("/usr/share/snmp/mibs-old/ietf/SNMPv2-MIB");
	// 	read_mib("/usr/share/snmp/mibs-old/UCD-SNMP-MIB.txt");
		init_mib();
	// 	setenv("MIBDIRS", "/usr/share/snmp/mibs/", 1);
	// 	setenv("MIBS", "ALL", 1);
	// 	setenv("MIBDIRS", "", 1);
	// 	setenv("MIBS", "", 1);
	// 	add_mibfile("/usr/share/snmp/mibs/UCD-SNMP-MIB.txt", NULL);
	// 	add_mibdir("/usr/share/snmp/mibs/");
	// 	add_mibfile("UCD-SNMP-MIB.txt", NULL);

		netsnmp_session session;
		snmp_sess_init(&session);

		std::string hostnameOID = "iso.3.6.1.2.1.1.5.0";
		hostnameOID = "sysName.0";
	// 	hostnameOID = " iso.3.6.1.2.1.1.3.0";

		oid name[MAX_OID_LEN];
		size_t name_length = MAX_OID_LEN;	// MUST BE SET TO MAX_OID_LEN
		oid* res = snmp_parse_oid(hostnameOID.c_str(), name, &name_length);
		if(!res) {
			snmp_perror(hostnameOID.c_str());
			return;
		}

		printf("Printing OBJID:\n");
		fprint_objid(stdout, name, name_length);
		printf("Printing Description:\n");
		fprint_description(stdout, name, name_length, 1000);
		printf("----------------------------\n");


		struct tree* tp;
		tp = get_tree(name, name_length, get_tree_head());
		if(tp == NULL)
		{
			printf("CANNOT FIND TREE FOR OID\n");
		}
		else
		{
			// parse.h defines what those numbers mean: e.g. MIB_ACCESS_READWRITE   19
			printf("Label:\t%s\n", tp->label);
			printf("Type:\t%d\n", tp->type);
			printf("Access:\t%d\n", tp->access);
			printf("Status:\t%d\n", tp->status);

			printf("Hint:\t%s\n", tp->hint);
			printf("Units:\t%s\n", tp->units);
			printf("Description:\t%s\n", tp->description);

			print_mib_tree(stdout, tp, 10000);
		}
		printf("\n");
	}


	void snmp_test_mibs_table()
	{
		init_mib();

		netsnmp_session session;
		snmp_sess_init(&session);

		std::string tableOID = "iso.3.6.1.2.1.1.5.0";
		tableOID = "fileTable.0";

		oid name[MAX_OID_LEN];
		size_t name_length = MAX_OID_LEN;	// MUST BE SET TO MAX_OID_LEN
		oid* res = snmp_parse_oid(tableOID.c_str(), name, &name_length);
		if(!res) {
			snmp_perror(tableOID.c_str());
			return;
		}

		printf("Printing OBJID:\n");
		fprint_objid(stdout, name, name_length);
		printf("Printing Description:\n");
		fprint_description(stdout, name, name_length, 1000);
		printf("----------------------------\n");


		struct tree* tp;
		tp = get_tree(name, name_length, get_tree_head());
		if(tp == NULL)
		{
			printf("CANNOT FIND TREE FOR OID\n");
		}
		else
		{
			// parse.h defines what those numbers mean: e.g. MIB_ACCESS_READWRITE   19
			printf("Label:\t%s\n", tp->label);
			printf("Type:\t%d\n", tp->type);
			printf("Access:\t%d\n", tp->access);
			printf("Status:\t%d\n", tp->status);

			printf("Hint:\t%s\n", tp->hint);
			printf("Units:\t%s\n", tp->units);
			printf("Description:\t%s\n", tp->description);

			print_mib_tree(stdout, tp, 10000);
		}
		printf("\n");
		printf("------------------------------\n");

		struct tree* leaveNode = tp->child_list->child_list;
		while(leaveNode)
		{
			printf("Name: %s\n", leaveNode->label);
			leaveNode = leaveNode->next_peer;
		}
	}



	void snmp_device_set(Device* device, std::string objectID, std::string type, std::string value)
	{
		// 	netsenmp_session session;
		init_mib();
		netsnmp_session session;
		snmp_sess_init(&session);

		session.version = 1;
		session.retries	= 3;

		session.peername		= (char*) device->getConfig().peername.c_str();
		session.community		= (unsigned char*) device->getConfig().auth.username.c_str();
		session.community_len	= device->getConfig().auth.username.size();
		// TODO Authentication for version 3

		void* handle = snmp_sess_open(&session);
		if(handle == NULL)
		{
			printf("ERROR SESSION\n");
			snmp_sess_perror("snmpfs", &session);
			return;
		}

		netsnmp_pdu* pdu;
		pdu = snmp_pdu_create(SNMP_MSG_SET);

		oid name[MAX_OID_LEN];
		size_t name_length = MAX_OID_LEN;	// MUST BE SET TO MAX_OID_LEN
		oid* res = snmp_parse_oid(objectID.c_str(), name, &name_length);
		if(!res) snmp_perror(objectID.c_str());

		int failed = snmp_add_var(pdu, name, name_length, type[0], value.c_str());
		if(failed)
		{
			// Returns an error like
			// sysLocation.0: Bad variable type (Type of attribute is OCTET STRING, not INTEGER)
			snmp_perror(objectID.c_str());
		}

		netsnmp_pdu* response;
		int status = snmp_sess_synch_response(handle, pdu, &response);
		if(status != STAT_SUCCESS) printf("ERROR\n");

		for(netsnmp_variable_list* vars = response->variables; vars; vars = vars->next_variable)
		{
			print_variable(vars->name, vars->name_length, vars);
			printf("Type: %d\n", vars->type);
			printf("Data-Len: %zu\n", vars->val_len);
			printf("Data Integer:\t%ld\n", *vars->val.integer);
			printf("Data String:\t%hhu\n", *vars->val.string);
			printf("Data OBJID:\t%ld\n", *vars->val.objid);

			switch(vars->type)
			{
				case ASN_NULL:
					printf("ASN_NULL");
					break;
				case ASN_BOOLEAN:
					printf("ASN_BOOLEAN");
					break;
				case ASN_INTEGER:
					printf("ASN_INTEGER");
					break;
				case ASN_INTEGER64:
					printf("ASN_INTEGER64");
					break;
				case ASN_GAUGE:
					printf("ASN_GAUGE");
					break;
				case ASN_TIMETICKS:
					printf("ASN_TIMETICKS");
					break;

				case ASN_BIT_STR:
					printf("ASN_BIT_STR");
					break;
				case ASN_OCTET_STR:
					printf("ASN_OCTET_STR");
					{
						std::string str((char*)vars->val.string, vars->val_len);
						printf("\nGOT String: %s\n", str.c_str());
					}
					break;

				default:
					printf("UNKNOWN TYPE");
			}
			printf("\n");
			printf("\n");
		}

		// 	snmp_free_pdu(pdu);	// synch does free it for us

		snmp_sess_close(handle);
	}

	void test_oid()
	{
		netsnmp_init_mib();
		std::string hostnameOID		= "iso.3.6.1.2.1.1.5.0";
		std::string ifTableOID		= "iso.3.6.1.2.1.2.2";
		std::string systemOID		= "iso.3.6.1.2.1.1";
		std::string& lookupOID	= ifTableOID;
		ObjectID objID(lookupOID);

		fprint_objid(stdout, objID, objID);
		tree* tr = get_tree(objID, objID, get_tree_head());
		printf("Searching for objectID %s\n", ((std::string) objID).c_str());
		if(tr)
		{
			printf("Childs: %s\n",	tr->child_list ? "YES" : "NO");
			printf("Label:\t%s\n",	tr->label);
			printf("SubID:\t%lu\n",	tr->subid);
			printf("Type:\t%d\n",	tr->type);
			printf("Access:\t%d\n",	tr->access);
			printf("Status:\t%d\n",	tr->status);
			printf("Descr:\t%s\n",	tr->description);
			printf("Hints:\t%s\n",	tr->hint);
			printf("Units:\t%s\n",	tr->units);

		}
		if(tr)	printf("Tree node with name %s\n", tr->label);
		if(tr) print_mib_tree(stdout, tr, 10000);


		ObjectID other(lookupOID);
		if(objID == other)
			printf("EQUAL!\n");
	}

	void test_table_mib()
	{
		// const ObjectID& oid(".1.3.6.1.3.1997.256");	// custom
		const ObjectID& oid("iso.3.6.1.2.1.2.2");		// interfaces
		tree* tr = ObjectID::toMIB(oid);

		if(tr)
			printf("Loading Table from MIB\n");
		else
		{
			printf("NO MIB FOUND\n");
			return;
		}

		tree* entry = tr->child_list;


		module* mod = find_module(tr->modid);
		printf("From MIB %s %s\n", mod->name, mod->file);
		printf("Childs: %s\n",	tr->child_list ? "YES" : "NO");
		printf("Label:\t%s\n",	tr->label);
		printf("SubID:\t%lu\n",	tr->subid);
		printf("Type:\t%d\n",	tr->type);
		printf("Access:\t%d\n",	tr->access);
		printf("Status:\t%d\n",	tr->status);
		printf("Descr:\t%s\n",	tr->description);
		printf("Hints:\t%s\n",	tr->hint);
		printf("Units:\t%s\n",	tr->units);


		printf("Table OID:  %s\n", ((std::string) oid).c_str());
		if(entry)
		{
			printf("Entry OID:  %s\n", ((std::string) oid.getSubOID(entry->subid)).c_str());
			printf("Label:\t%s\n",	entry->label);

			for(tree* col = entry->child_list; col; col = col->next_peer)
			{
				// table->addColumn(col->label, oid.getSubOID(entry->subid).getSubOID(col->subid));
			}
		}
	}

	void test_mib_loading()
	{
		// TODO Try to load MIBs per session
	}




	static int
	pre_parse(netsnmp_session * session, netsnmp_transport *transport,
			void *transport_data, int transport_data_length)
	{
		printf("pre_parse\n");
		std::string data((char*) transport_data, transport_data_length);
		printf("%s\n", data.c_str());
		return 1;	// Accept
		return 0;	// Discard
	}

	int post_parse(netsnmp_session* session, netsnmp_pdu* pdu, int what)
	{
		printf("post_parse\n");
		printf("what? %d\n", what);

		return 1;
	}

	int
	snmp_input(int op, netsnmp_session *session, int reqid, netsnmp_pdu *pdu, void *magic)
	{
		printf("snmp_input\n");
		printf("\top:\t%d\n", op);
		printf("\treqid:\t%d\n", reqid);

		netsnmp_variable_list* var;
		for(var = pdu->variables; var; var = var->next_variable)
		{
			ObjectID oid(var->name, var->name_length);

			size_t bufferSize = 1024;
			char* buffer = (char*) malloc(bufferSize);
			size_t outLen = 0;
			sprint_realloc_by_type((u_char **) &buffer, &bufferSize, &outLen, 0, var, NULL, NULL, NULL);

			printf("\tReceived %s -> %s\n", ((std::string) oid).c_str(), buffer);
		}

		return 1;
		return 0;
	}

	static netsnmp_session *
	snmptrapd_add_session(netsnmp_transport *t)
	{
		netsnmp_session sess, *session = &sess, *rc = NULL;

		snmp_sess_init(session);
		session->peername = SNMP_DEFAULT_PEERNAME;  /* Original code had NULL here */
		session->version = SNMP_DEFAULT_VERSION;
		session->community_len = SNMP_DEFAULT_COMMUNITY_LEN;
		session->retries = SNMP_DEFAULT_RETRIES;
		session->timeout = SNMP_DEFAULT_TIMEOUT;
		session->callback = snmp_input;
		session->callback_magic = (void *) t;
		session->authenticator = NULL;
		sess.isAuthoritative = SNMP_SESS_UNKNOWNAUTH;


	// 	std::string c = "public";
	// 	session->community = (u_char*) c.c_str();
	// 	session->community_len = c.size();
	// 	session->version = VERSION_2c;

		rc = snmp_add(session, t, pre_parse, NULL);
	// 	rc = snmp_add(session, t, NULL, NULL);
		if (rc == NULL) {
			snmp_sess_perror("snmptrapd", session);
		}
		printf("Session created\n");
		return rc;
	}

	#include <net-snmp/library/fd_event_manager.h>
	#include <err.h>
	void mainLoop()
	{
		int             count, numfds, block;
		fd_set          readfds,writefds,exceptfds;
		struct timeval  timeout;
		NETSNMP_SELECT_TIMEVAL timeout2;

		while (true) {

			numfds = 0;
			FD_ZERO(&readfds);
			FD_ZERO(&writefds);
			FD_ZERO(&exceptfds);
			block = 0;
			timerclear(&timeout);
			timeout.tv_sec = 5;
			snmp_select_info(&numfds, &readfds, &timeout, &block);
			#ifndef NETSNMP_FEATURE_REMOVE_FD_EVENT_MANAGER
			netsnmp_external_event_info(&numfds, &readfds, &writefds, &exceptfds);
			#endif /* NETSNMP_FEATURE_REMOVE_FD_EVENT_MANAGER */
			timeout2.tv_sec = timeout.tv_sec;
			timeout2.tv_usec = timeout.tv_usec;
			count = select(numfds, &readfds, &writefds, &exceptfds,
						!block ? &timeout2 : NULL);
			if (count > 0) {
				#ifndef NETSNMP_FEATURE_REMOVE_FD_EVENT_MANAGER
				netsnmp_dispatch_external_events(&count, &readfds, &writefds,
												&exceptfds);
				#endif /* NETSNMP_FEATURE_REMOVE_FD_EVENT_MANAGER */
				/* If there are any more events after external events, then
				* try SNMP events. */
				if (count > 0) {
					snmp_read(&readfds);
				}
			} else {
				switch (count) {
					case 0:
						snmp_timeout();
						break;
					case -1:
						if (errno == EINTR)
							continue;
					snmp_log_perror("select");
					return;
					break;
					default:
						fprintf(stderr, "select returned %d\n", count);
						return;
				}
			}
			run_alarms();
		}
	}

	void testTrapHandler()
	{
		init_snmp("snmpfs");

		std::string cp = "udp:16200";
		printf("Opening server...\n");
		netsnmp_transport* transport = netsnmp_transport_open_server("snmptrap", cp.c_str());

		if (transport == NULL)
		{
			snmp_log(LOG_ERR, "couldn't open %s -- errno %d (\"%s\")\n", cp.c_str(), errno, strerror(errno));
			return;
		}
		printf("Opening server successful...\n");


		netsnmp_session sess;
		netsnmp_session* session = &sess;
		snmp_sess_init(session);
	// 	session->peername = SNMP_DEFAULT_PEERNAME;  /* Original code had NULL here */
	// 	session->version = SNMP_DEFAULT_VERSION;
	// 	session->community_len = SNMP_DEFAULT_COMMUNITY_LEN;
	// 	session->retries = SNMP_DEFAULT_RETRIES;
	// 	session->timeout = SNMP_DEFAULT_TIMEOUT;
		session->callback = snmp_input;
		session->callback_magic = (void *) transport;
	// 	session->authenticator = NULL;

		netsnmp_session* rc = snmp_add(session, transport, pre_parse, post_parse);
		// 	rc = snmp_add(session, t, NULL, NULL);
		if (rc == NULL) {
			snmp_sess_perror("snmptrapd", session);
		}
		printf("Session created\n");

		mainLoop();

		while(true)
		{
			snmp_timeout();
		}

		netsnmp_session* ss = snmptrapd_add_session(transport);
		if(!ss)
		{
			snmp_log(LOG_ERR, "couldn't open snmp - %s", strerror(errno));
			return;
		}

		mainLoop();

		while(true)
		{
			snmp_timeout();
			run_alarms();
		}

	}


	void test_snmp_get()
	{
		printf("Sandbox SNMP GET\n");
		init_snmp("test");

		const std::string peername = "192.168.56.222";
		const std::string username = "public";

		const ObjectID oid("iso.3.6.1.2.1.1.555.0");

		netsnmp_session session;
		snmp_sess_init(&session);

		session.peername	= (char*) peername.c_str();
		session.retries	= 3;
		session.version = SNMP_VERSION_2c;
		session.community		= (unsigned char*) username.c_str();
		session.community_len	= username.size();

		void* handle = snmp_sess_open(&session);
		if(handle == NULL)
		{
			printf("ERROR SESSION\n");
			snmp_sess_perror("snmpfs", &session);
			return;
		}



		netsnmp_pdu* pdu;
		netsnmp_pdu* response;

		pdu = snmp_pdu_create(SNMP_MSG_GET);
		// snmp_add_var(pdu, oid, oid, '=', data.c_str());
		snmp_add_null_var(pdu, oid, oid);

		int status = snmp_sess_synch_response(handle, pdu, &response);
		if(status != STAT_SUCCESS) printf("NO SUCCESS\n");


		if(!response) throw std::runtime_error("No response");
		SNMP_ERR_NOERROR;
		printf("Err status %ld\n", response->errstat);
		printf("Err index %ld\n", response->errindex);

		netsnmp_variable_list* var;
		for(var = response->variables; var; var = var->next_variable)
		{
			ObjectID oid(var->name, var->name_length);
			char buffer[1024];
			snprint_variable(buffer, 1024, oid, oid, var);
			printf("Received %s (%c) -> %s\n", ((std::string) oid).c_str(), snmp_type2char(var->type), buffer);

		}


		snmp_sess_close(handle);
		handle = NULL;
	}


	void test_snmp_set()
	{
		printf("Sandbox SNMP Set\n");
		const std::string peername = "192.168.56.204";
		const std::string username = "public";

		const ObjectID oid("iso.3.6.1.3.1997.1.0");
		const std::string data = "YUHU\n";

		tree* mib = get_tree(oid, oid, get_tree_head());
		printf("Access: %d\n", mib->access);
		printf("Module: %s\n", find_module(mib->modid) ? "TRUE" : "FALSE");

		netsnmp_ds_set_int(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_STRING_OUTPUT_FORMAT, NETSNMP_STRING_OUTPUT_ASCII);

		netsnmp_session session;
		snmp_sess_init(&session);

		session.peername	= (char*) peername.c_str();
		session.retries	= 3;
		session.version = SNMP_VERSION_2c;
		session.community		= (unsigned char*) username.c_str();
		session.community_len	= username.size();

		void* handle = snmp_sess_open(&session);
		if(handle == NULL)
		{
			printf("ERROR SESSION\n");
			snmp_sess_perror("snmpfs", &session);
			return;
		}



		netsnmp_pdu* pdu;
		netsnmp_pdu* response;

		pdu = snmp_pdu_create(SNMP_MSG_SET);
		// snmp_add_var(pdu, oid, oid, '=', data.c_str());
		snmp_add_var(pdu, oid, oid, 's', data.c_str());

		int status = snmp_sess_synch_response(handle, pdu, &response);
		if(status != STAT_SUCCESS) printf("ERROR\n");


		if(!response) throw std::runtime_error("No response");
		SNMP_ERR_NOERROR;
		printf("Err status %ld\n", response->errstat);
		printf("Err index %ld\n", response->errindex);

		netsnmp_variable_list* var;
		for(var = response->variables; var; var = var->next_variable)
		{
			ObjectID oid(var->name, var->name_length);
			char buffer[1024];
			snprint_variable(buffer, 1024, oid, oid, var);
			printf("Received %s (%c) -> %s\n", ((std::string) oid).c_str(), snmp_type2char(var->type), buffer);

		}


		snmp_sess_close(handle);
		handle = NULL;
	}


	bool formatVariable(netsnmp_variable_list* var, std::string& data)
	{

		// TODO Move buffer outside of loop to minimize heap allocations
		size_t bufferSize = 1024;
		char* buffer = (char*) malloc(bufferSize);
		size_t outLen = 0;

		netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_QUICK_PRINT, true);
		netsnmp_ds_set_int(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_OID_OUTPUT_FORMAT, NETSNMP_OID_OUTPUT_NUMERIC);

		// TODO Could additional infos be used? subtree->enums, subtree->hint, units
		bool suc = sprint_realloc_by_type((u_char **) &buffer, &bufferSize, &outLen, 0, var, NULL, NULL, NULL);
		if(!suc) return suc;

	#if DEBUG_FORMAT
		ObjectID id(var->name, var->name_length);
		printf("VALUE for %s\n", ((std::string) id).c_str());
		printf("VALUE printed %s\n", suc ? "TRUE" : "FALSE");
		printf("VALUE raw is %s\n", buffer);
		printf("VALUE len is %zu\n", outLen);
		printf("VALUE str is %s\n", std::string(buffer, outLen).c_str());
	#endif
		data.assign(buffer, outLen);
		free(buffer);

		// Strings are shipped in quotes
		if(data.starts_with('\"') && data.ends_with('\"'))
			data = data.substr(1, data.length() - 2);

		// Quotes in strings are escaped...
		data = replace(data, "\\\"", "\"");

		return true;
	}

	void test_snmp_format()
	{
		printf("Sandbox SNMP Format\n");
		const std::string peername = "192.168.56.201";
		const std::string username = "public";

		std::vector<ObjectID> oids;
	// 	oids.push_back(ObjectID("iso.3.6.1.2.1.25.1.1.0"));	// uptime
		oids.push_back(ObjectID("iso.3.6.1.2.1.1.5.0"));	// hostname
		oids.push_back(ObjectID("iso.3.6.1.2.1.1.4.0"));		// contact



		netsnmp_session session;
		snmp_sess_init(&session);

		session.peername	= (char*) peername.c_str();
		session.retries	= 3;
		session.version = SNMP_VERSION_2c;
		session.community		= (unsigned char*) username.c_str();
		session.community_len	= username.size();

		void* handle = snmp_sess_open(&session);
		if(handle == NULL)
		{
			printf("ERROR SESSION\n");
			snmp_sess_perror("snmpfs", &session);
			return;
		}



		netsnmp_pdu* pdu;
		netsnmp_pdu* response;

		pdu = snmp_pdu_create(SNMP_MSG_GET);
		for(const ObjectID& oid : oids)
			snmp_add_null_var(pdu, oid, oid);

		int status = snmp_sess_synch_response(handle, pdu, &response);
		if(status != STAT_SUCCESS) printf("ERROR\n");


		if(!response) throw std::runtime_error("No response");
		printf("Err status %ld\n", response->errstat);
		printf("Err index %ld\n", response->errindex);
		SNMP_ERR_NOERROR;

		netsnmp_variable_list* var;
		for(var = response->variables; var; var = var->next_variable)
		{
			ObjectID oid(var->name, var->name_length);

			std::string string;
			bool suc = formatVariable(var, string);
			if(!suc) throw std::runtime_error("Format Error");

			printf("Received %s -> %s\n", ((std::string) oid).c_str(), string.c_str());
		}


		snmp_sess_close(handle);
		handle = NULL;
	}


	void test_snmp_errstat()
	{
		printf("Sandbox SNMP Errstat\n");
		const std::string peername = "192.168.56.201";
		const std::string username = "public";

		std::vector<ObjectID> oids;
		// 	oids.push_back(ObjectID("iso.3.6.1.2.1.25.1.1.0"));			// uptime
		oids.push_back(ObjectID("iso.3.6.1.2.1.1.5.0"));				// hostname
		oids.push_back(ObjectID("iso.3.6.1.2.1.1.4.0"));				// contact
		oids.push_back(ObjectID("iso.3.6.1.4.1.8072.1.3.2.1.0"));		// invalid object probe
		oids.push_back(ObjectID("iso.3.6.1.4.1.8072.1.2.1.1.6.0.1.2.0"));// invalid object probe
		oids.push_back(ObjectID("iso.3.6.1.4.1.8072.1.5.1.0"));			// invalid object probe



		netsnmp_session session;
		snmp_sess_init(&session);

		session.peername	= (char*) peername.c_str();
		session.retries	= 3;
		session.version = SNMP_VERSION_2c;
		session.community		= (unsigned char*) username.c_str();
		session.community_len	= username.size();

		void* handle = snmp_sess_open(&session);
		if(handle == NULL)
		{
			printf("ERROR SESSION\n");
			snmp_sess_perror("snmpfs", &session);
			return;
		}



		netsnmp_pdu* pdu;
		netsnmp_pdu* response;

		pdu = snmp_pdu_create(SNMP_MSG_GET);
		for(const ObjectID& oid : oids)
			snmp_add_null_var(pdu, oid, oid);

		int status = snmp_sess_synch_response(handle, pdu, &response);
		if(status != STAT_SUCCESS) printf("ERROR\n");


		if(!response) throw std::runtime_error("No response");
		printf("Err status %ld\n", response->errstat);
		printf("Err index %ld\n", response->errindex);
		SNMP_ERR_NOERROR;

		netsnmp_variable_list* var;
		for(var = response->variables; var; var = var->next_variable)
		{
			ObjectID oid(var->name, var->name_length);

			std::string string;
			bool suc = formatVariable(var, string);
			if(!suc) throw std::runtime_error("Format Error");

			printf("Received %s -> %s\n", ((std::string) oid).c_str(), string.c_str());
		}


		snmp_sess_close(handle);
		handle = NULL;
	}

	bool test_util_trim_helper(const std::string& str, const std::string& correct)
	{
		std::string trimmed = trim(str);
		bool equal = trimmed == correct;
		printf("'%s' -> '%s' == '%s'    (%s)\n", str.c_str(), trimmed.c_str(), correct.c_str(), equal ? "CORRECT" : "WRONG");
		return equal;
	}

	// TODO MOVE TO UNIT TEST
	void test_util_trim()
	{
		test_util_trim_helper("    TEST        ",	"TEST");
		test_util_trim_helper("	  	TEST        ",	"TEST");
		test_util_trim_helper("\t\nTEST\n   \t",	"TEST");

		test_util_trim_helper("    \"This is a test!\"\n        ",	"\"This is a test!\"");

	}


	void* createSession()
	{
		netsnmp_session session;
		snmp_sess_init(&session);

		session.version = 1;
		session.retries	= 3;

		std::string peername	= "192.168.56.201";
		session.peername		= (char*) peername.c_str();

		std::string community = "public";
		session.community		= (unsigned char*) community.c_str();
		session.community_len	= community.size();

		void* handle = snmp_sess_open(&session);
		assert(handle);
		return handle;
	}

	struct ConcurrencyData {
		std::string name;
		void* handle;
		ObjectID id;
		bool running;
	};

	void requestThread(ConcurrencyData* cdata)
	{
		while(cdata->running)
		{
			netsnmp_pdu* pdu = snmp_pdu_create(SNMP_MSG_GET);
			snmp_add_null_var(pdu, (*cdata).id, (*cdata).id);

			netsnmp_pdu* response;
			int status = snmp_sess_synch_response(cdata->handle, pdu, &response);
			assert(status == STAT_SUCCESS);
			assert(response->errstat == SNMP_ERR_NOERROR);

			netsnmp_variable_list* var;
			for(var = response->variables; var; var = var->next_variable)
			{
				ObjectData data = {};
				data.id		= ObjectID(var->name, var->name_length);
				data.type	= snmp_type2char(var->type);
				bool suc = formatVariable(var, data.data);
				assert(suc);
				printf("%s:\t%s\n", cdata->name.c_str(), data.data.c_str());
			}

			snmp_free_pdu(response);
		}
	}

	void test_concurrency()
	{
		init_snmp("snmpfs");
		ConcurrencyData data0;
		data0.name		= "thread0";
		data0.handle	= createSession();
		data0.id		= ObjectID("iso.3.6.1.2.1.25.1.1.0");
		data0.running	= true;

		ConcurrencyData data1;
		data1.name		= "thread1";
		data1.handle	= createSession();
		data1.id		= ObjectID("iso.3.6.1.2.1.25.1.1.0");
		data1.running	= true;
		// data1.handle	= data0.handle;

		std::thread thread0(requestThread, &data0);
		std::thread thread1(requestThread, &data1);


		std::this_thread::sleep_for(std::chrono::milliseconds(10000));
		data0.running = false;
		data1.running = false;
		thread0.join();
		thread1.join();

		snmp_sess_close(data0.handle);
		printf("DONE\n");
	}



	void test_writing()
	{
		init_snmp("test");
		read_all_mibs();

		netsnmp_session session;
		snmp_sess_init(&session);

		session.version = SNMP_VERSION_2c;
		session.retries	= 3;

		std::string peername	= "192.168.56.222";
		session.peername		= (char*) peername.c_str();

		std::string community	= "private";
		session.community		= (unsigned char*) community.c_str();
		session.community_len	= community.size();

		void* handle = snmp_sess_open(&session);
		assert(handle);

		ObjectID oid(".1.3.6.1.3.1997.1.0");
		netsnmp_pdu* pdu;
		netsnmp_pdu* response;

		// CREATE PDU
		pdu = snmp_pdu_create(SNMP_MSG_SET);
		int suc_add = snmp_add_var(pdu, oid, oid, 'i', "2\n");
		/*
		* SNMPERR_RANGE - type, value, or length not found or out of range
		* SNMPERR_VALUE - value is not correct
		* SNMPERR_VAR_TYPE - type is not correct
		* SNMPERR_BAD_NAME - name is not found
		* */
		printf("suc_add: %d\n", suc_add);
		// SNMPERR_BAD_NAME

		// SEND PDU
		int status = snmp_sess_synch_response(handle, pdu, &response);
		assert(status == STAT_SUCCESS);
		assert(response);
		printf("status: %d\n", status);
		printf("respon: %ld\n", response->errstat);

		// PROCESS RESPONSE
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
				printf("ERR\n");
			}
			else
			{
				data.valid = true;
			}

			printf("Received SET result: %s\n", data.data.c_str());
		}

		snmp_free_pdu(response);
	}

	void test_tqueue()
	{
		printf("HALLO\n");
		SandboxObject* obj0 = new SandboxObject;
		SandboxObject* obj1 = new SandboxObject;
		printf("obj0: %p\n", (void*) obj0);
		printf("obj1: %p\n", (void*) obj1);

		tqueue<SandboxObject> queue;
		queue.pushIn(obj0, 10000);
		queue.push(obj1);
		queue.print();

		SandboxObject* n0 = queue.next();
		printf("next: %p\n", (void*) n0);

		SandboxObject* n1;
		uint64_t delay;
		queue.next(&n1, &delay);
	}


}	// namespace snmpfs


