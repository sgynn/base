#pragma once

#include <vector>
#include <base/string.h>
#include <base/file.h>

namespace base {
	class VirtualFileSystem {
		public:
		class Folder;
		class File {
			public:
			String name;
			operator bool() const { return name; }
			bool operator==(const File& o) const { return m_source==o.m_source && name == o.name; }
			base::File read() const;
			const Folder* isFolder() const;
			String getFullPath() const; // For error log
			bool inArchive() const; // Is tthis file part of a pack, fullPath is not a valid file.
			File() {}
			private:
			File(VirtualFileSystem* fs, const String& name, int src) : name(name), m_fs(fs), m_source(src) {}
			friend class VirtualFileSystem;
			VirtualFileSystem* m_fs = nullptr;
			int m_source = 0;
		};
		class Folder {
			public:
			String name;
			const File& getFile(const char* name) const;
			std::vector<File>::const_iterator begin() const { return m_files.begin(); }
			std::vector<File>::const_iterator end() const { return m_files.end(); }
			private:
			Folder(VirtualFileSystem* fs, const String& name) : name(name), m_fs(fs) {}
			friend class VirtualFileSystem;
			VirtualFileSystem* m_fs;
			std::vector<File> m_files; // Includes folders
			std::vector<int> m_folders; 
		};

		public:
		VirtualFileSystem();
		bool addPath(const char*, bool recursive=true, const char* target=nullptr);
		bool addArchive(const char*, const char* target=nullptr);
		const Folder& getFolder(const char* path=0) const;
		const File& getFile(const char* path) const;

		private:
		friend class File;
		struct Source { String path; String mount; bool archive; String subFolder; };
		int getOrCreateFolder(const char* path);
		void addFile(Folder& folder, const char* name, int source);
		std::vector<Source> m_sources;
		std::vector<Folder> m_folders;
	};
}

