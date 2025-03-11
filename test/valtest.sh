#!/bin/bash
valgrind --leak-check=full --log-file="valgrind.txt" ../build/snmpfs -d -s -f mnt/ -c test.xml | tee out.txt
