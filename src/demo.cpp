#include "demo.h"

#include "fuse/virtualfile.h"
#include "fuse/virtuallogger.h"

namespace snmpfs {

	FileNode* Demo::createTree()
	{
		FileNode* root = new FileNode("/");
		FileNode* devices = new FileNode("devices");
		FileNode* dev0 = new FileNode("dev0");
		FileNode* dev1 = new FileNode("dev1");

		root->addChild(devices);
		devices->addChild(dev0);
		devices->addChild(dev1);

		VirtualFile* virt0 = new VirtualFile("virt0");
		VirtualFile* virt1 = new VirtualFile("virt1");
		root->addChild(virt0);
		root->addChild(virt1);

		VirtualLogger* vLog = new VirtualLogger("logger");
		vLog->printLine("New VirtualLogger created!");
		for(int i = 0; i < 100; i++) vLog->printLine(std::to_string(i) + ": Printing Demo Message");
		root->addChild(vLog);

		insertFileNodeByPath("/devices/test/virtual/virt0", virt0, root);
		VirtualFile* oha = new VirtualFile("oha");
		insertFileNodeByPath("oha", oha, root);
		printf("%s", root->printFileTree().c_str());

		FileNode* node = getFileNodeByPath("/devices/dev2", root);
		if(node == dev0) printf("IT WORKS\n");
		if(!node) printf("File not found\n");

		return root;
	}

	void Demo::addObject(DeviceConfig& device, std::string name, std::string oid, ObjectType type)
	{
		ObjectConfig config;
		config.name		= name;
		config.interval	= 5;
		config.rawOID	= oid;
		config.type		= type;
		device.objects.push_back(config);
	}

	void Demo::addObjects(DeviceConfig& device)
	{
		addObject(device, "hostname",		"iso.3.6.1.2.1.1.5.0",		SCALAR);	// STRING
		addObject(device, "uptime",		"iso.3.6.1.2.1.25.1.1.0",		SCALAR);	// TIMETICKS
		addObject(device, "oid-example",	"iso.3.6.1.2.1.1.2.0",		SCALAR);	// OID
		addObject(device, "hex-string",	"iso.3.6.1.2.1.25.1.2.0",		SCALAR);	// HEX-STRING
		addObject(device, "integer",		"iso.3.6.1.2.1.25.1.3.0",	SCALAR);	// INTEGER
		addObject(device, "gauge",		"iso.3.6.1.2.1.25.1.5.0",		SCALAR);	// GAUGE32


		addObject(device, "interfaces",	"iso.3.6.1.2.1.2.2",			TABLE);		// TABLE
		addObject(device, "system",		"iso.3.6.1.2.1.1",				TREE);		// SUBTREE
		addObject(device, "all",			"iso",						TREE);		// SUBTREE

		ObjectConfig table;
		table.name		= "storage";
		table.interval	= 5;
		table.rawOID	= "iso.3.6.1.2.1.25.2.3";
		table.type		= TABLE;
		table.columns.push_back({"id", "iso.3.6.1.2.1.25.2.3.1.1"});
		table.columns.push_back({"desc", "iso.3.6.1.2.1.25.2.3.1.3"});
		table.columns.push_back({"size", "iso.3.6.1.2.1.25.2.3.1.5"});
		table.columns.push_back({"used", "iso.3.6.1.2.1.25.2.3.1.6"});
		device.objects.push_back(table);

		// 	add(device, "storageFull",	"iso.3.6.1.2.1.25.2.3",			TABLE);		// TABLE
	}


	snmpfsConfig Demo::createConfig()
	{
		snmpfsConfig config;

		DeviceConfig device;
		device.interval = 0;	// TODO Allow interval of 0 (immediate update ?)
		device.interval = 5;
		device.name		= "virtsnmp0";
		device.peername	= "192.168.56.201";
		device.auth.version		= VERSION_1;
		device.auth.username	= "public";
		device.auth.version		= VERSION_2c;
		addObject(device, "sysContact",		"iso.3.6.1.2.1.1.4.0",		SCALAR);
		addObject(device, "all",			"iso",						TREE);
		// addObject(device, "sysLocation",	"iso.3.6.1.2.1.1.6.0",		SCALAR);
		// addObjects(device);
		config.devices.push_back(device);

		return config;

		device.name		= "virtsnmp0B";
		config.devices.push_back(device);

		DeviceConfig hpswitch = device;
		hpswitch.name		= "switch";
		hpswitch.peername	= "192.168.2.3";
		hpswitch.auth.version	= VERSION_1;
		hpswitch.auth.username	= "public";
		addObjects(hpswitch);
		config.devices.push_back(hpswitch);


		// TODO For testing snmpfs with loads of devices
		for(size_t i = 0; i < 100; i++)
		{
			DeviceConfig gen;
			gen.interval	= 5;
			gen.name		= "demo" + std::to_string(i);
			gen.peername	= "192.168.56.201";
			gen.auth.version	= VERSION_1;
			gen.auth.username	= "public";
			addObjects(gen);
			config.devices.push_back(gen);
		}

		return config;
	}

}	// namespace snmpfs
