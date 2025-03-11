#pragma once

#include "objectid.h"
#include <mutex>
#include <string>
#include <vector>

namespace snmpfs {

	class ObjectObserver
	{
	public:
		/**
		 * Called when the actual data represented by this object changed
		 * restore is true if only the data should be set and not timestamps
		 * happens in case of an erronous write
		 */
		virtual void changed(bool restore) = 0;

		// TODO SNMP ERROR
		// virtual void errored() = 0;

		/**
		 * Is called when the Object was updated via SNMP, does not mean the data has changed
		 */
		virtual void updated() = 0;
	};

	// Forward declaration to avoid circular includes
	class Device;

	struct ObjectData {
		bool valid;
		uint64_t error;

		ObjectID id;
		unsigned char type;
		std::string data;
	};

	/**
	* @todo write docs
	*/
	class Object
	{
	public:
		/**
		* Default constructor
		*/
		Object(Device* device, ObjectID id);
		Object(Device* device, ObjectID id, char type);
		Object(Device* device, ObjectID id, const ObjectData& data);

		/**
		* Destructor
		*/
	virtual ~Object();

		// SNMP RELATED
		ObjectID getID() const;

		virtual bool isReadable() const;
		virtual bool isWritable() const;
		void setReadable(bool readable);
		void setWritable(bool writable);

		void handleError(const ObjectData& response);
		virtual std::string getData() const;									///> GET data represented as string
		virtual bool update();													///> UPDATE data from snmp GET
		virtual bool updateData(const std::string& data);						///> UPDATE data on Device
		virtual bool updateData(const ObjectID& oid, const std::string& data);	///> UPDATE data from string (does not update remote data)

		// OBSERVER RELATED
		void registerObserver(ObjectObserver* observer);
		void unregisterObserver(ObjectObserver* observer);
		void notifyChanged(bool restore = false) const;
		void notifyUpdated() const;

	protected:
		Device* device;

		ObjectID id;
		char type;
		std::string data;

		mutable std::mutex observerMutex;
		std::vector<ObjectObserver*> observers;
	};

}	// namespace snmpfs
