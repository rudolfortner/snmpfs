#pragma once

#include "traphandler.h"
#include "snmp/snmp_ext.h"

#include <map>
#include <mutex>
#include <thread>
#include <vector>

namespace snmpfs {

	/**
	* @todo write docs
	*/
	class AbstractTrapReceiver
	{
	public:
		/**
		* Default constructor
		*/
		AbstractTrapReceiver(uint16_t port);

		/**
		* Destructor
		*/
		virtual ~AbstractTrapReceiver();

		uint16_t getPort() const;

	private:
		uint16_t port;
		netsnmp_session* session;

		/*
		* Callbacks needed to receive traps
		* pre_parse and post_parse need to return 1 in order for incoming messages to get forwarded to process callback
		*/
		virtual int pre_parse(netsnmp_session * session, netsnmp_transport *transport, void *transport_data, int transport_data_length) = 0;
		virtual int post_parse(netsnmp_session* session, netsnmp_pdu* pdu, int unknown) = 0;
		virtual int process(int op, netsnmp_session* session, int reqid, netsnmp_pdu* pdu, void* magic) = 0;


		// static functions to register a AbstractTrapReceiver since one static function is needed
		static void registerReceiver(netsnmp_session* session, AbstractTrapReceiver* receiver);
		static void unregisterReceiver(netsnmp_session* session);
		static int receiverPreParse(netsnmp_session * session, netsnmp_transport *transport, void *transport_data, int transport_data_length);
		static int receiverPostParse(netsnmp_session* session, netsnmp_pdu* pdu, int unknown);
		static int receiverProcess(int op, netsnmp_session* session, int reqid, netsnmp_pdu* pdu, void* magic);
		static void receiverLoop();
	};

	class TrapReceiver : public AbstractTrapReceiver
	{
	public:
		/**
		* Default constructor
		*/
		TrapReceiver(uint16_t port);

		/**
		* Destructor
		*/
		~TrapReceiver();

		void addHandler(AbstractTrapHandler* handler);
		void freeHandlers();
		void removeHandler(AbstractTrapHandler* handler);

	private:
		std::mutex handlerMutex;
		std::vector<AbstractTrapHandler*> handlers;

		int pre_parse(netsnmp_session * session, netsnmp_transport *transport, void *transport_data, int transport_data_length);
		int post_parse(netsnmp_session* session, netsnmp_pdu* pdu, int unknown);
		int process(int op, netsnmp_session* session, int reqid, netsnmp_pdu* rawPDU, void* magic);
	};

}	// namespace snmpfs
