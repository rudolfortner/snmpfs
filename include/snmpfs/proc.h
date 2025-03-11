#pragma once

#include <mutex>
#include <string>
#include <vector>

namespace snmpfs {

	class ProcObserver
	{
	public:
		virtual void updateProc() = 0;
	};



	class ProcBase
	{
	public:
		virtual std::string toString() const = 0;

		void registerObserver(ProcObserver* observer);
		void unregisterObserver(ProcObserver* observer);
		void fireChanged() const;
	private:
		mutable std::mutex mutex;
		std::vector<ProcObserver*> observers;
	};


	template <typename T>
	class ProcVariable : public ProcBase
	{
	public:
		ProcVariable()
		{

		}

		ProcVariable(T value)
		{
			set(value);
		}

		T get() const
		{
			std::unique_lock<std::recursive_mutex> lock(mutex);
			return value;
		}

		void set(T set)
		{
			std::unique_lock<std::recursive_mutex> lock(mutex);
			value = set;
			fireChanged();
		}

		virtual std::string toString() const = 0;

	protected:
		mutable std::recursive_mutex mutex;
		T value;
	};

	class ProcCounter : public ProcVariable<uint64_t>
	{
	public:

		ProcCounter()
		{
			set(0);
		}

		ProcCounter(uint64_t value)
		{
			set(value);
		}

		void inc()
		{
			std::unique_lock<std::recursive_mutex> lock(mutex);
			value++;
			fireChanged();
		}

		void dec()
		{
			std::unique_lock<std::recursive_mutex> lock(mutex);
			value--;
			fireChanged();
		}


		std::string toString() const
		{
			std::unique_lock<std::recursive_mutex> lock(mutex);
			return std::to_string(value);
		}

	};


	class ProcString : public ProcVariable<std::string>
	{
	public:

		ProcString()
		{
			set(std::string());
		}

		ProcString(std::string value)
		{
			set(value);
		}

		std::string toString() const
		{
			std::unique_lock<std::recursive_mutex> lock(mutex);
			return value;
		}

	};



	class ProcTimestamp : public ProcVariable<std::time_t>
	{
	public:

		ProcTimestamp()
		{
			set(0);
		}

		ProcTimestamp(std::time_t value)
		{
			set(value);
		}

		void setNow()
		{
			set(std::time(nullptr));
		}

		std::string toString() const
		{
			std::unique_lock<std::recursive_mutex> lock(mutex);
			return std::asctime(std::gmtime(&value));
		}

	};

	struct ProcData {
		// SNMPFS
		ProcTimestamp snmpfsCreation;
		ProcTimestamp snmpfsLastUpdate;
		ProcString snmpfsPID, snmpfsUID, snmpfsGID;
		ProcString snmpfsVersion;

		// FUSE Counters
		// TODO Counters for getattr, open, write, read, ...

		// FUSE Timestamps
		// TODO Timestamps for getattr, open, write, read, ...


		// SNMP Counters
		ProcCounter snmpRequestCount;
		ProcCounter snmpRequestTimeouts;
		ProcCounter snmpRequestErrors;

		// SNMP Timestamps
		ProcTimestamp snmpLastRequest;
	};

	class FileNode;
	void addProcFile(FileNode* parent, const std::string name, ProcBase* base);
	FileNode* createProcFS(ProcData* data);

}	// namespace snmpfs
