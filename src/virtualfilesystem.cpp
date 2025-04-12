#include <base/virtualfilesystem.h>
#include <base/directory.h>
#include <base/archive.h>

using namespace base;

const VirtualFileSystem::Folder* VirtualFileSystem::File::isFolder() const {
	if(m_source&1<<31) return &m_fs->m_folders[m_source&0xffffff];
	else return nullptr;
}

base::File VirtualFileSystem::File::read() const {
	if(m_source<0 || !name) return base::File();
	Source& src = m_fs->m_sources[m_source];
	if(src.archive) {
		if(!src.subFolder) return Archive::loadFile(src.path, name);
		return Archive::loadFile(src.path, String::cat(src.subFolder, "/", name));
	}
	else {
		return base::File(String::cat(src.path, "/",  name), base::File::BUFFER);
	}
}

bool VirtualFileSystem::File::inArchive() const {
	if(m_source<0 || !name) return false;
	return m_fs->m_sources[m_source].archive;
}

String VirtualFileSystem::File::getFullPath() const {
	if(m_source<0 || !name) return "";
	Source& src = m_fs->m_sources[m_source];
	if(src.archive) {
		if(!src.subFolder) return String::cat(src.path, ":", name);
		return src.path +":" + src.subFolder + "/" + name;
	}
	else {
		return String::cat(src.path, "/",  name);
	}
}

VirtualFileSystem::VirtualFileSystem() {
	m_folders.push_back(Folder(this, ""));
}

const VirtualFileSystem::File& VirtualFileSystem::getFile(const char* path) const {
	static File null(nullptr, nullptr, 0);
	int dir = 0;
	while(path && path[0]) {
		const char* e = strchr(path, '/');
		if(!e) {
			for(const File& f: m_folders[dir].m_files) {
				if(f.name == path) return f;
			}
			return null;
		}
		StringView sub(path, e - path);
		if(sub.empty() || sub == ".") { path = e + 1; continue; }
		int subFolder = 0;
		for(int i: m_folders[dir].m_folders) {
			if(m_folders[i].name == sub) { subFolder = i; break; }
		}
		if(subFolder) {
			dir = subFolder;
			path = e + 1;
		}
		else return null; // Folder not found
	}
	return null;
}

const VirtualFileSystem::Folder& VirtualFileSystem::getFolder(const char* path) const {
	int dir = 0;
	while(path && path[0]) {
		const char* e = strchr(path, '/');
		StringView sub(path, e? e - path: strlen(path));
		if(sub.empty() || sub == ".") { path = e + 1; continue; }
		int subFolder = 0;
		for(int i: m_folders[dir].m_folders) {
			if(m_folders[i].name == sub) { subFolder = i; break; }
		}
		if(subFolder) {
			dir = subFolder;
			path = e + 1;
		}
		else { // Not found
			static Folder null(nullptr, nullptr);
			return null;
		}
	}
	return m_folders[dir];
}


int VirtualFileSystem::getOrCreateFolder(const char* path) {
	int dir = 0;
	while(path && path[0]) {
		const char* e = strchr(path, '/');
		StringView sub(path, e? e - path: strlen(path));
		if(sub.empty() || sub == ".") { path = e + 1; continue; }
		int subFolder = 0;
		for(int i: m_folders[dir].m_folders) {
			if(m_folders[i].name == sub) { subFolder = i; break; }
		}
		if(subFolder) dir = subFolder;
		else {
			int newDir = m_folders.size();
			m_folders[dir].m_folders.push_back(newDir);
			m_folders[dir].m_files.push_back(File(this, sub, 1<<31 | newDir));
			m_folders.push_back(Folder(this, sub));
			dir = newDir;
		}
		path = e? e + 1: nullptr;
	}
	return dir;
}

void VirtualFileSystem::addFile(Folder& folder, const char* name, int source) {
	for(File& f: folder.m_files) {
		if(f.name == name) {
			f.name = name;
			f.m_source = source;
			return;
		}
	}
	folder.m_files.push_back(File(this, name, source));
}

bool VirtualFileSystem::addPath(const char* path, bool recursive, const char* target) {
	for(Source& s: m_sources) {
		if(!s.archive && s.path == path && s.mount == target) return false; // Already exists
	}
	int source = m_sources.size();
	m_sources.push_back({path, target, false});
	int folder = getOrCreateFolder(target);

	// Add files
	for(auto& f: Directory(path)) {
		if(f.type == Directory::FILE) addFile(m_folders[folder], f.name, source);
		else if(recursive && f.name[0] != '.') {
			addPath(String::format("%s/%s", path, f.name), true, target? String::format("%s/%s", target, f.name): f.name);
		}
	}
	return true;
}

bool VirtualFileSystem::addArchive(const char* path, const char* target) {
	for(Source& s: m_sources) {
		if(s.archive && s.path == path) return false; // Already exists
	}
	// Index files
	Archive arc(path);
	if(!arc) return false;
	size_t sourceStart = m_sources.size();
	for(auto& f: arc) {
		// File name may have path
		const char* e = strrchr(f.name, '/');
		StringView subPath = e? StringView(f.name, e-f.name): StringView();
		int source = -1;
		// Get or create sub-source
		for(size_t i=sourceStart; i<m_sources.size(); ++i) {
			if(m_sources[i].subFolder == subPath) { source=i; break; }
		}
		if(source<0) {
			source = m_sources.size();
			m_sources.push_back(Source{path, target, true, subPath});
		}

		// Add file
		if(e) {
			int folder = getOrCreateFolder(String(target) + "/" + StringView(f.name, e-f.name));
			addFile(m_folders[folder], e+1, source);
		}
		else {
			int folder = getOrCreateFolder(target);
			addFile(m_folders[folder], f.name, source);
		}
	}
	return true;
}

bool VirtualFileSystem::scanFolders() {
	int foundNewFiles = 0;
	for(size_t i=0; i<m_sources.size(); ++i) {
		if(m_sources[i].archive) continue;
		int folder = getOrCreateFolder(m_sources[i].mount);
		for(auto& f: Directory(m_sources[i].path)) {
			if(f.type == Directory::FILE) {
				bool exists = false;
				for(const File& file: m_folders[folder]) {
					if(file.name == f.name) { exists=true; break; }
				}
				if(exists) continue;
				addFile(m_folders[folder], f.name, i);
				++foundNewFiles;
			}
			// ToDo: subfolders if recursive
		}
	}
	return foundNewFiles;
}


int VirtualFileSystem::getSource(const char* systemPath) const {
	for(size_t i=0; i<m_sources.size(); ++i) {
		if(m_sources[i].path == systemPath) return i;
	}
	return -1;
}

int VirtualFileSystem::getWriteableSource() const {
	int source = -1;
	for(size_t i=0; i<m_sources.size(); ++i) {
		if(m_sources[i].archive) continue; // Archives are read only
		if(source<0 || !m_sources[i].path.startsWith(m_sources[source].path)) source = i;
	}
	return source;
}

int VirtualFileSystem::getFolderFromFile(const char* filePath) {
	const char* e = strrchr(filePath, '/');
	StringView path = e? StringView(filePath, e-filePath): StringView();
	for(uint i=0; i<m_folders.size(); ++i) if(m_folders[i].name == path) return i;
	return -1;
}

const VirtualFileSystem::File& VirtualFileSystem::createTransientFile(const char* file, int source) {
	static File invalid;
	if(getFile(file)) return invalid; // File already exists
	if(source < 0) source = getWriteableSource();
	if(source<0 || source >= (int)m_sources.size()) return invalid;

	int folder = getFolderFromFile(file);
	if(folder < 0) return invalid;
	const char* e = strrchr(file, '/');
	const char* filename = e? e+1: file;

	m_folders[folder].m_files.push_back(File(this, filename, source));
	return m_folders[folder].m_files.back();
}

bool VirtualFileSystem::removeFile(const char* file) {
	int folderIndex = getFolderFromFile(file);
	if(folderIndex<0) return false;
	Folder& folder = m_folders[folderIndex];
	const char* e = strrchr(file, '/');
	const char* filename = e? e+1: file;

	for(size_t i=0; i<folder.m_files.size(); ++i) {
		if(folder.m_files[i].name == filename) {
			folder.m_files[i] = folder.m_files.back();
			folder.m_files.pop_back();
			return true;
		}
	}
	return false;
	
	
}


