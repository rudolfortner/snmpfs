# snmpfs

snmpfs is a filesystem in userspace for monitoring networked devices with SNMP. It was created as part of a master's thesis and shows that it is possible to provide SNMP related information through a filesystem.


## Building
First installDeps.sh should be run to install all required system libraries and other dependencies. Afterwards the application can be built from source with CMake and ninja or make.

```
bash installDeps.sh
mkdir build
cd build
cmake ..
ninja
```

## Usage
Using snmpfs is simple. It requires a mountpoint to be specified and the mandatory path to a valid configuration file. The [DTD](config/snmpfs.dtd) can be used to validate the users configuration file.

```
snmpfs -c <config> <mnt>
```

Upon executing the command given above, possible errors in the configuration file are printed to the console. If no errors occur, the FUSE daemon gets started and moved to the background. All subsequent messages are logged to the system logger via syslog.