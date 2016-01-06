#ifndef _DIRECTORY_
#define _DIRECTORY_

#include <vector>

namespace base {

	/** Directory class for listing files in a directory */
	class Directory {
		public:
		enum FileType { FILE, DIRECTORY };

		Directory(const char* path=".");
		~Directory();
		/** Test if a file is in this directory */
		bool contains(const char* file);
		/** Get the current path */
		const char* path() const { return m_path; }

		/// Iterator ///
		struct File { char name[64]; int ext; int type; };
		typedef std::vector<File>::const_iterator iterator;

		iterator begin() { scan(); return m_files.begin(); }
		iterator end() const   { return m_files.end(); }
		
		/** Clean a path */
		static int clean(const char* path, char* out, int lim=0);
		static int getFullPath(const char* path, char* out, int lim=0);
		static int getRelativePath(const char* path, char* out, int lim=0);
		static bool isRelative(const char*);
		protected:
		int scan();
		char m_path[1024];
		std::vector<File> m_files;
	};
};

#endif

