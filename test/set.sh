#!/bin/bash
snmpset -v 2c -c public 192.168.56.201 iso.3.6.1.2.1.1.4.0 s "admin"
snmpset -v 2c -c public 192.168.56.201 iso.3.6.1.2.1.1.6.0 s "virtual server"

