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

