cmake_minimum_required(VERSION 3.20)

project(snmpfs)

# BUILD FLAGS
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_FILE_OFFSET_BITS=64")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive") # Maybe remove later, used to make filler function possible
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall --pedantic -g")

# MAIN EXECUTABLE
add_executable(snmpfs src/snmpfs.cpp)
target_include_directories(snmpfs PUBLIC include/snmpfs)

# CORE SOURCES
target_sources(snmpfs PRIVATE src/core/csv.cpp)
target_sources(snmpfs PRIVATE src/core/taskmanager.cpp)
target_sources(snmpfs PRIVATE src/core/util.cpp)

# FUSE RELATED SOURCES
target_sources(snmpfs PRIVATE src/fuse/filenode.cpp)
target_sources(snmpfs PRIVATE src/fuse/objectnode.cpp)
target_sources(snmpfs PRIVATE src/fuse/procfile.cpp)
target_sources(snmpfs PRIVATE src/fuse/virtualfile.cpp)
target_sources(snmpfs PRIVATE src/fuse/virtuallogger.cpp)

# SNMP RELATED SOURCES
target_sources(snmpfs PRIVATE src/snmp/device.cpp)
target_sources(snmpfs PRIVATE src/snmp/devicetree.cpp)
target_sources(snmpfs PRIVATE src/snmp/object.cpp)
target_sources(snmpfs PRIVATE src/snmp/objectid.cpp)
target_sources(snmpfs PRIVATE src/snmp/table.cpp)
target_sources(snmpfs PRIVATE src/snmp/snmp_ext.cpp)
target_sources(snmpfs PRIVATE src/snmp/trap.cpp)
target_sources(snmpfs PRIVATE src/snmp/traphandler.cpp)
target_sources(snmpfs PRIVATE src/snmp/trapreceiver.cpp)

# OTHER SOURCES
target_sources(snmpfs PRIVATE src/configio.cpp)
target_sources(snmpfs PRIVATE src/demo.cpp)
target_sources(snmpfs PRIVATE src/deviceinit.cpp)
target_sources(snmpfs PRIVATE src/proc.cpp)
target_sources(snmpfs PRIVATE src/sandbox.cpp)

# FUSE
find_package(FUSE REQUIRED)
include_directories(${FUSE_INCLUDE_DIR})
target_link_libraries(snmpfs ${FUSE_LIBRARIES})

# SNMP
find_library(NETSNMP_AGENT		NAMES "netsnmpagent")
find_library(NETSNMP_LIBRARY	NAMES "netsnmp")
find_library(NETSNMP_MIBS		NAMES "netsnmpmibs")

target_link_libraries(snmpfs ${NETSNMP_AGENT})
target_link_libraries(snmpfs ${NETSNMP_LIBRARY})
target_link_libraries(snmpfs ${NETSNMP_MIBS})

# TinyXML
include_directories("deps/tinyxml2/include/")
target_sources(snmpfs PRIVATE deps/tinyxml2/src/tinyxml2.cpp)


install(TARGETS snmpfs RUNTIME DESTINATION bin)
