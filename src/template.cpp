
static void snmp_test()
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

		switch(vars->type)
		{
			case ASN_NULL:
				printf("NULL");
			case ASN_TIMETICKS:
				printf("TIMETICKS");
				break;
		}

	}

	// 	snmp_free_pdu(pdu);	// synch does free it for us

	snmp_sess_close(handle);
}
