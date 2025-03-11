#include "snmp/trapreceiver.h"

#include "snmp/objectid.h"
#include "snmp/trap.h"

#include <algorithm>
#include <net-snmp/agent/agent_trap.h>
#include <net-snmp/library/fd_event_manager.h>
#include <sstream>

namespace snmpfs {

	AbstractTrapReceiver::AbstractTrapReceiver(uint16_t port)
	{
		this->port = port;

		// Open Server
		std::stringstream conString;
		conString << "udp:" << port;

		netsnmp_transport* transport = netsnmp_transport_open_server("snmptrap", conString.str().c_str());
		if (transport == NULL)
		{
			syslog(LOG_ERR, "couldn't open %s -- errno %d (\"%s\")\n", conString.str().c_str(), errno, strerror(errno));
			return;
		}


		// Create Session
		netsnmp_session sess;
		snmp_sess_init(&sess);
		sess.callback = AbstractTrapReceiver::receiverProcess;
		sess.callback_magic = (void *) transport;
		sess.isAuthoritative = SNMP_SESS_NONAUTHORITATIVE;

		sess.peername		= SNMP_DEFAULT_PEERNAME;  /* Original code had NULL here */
		sess.version		= SNMP_DEFAULT_VERSION;
		sess.community_len	= SNMP_DEFAULT_COMMUNITY_LEN;
		sess.retries		= SNMP_DEFAULT_RETRIES;
		sess.timeout		= SNMP_DEFAULT_TIMEOUT;
		sess.callback		= AbstractTrapReceiver::receiverProcess;
		sess.callback_magic	= (void *) transport;
		sess.authenticator	= NULL;
	// 	sess.authenticator	= auth_func;
		sess.isAuthoritative= SNMP_SESS_NONAUTHORITATIVE;

		session = snmp_add(&sess, transport, AbstractTrapReceiver::receiverPreParse, AbstractTrapReceiver::receiverPostParse);
		if(session == NULL) {
			snmp_syslog_err(&sess);
			return;
		}

		AbstractTrapReceiver::registerReceiver(session, this);
	}

	AbstractTrapReceiver::~AbstractTrapReceiver()
	{
		AbstractTrapReceiver::unregisterReceiver(session);
		snmp_close(session);
	}

	uint16_t AbstractTrapReceiver::getPort() const
	{
		return port;
	}



	static bool receiverRunning = false;
	static std::mutex receiverMutex;
	static std::thread receiverThread;
	static std::map<netsnmp_session*, AbstractTrapReceiver*> receiverMap;

	void AbstractTrapReceiver::registerReceiver(netsnmp_session* session, AbstractTrapReceiver* receiver)
	{
		std::unique_lock<std::mutex> lock(receiverMutex);
		receiverMap[session] = receiver;

		// Start receiverThread if not already running
		if(!receiverRunning)
		{
			receiverRunning = true;
			receiverThread = std::thread(&AbstractTrapReceiver::receiverLoop);
		}
	}

	void AbstractTrapReceiver::unregisterReceiver(netsnmp_session* session)
	{
		std::unique_lock<std::mutex> lock(receiverMutex);
		receiverMap.erase(session);

		// Stop receiverThread if nomore receivers are in the receiverMap
		if(receiverRunning && receiverMap.size() == 0)
		{
			receiverRunning = false;
			receiverThread.join();
		}
	}

	int AbstractTrapReceiver::receiverPreParse(netsnmp_session* session, netsnmp_transport* transport, void* transport_data, int transport_data_length)
	{
		std::unique_lock<std::mutex> lock(receiverMutex);
		if(!receiverMap.contains(session)) return 0;
		AbstractTrapReceiver* receiver = receiverMap[session];

		if(!session)
			throw std::runtime_error("No trap receiver for given session");

		return receiver->pre_parse(session, transport, transport_data, transport_data_length);
	}

	int AbstractTrapReceiver::receiverPostParse(netsnmp_session* session, netsnmp_pdu* pdu, int unknown)
	{
		std::unique_lock<std::mutex> lock(receiverMutex);
		if(!receiverMap.contains(session)) return 0;
		AbstractTrapReceiver* receiver = receiverMap[session];

		if(!session)
			throw std::runtime_error("No trap receiver for given session");

		return receiver->post_parse(session, pdu, unknown);
	}

	int AbstractTrapReceiver::receiverProcess(int op, netsnmp_session* session, int reqid, netsnmp_pdu* pdu, void* magic)
	{
		std::unique_lock<std::mutex> lock(receiverMutex);
		if(!receiverMap.contains(session)) return 0;
		AbstractTrapReceiver* receiver = receiverMap[session];

		if(!session)
			throw std::runtime_error("No trap receiver for given session");

		return receiver->process(op, session, reqid, pdu, magic);
	}

	/**
	 * Code literally taken 1:1 from snmptrapd.c
	 */
	void AbstractTrapReceiver::receiverLoop()
	{
		int             count, numfds, block;
		fd_set          readfds,writefds,exceptfds;
		struct timeval  timeout;

		while (receiverRunning) {
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

			block = 0;
			count = select(numfds, &readfds, &writefds, &exceptfds, !block ? &timeout : NULL);

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
						break;
					default:
						fprintf(stderr, "select returned %d\n", count);
						break;
				}
			}
			run_alarms();
		}
	}




	TrapReceiver::TrapReceiver(uint16_t port) : AbstractTrapReceiver(port)
	{

	}

	TrapReceiver::~TrapReceiver()
	{

	}

	void TrapReceiver::addHandler(AbstractTrapHandler* handler)
	{
		std::unique_lock<std::mutex> lock(handlerMutex);
		handlers.push_back(handler);
	}

	void TrapReceiver::freeHandlers()
	{
		std::unique_lock<std::mutex> lock(handlerMutex);

		for(AbstractTrapHandler* handler : handlers)
			delete handler;

		handlers.clear();
	}

	void TrapReceiver::removeHandler(AbstractTrapHandler* handler)
	{
		std::unique_lock<std::mutex> lock(handlerMutex);
		handlers.erase(std::remove(handlers.begin(), handlers.end(), handler), handlers.end());
	}



	static std::string addr2ip(char* addr)
	{
		std::string address = "0.0.0.0";

		if(addr != NULL)
		{
			/* Catch udp,udp6,tcp,tcp6 transports using "[" */
			char *tcpudpaddr = strstr(addr, "[");
			if(tcpudpaddr != 0)
			{
				char sbuf[64];
				char *xp;

				strlcpy(sbuf, tcpudpaddr + 1, sizeof(sbuf));
				xp = strstr(sbuf, "]");
				if(xp) *xp = '\0';

				address = sbuf;
			}
		}
		return address;
	}

	int TrapReceiver::pre_parse(netsnmp_session* session, netsnmp_transport* transport, void* transport_data, int transport_data_length)
	{
		// printf("pre_parse\n");
		// printf(" -> Port: %d\n", getPort());
		// printf(" -> Transport: %p\n", (void *) transport);
		return 1;
	}

	int TrapReceiver::post_parse(netsnmp_session* session, netsnmp_pdu* pdu, int unknown)
	{
		// printf("post_parse\n");
		// printf(" -> Port: %d\n", getPort());
		// printf(" -> Version: %ld\n", pdu->version);
		// printf(" -> Command: %d\n", pdu->command);
		return 1;
	}

	int TrapReceiver::process(int op, netsnmp_session* session, int reqid, netsnmp_pdu* rawPDU, void* magic)
	{
		if(op == NETSNMP_CALLBACK_OP_TIMED_OUT)
		{
			snmp_log(LOG_ERR, "Timeout: This shouldn't happen!\n");
			return 0;
		}
		else if(op == NETSNMP_CALLBACK_OP_SEND_FAILED)
		{
			snmp_log(LOG_ERR, "Send Failed: This shouldn't happen either!\n");
			return 0;
		}
		else if(op == NETSNMP_CALLBACK_OP_CONNECT)
		{
			return 0;
		}
		else if(op == NETSNMP_CALLBACK_OP_DISCONNECT)
		{
			return 0;
		}
		else if(op == NETSNMP_CALLBACK_OP_RECEIVED_MESSAGE)
		{
			// CONTINUE WITH CODE BELOW (PROCESS MESSAGE)
		}
		else
		{
			syslog(LOG_ERR, "Unknown operation (%d): This shouldn't happen!\n", op);
			return 0;
		}

		// Prepare PDU
		netsnmp_pdu* pdu;
		if(rawPDU->version == SNMP_VERSION_1)	pdu = convert_v1pdu_to_v2(rawPDU);
		else									pdu = rawPDU;

		// printf("process:\n");
		// printf(" -> Session Community:\t%s\n", session->community);
		// printf(" -> Session Peername:\t%s\n", session->peername);
		// printf(" -> Session Localname:\t%s\n", session->localname);
		// printf(" -> Port:\t%d\n", getPort());
		// printf(" -> Op:\t%d\n", op);
		// printf(" -> agent_addr:\t%d %d %d %d\n", pdu->agent_addr[0], pdu->agent_addr[1], pdu->agent_addr[2], pdu->agent_addr[3]);
		// printf(" -> comunity:\t%s\n", pdu->community);
		// printf(" -> contextName:\t%s\n", pdu->contextName);
		// printf(" -> peer:\t%s\n", session->peername);
		// printf(" -> magic: %p\n", magic);
  //
		// printf(" -> secName: %s\n", session->securityName);
		// printf(" -> COMMAND: %d\n", rawPDU->command);
		// printf(" -> traptype: %ld\n", pdu->trap_type);

		netsnmp_transport* transport = (netsnmp_transport *) magic;
		char* addr_string = transport->f_fmtaddr(transport, pdu->transport_data, pdu->transport_data_length);

		TrapData trap;
		trap.address = addr2ip(addr_string);
		trap.pdu = pdu;
		free(addr_string);


		for(AbstractTrapHandler* handler : handlers)
		{
			bool accept = handler->handle(trap);
			if(accept)
			{
				// printf("Trap accepted by %s\n", handler->getName().c_str());
			}
			else
			{
				// printf("Trap rejected by %s\n", handler->getName().c_str());
				break;
			}
		}

		// printf(" -> COMMAND: %d\n", pdu->command);
		// printf(" -> COMMAND");
		switch(pdu->command)
		{
			case SNMP_MSG_TRAP:
				// printf("-> TRAP\n");
				break;

			case SNMP_MSG_TRAP2:
				// printf(" -> TRAP2\n");
				break;

			case SNMP_MSG_INFORM:
				// printf("-> INFORM\n");
				break;

			default:
				// printf(" -> DEFAULT\n");
				throw std::runtime_error("invalid operation");
		}

		// TODO Send INFORM
		// ACKNOWLEDGE THE INFORM
		// printf(" -> ACK\n");
		if(pdu->command == SNMP_MSG_INFORM)
		{
			netsnmp_pdu* reply = snmp_clone_pdu(pdu);

			if(reply)
			{
				// printf(" -> Sending Reply\n");
				reply->command	= SNMP_MSG_RESPONSE;
				reply->errstat	= 0;
				reply->errindex	= 0;

				if(!snmp_send(session, reply))
				{
					// printf(" -> ERROR Reply\n");
					snmp_sess_perror("snmpfs: Couldn't respond to inform pdu", session);
					snmp_free_pdu(reply);
				}
			}
			else
			{
				// printf(" -> ERROR PDU CLONE\n");
				syslog(LOG_ERR, "Couldn't clone PDU for INFORM response\n");
			}
		}

		if(pdu != rawPDU)
			snmp_free_pdu(pdu);

		return 1;
	}

}	// namespace snmpfs
