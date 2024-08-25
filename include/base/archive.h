#pragma once

#include <vector>
#include <base/string.h>

namespace base {
	class File;
	class Archive {
		public:
		struct ArchiveFile {
			String name;
			int index;
		};

		Archive(const char* path);
		~Archive();
		File readFile(const char* file);
		std::vector<ArchiveFile>::const_iterator begin() const { return m_files.begin(); }
		std::vector<ArchiveFile>::const_iterator end() const { return m_files.end(); }
		operator bool() const { return m_valid; }

		static File loadFile(const char* archive, const char* file);
		private:
		std::vector<ArchiveFile> m_files;
		String m_fileName;
		bool m_valid = false;
	};
}
