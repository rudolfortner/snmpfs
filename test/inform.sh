#!/bin/bash

#echo "v2 inform"
#snmpinform -v 2c -c test localhost:16200 '' SNMPv2-MIB::coldStart SNMPv2-MIB::sysName.0 s "MyDevice"

echo "v3 inform"
snmpinform -v 3  -u admin -l authPriv -a SHA -A authpass -x AES -X privpass 192.168.56.1:16200 '' SNMPv2-MIB::coldStart SNMPv2-MIB::sysName.0 s "MyDevice"
