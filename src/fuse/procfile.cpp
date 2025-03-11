# include "fuse/procfile.h"

// #include "fuse/virtualfile.h"
// #include "proc.h"
#include <sys/stat.h>

namespace snmpfs {

	ProcFile::ProcFile(std::string name, ProcBase* proc) : VirtualFile(name), proc(proc)
	{
		proc->registerObserver(this);
		updateProc();
	}


	ProcFile::~ProcFile()
	{
		proc->unregisterObserver(this);
	}

	int ProcFile::write(const char* buf, size_t size, off_t offset)
	{
		return -EACCES;
	}

	uint64_t ProcFile::getMode() const
	{
		// By default FileNode represents a Directory with Read/Write for Owner/Group
		uint64_t mode = 0;
		mode |= S_IFREG;							// Regular File
		mode |= S_IRUSR | S_IRGRP | S_IROTH;		// Read by Owner and Group and Others
		// mode |= S_IWUSR | S_IWGRP | S_IWOTH;		// Write by Owner and Group and Others
		// mode |= S_IXUSR | S_IXGRP | S_IXOTH;		// Extended Read by Owner and Group and Others
		return mode;
	}

	void ProcFile::updateProc()
	{
		if(!proc) return;
		std::string str = proc->toString();
		truncate(str.size());
		VirtualFile::write(str.c_str(), str.size(), 0);
		// printf("Updating ProcFile %s with %s\n", name.c_str(), str.c_str());
	}

}	// namespace snmpfs
