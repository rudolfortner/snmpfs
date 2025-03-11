#pragma once

#include <filesystem>
#include <string>
#include <sstream>
#include <vector>

namespace snmpfs {

	/**
	 * FileNode is the internal representation of a single directory/file.
	*/
	class FileNode
	{
	public:
		FileNode();
		FileNode(std::string name);
		virtual ~FileNode();

		std::string name;
		std::vector<FileNode*> children;


		void addChild(FileNode* node);
		void freeChilds();
		FileNode* getChildByName(std::string name) const;
		std::string printFileTree() const;

		// CALLS FOR ACCESSING DATA
		virtual int open(bool trunc);
		virtual int read(char* buf, size_t size, off_t offset);
		virtual int write(const char* buf, size_t size, off_t offset);
		virtual int flush();
		virtual int release();
		virtual int truncate(off_t size);

		// CALLS FOR ATTRIBUTES
		virtual uint64_t getMode() const;
		virtual uint64_t getLinkCount() const;
		virtual uint64_t getSize() const;

		virtual timespec getTimeAccess() const;
		virtual timespec getTimeModification() const;
		virtual timespec getTimeStatusChange() const;

	private:
		void printFileTreeRec(std::stringstream& builder, uint32_t depth) const;
	};

	/**
	 * Searches the file hierarchy for the FileNode corresponding to the given path
	 */
	FileNode* getFileNodeByPath(std::filesystem::path path, FileNode* root);

	/**
	 * Inserts the given FileNode at its appropriate position according to the path
	 */
	void insertFileNodeByPath(std::filesystem::path path, FileNode* node, FileNode* root);

}	// namespace snmpfs
