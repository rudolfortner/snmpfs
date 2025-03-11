#!/bin/bash
snmpwalk -v3 -l authPriv -a SHA -A authpass -x AES -X privpass -u admin 192.168.56.230
