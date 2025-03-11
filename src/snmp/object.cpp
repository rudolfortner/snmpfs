#include "snmp/object.h"

#include "snmp/device.h"
#include <algorithm>

namespace snmpfs {

	Object::Object(Device* device, ObjectID id) : Object(device, id, '=')
	{

	}

	Object::Object(Device* device, ObjectID id, char type) : device(device), id(id), type(type)
	{

	}

	Object::Object(Device* device, ObjectID id, const ObjectData& data) : device(device), id(id), type(data.type)
	{
		this->data = data.data;
	}

	Object::~Object()
	{

	}

	ObjectID Object::getID() const
	{
		return id;
	}


	bool Object::isReadable() const
	{
		return id.isReadable();
	}

	bool Object::isWritable() const
	{
		return id.isWritable();
	}

	void Object::setReadable(bool readable)
	{
		id.setReadable(readable);
	}

	void Object::setWritable(bool writable)
	{
		id.setWritable(writable);
	}


	void Object::handleError(const ObjectData& response)
	{
		device->logErr("Error in response for object " + ((std::string) id) + " " + snmp_error_code_name(response.error));

		switch(response.error)
		{
			case SNMP_ERR_NOACCESS:
				id.setReadable(false);
			case SNMP_ERR_NOTWRITABLE:
				id.setWritable(false);
				break;
		}
	}

	std::string Object::getData() const
	{
		return data;
	}

	bool Object::update()
	{
		// printf("[Object] Requesting update of data from device\n");
		ObjectData response = device->get(id);

		if(response.valid)
		{
			// printf("[Object] SET was successful\n");
			notifyUpdated();
			updateData(response.id, response.data);
			return true;
		}
		else
		{
			// printf("[Object] GET failed\n");
			handleError(response);
			return false;
		}
	}

	bool Object::updateData(const std::string& data)
	{
		// Prepare data we set to Device via SET
		// E.g. numbers having a trailing \n fail
		std::string preparedData = data;
		if(data.ends_with('\n'))
		{
			preparedData = preparedData.substr(0, preparedData.size() - 1);
		}

		// Check if data even changed
		if(this->data == preparedData)
			return true;

		// printf("[Object] SET %s with %s\n", ((std::string) id).c_str(), preparedData.c_str());
		ObjectData response = device->set(id, type, preparedData);
		if(response.valid)
		{
			// printf("[Object] SET was successful\n");
			updateData(response.id, response.data);
			return true;
		}
		else
		{
			// printf("[Object] SET failed -> Restoring old data\n");
			handleError(response);
			notifyChanged(true);
			return false;
		}
	}


	bool Object::updateData(const ObjectID& oid, const std::string& data)
	{
		if(this->id != oid)
		{
			// Trying to update OID that is not represented by this Object
			// printf("DATA DOES NOT BELONG TO OBJECT\n");
			// printf("DATA FOR %s CANT BE SET ON %s\n", ((std::string) id).c_str(), ((std::string) getID()).c_str());
			return false;
		}

		if(this->data == data)
		{
			// DATA did not change, do not trigger notifyUpdate
			// TODO Think about whether this should be true or false
			// printf("DATA UNCHANGED\n");
			// printf("DATA FOR %s CANT BE SET ON %s\n", ((std::string) id).c_str(), ((std::string) getID()).c_str());
			return false;
		}

		// printf("[Object] Setting Object Data to %s\n", data.c_str());
		this->data = data;
		notifyChanged();
		return true;
	}



	void Object::registerObserver(ObjectObserver* observer)
	{
		std::unique_lock<std::mutex> lock(observerMutex);
		observers.push_back(observer);
	}

	void Object::unregisterObserver(ObjectObserver* observer)
	{
		std::unique_lock<std::mutex> lock(observerMutex);
		observers.erase(std::remove(observers.begin(), observers.end(), observer), observers.end());
	}


	void Object::notifyChanged(bool restore) const
	{
		std::unique_lock<std::mutex> lock(observerMutex);
		for(ObjectObserver* observer : observers)
		{
			observer->changed(restore);
		}
	}


	void Object::notifyUpdated() const
	{
		std::unique_lock<std::mutex> lock(observerMutex);
		for(ObjectObserver* observer : observers)
		{
			observer->updated();
		}
	}

}	// namespace snmpfs


