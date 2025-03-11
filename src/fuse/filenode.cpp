#include "fuse/filenode.h"
#include <sys/stat.h>

namespace snmpfs {

	FileNode::FileNode() : FileNode("unnamed")
	{

	}

	FileNode::FileNode(std::string name)
	{
		this->name = name;
	}

	FileNode::~FileNode()
	{

	}

	void FileNode::addChild(FileNode* node)
	{
		children.push_back(node);
	}

	void FileNode::freeChilds()
	{
		for(FileNode* child : children)
		{
			child->freeChilds();
			delete child;
		}
		children.clear();
	}

	FileNode* FileNode::getChildByName(std::string name) const
	{
		for(FileNode* child : children)
		{
			if(child->name == name) return child;
		}
		return nullptr;
	}

	std::string FileNode::printFileTree() const
	{
		std::stringstream stream;
		printFileTreeRec(stream, 0);
		return stream.str();
	}

	void FileNode::printFileTreeRec(std::stringstream& builder, uint32_t depth) const
	{
		for(uint32_t d = 1; d < depth; d++)
			builder << "    ";
		if(depth > 0)
			builder << "|-- ";
		builder << name << std::endl;

		for(const FileNode* child : children)
			child->printFileTreeRec(builder, depth + 1);
	}


	int FileNode::open(bool trunc)
	{
		return 0;
	}

	int FileNode::read(char* buf, size_t size, off_t offset)
	{
		return 0;
	}

	int FileNode::write(const char* buf, size_t size, off_t offset)
	{
		return 0;
	}

	int FileNode::truncate(off_t size)
	{
		return 0;
	}

	int FileNode::flush()
	{
		return 0;
	}

	int FileNode::release()
	{
		return 0;
	}




	uint64_t FileNode::getMode() const
	{
		// By default FileNode represents a Directory with Read/Write for Owner/Group
		uint64_t mode = 0;
		mode |= S_IFDIR;							// Directory
		mode |= S_IRUSR | S_IRGRP | S_IROTH;		// Read by Owner and Group and Others
		mode |= S_IWUSR | S_IWGRP | S_IWOTH;		// Write by Owner and Group and Others
		mode |= S_IXUSR | S_IXGRP | S_IXOTH;		// Extended Read by Owner and Group and Others
		return mode;
	}

	uint64_t FileNode::getLinkCount() const
	{
		return 1;
	}

	uint64_t FileNode::getSize() const
	{
		return 0;
	}


	timespec FileNode::getTimeAccess() const
	{
		timespec ts;
		timespec_get(&ts, TIME_UTC);
		return ts;
	}

	timespec FileNode::getTimeModification() const
	{
		timespec ts;
		timespec_get(&ts, TIME_UTC);
		return ts;
	}

	timespec FileNode::getTimeStatusChange() const
	{
		timespec ts;
		timespec_get(&ts, TIME_UTC);
		return ts;
	}







	FileNode* getFileNodeByPath(std::filesystem::path path, FileNode* root)
	{
		if(root == NULL) return NULL;

		FileNode* node = nullptr;
		for(auto& part : path)
		{
			if(part == "/")
			{
				node = root;
				continue;
			}

			if(!node)
				return nullptr;

			node = node->getChildByName(part);
		}
		return node;
	}

	void insertFileNodeByPath(std::filesystem::path path, FileNode* node, FileNode* root)
	{
		if(!node) throw std::runtime_error("node is null");
		if(!root) throw std::runtime_error("root is null");

		std::vector<std::string> sep;
		for(auto& part : path)
			sep.push_back(part);


		if(sep.size() == 1)
		{
			if(sep[0] != "/")	node->name = sep[0];
			root->addChild(node);
			return;
		}

		FileNode* current = nullptr;
		for(size_t i = 0; i < sep.size(); i++)
		{
			const std::string& part = sep[i];

			// CHECK ROOT
			if(i == 0 && part == "/")
			{
				current = root;
				continue;
			}

			if(!current)
				return;

			if(i == sep.size() - 1)
			{
				if(current->getChildByName(node->name))
				{
					throw std::runtime_error("File already there");
				}
				else if(part.empty())
				{
					current->addChild(node);
					return;
				}
				else if(part == path.filename())
				{
					node->name = part;
					current->addChild(node);
					return;
				}

			}

			FileNode* child = current->getChildByName(part);
			if(!child)
			{
				// Create directory node that is missing in between
				child = new FileNode(part);
				current->addChild(child);
			}
			current = child;
		}
	}

}	// namespace snmpfs

