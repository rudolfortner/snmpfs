#pragma once

#include "virtualfile.h"
#include "../proc.h"

namespace snmpfs {

	/**
	 * Represents a ProcVariable as a file
	 */
	class ProcFile : public ProcObserver, public VirtualFile
	{
	public:
		ProcFile(std::string name, ProcBase* proc);
		~ProcFile();

		uint64_t getMode() const;
		int write(const char* buf, size_t size, off_t offset);

	private:
		ProcBase* proc;

		// ProcObserver
		void updateProc();
	};

}	// namespace snmpfs
