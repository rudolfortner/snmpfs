#!/bin/bash
mkdir -p mnt

#valgrind \
../build/snmpfs -d -s -f mnt -c test.xml
