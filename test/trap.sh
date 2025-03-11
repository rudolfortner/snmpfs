#!/bin/bash

echo "v2 trap"
snmptrap -v 2c -c test localhost:16200 '' SNMPv2-MIB::coldStart SNMPv2-MIB::sysName.0 s "MyDevice"

echo "v3 trap"
snmptrap -v 3  -u admin -l authPriv -a SHA -A authpass -x AES -X privpass 192.168.56.1:16200 '' SNMPv2-MIB::coldStart SNMPv2-MIB::sysName.0 s "MyDevice"
