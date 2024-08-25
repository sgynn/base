#include <base/archive.h>
#include <base/file.h>

#define MINIZ_HEADER_FILE_ONLY
#include "miniz.c"


using namespace base;

Archive::Archive(const char* file) : m_fileName(file) {
	mz_zip_archive zipFile;
	memset(&zipFile, 0, sizeof(zipFile));
	mz_bool status = mz_zip_reader_init_file(&zipFile, file, 0);
	if(!status) return;

	int files = mz_zip_reader_get_num_files(&zipFile);
	mz_zip_archive_file_stat stat;
	for(int i=0; i<files; ++i) {
		if(mz_zip_reader_file_stat(&zipFile, i, &stat)) {
			if(stat.m_uncomp_size > 0) m_files.push_back({stat.m_filename, i});
			m_valid = true;
		}
		else break;
	}
	mz_zip_reader_end(&zipFile);
}

Archive::~Archive() {
}

File Archive::readFile(const char* name) {
	File file;
	for(ArchiveFile& f: m_files) {
		if(strcmp(f.name, name)==0) {
			size_t size;
			mz_zip_archive zipFile;
			memset(&zipFile, 0, sizeof(zipFile));
			mz_zip_reader_init_file(&zipFile, m_fileName, 0);
			zipFile.m_pAlloc = [](void* mem, size_t i, size_t s)->void* { return new char[i*s]; };
			mz_bool status = mz_zip_reader_init_file(&zipFile, m_fileName, 0);
			if(!status) return file;
			if(void* data = mz_zip_reader_extract_to_heap(&zipFile, f.index, &size, 0)) {
				file = File(name, (char*)data, size);
			}
			mz_zip_reader_end(&zipFile);
		}
	}
	return file;
}

File Archive::loadFile(const char* archive, const char* name) {
	File file;
	mz_zip_archive zipFile;
	memset(&zipFile, 0, sizeof(zipFile));
	mz_zip_reader_init_file(&zipFile, archive, 0);
	int fileIndex = mz_zip_reader_locate_file(&zipFile, name, 0, 0);
	if(fileIndex >= 0) {
		mz_zip_archive_file_stat stat;
		if(mz_zip_reader_file_stat(&zipFile, fileIndex, &stat)) {
			char* data = new char[stat.m_uncomp_size+1];
			if(mz_zip_reader_extract_to_mem(&zipFile, fileIndex, data, stat.m_uncomp_size, 0)) {
				data[stat.m_uncomp_size] = 0;
				file = File(name, data, stat.m_uncomp_size);
			}
			else delete [] data;
		}
	}
	mz_zip_reader_end(&zipFile);
	return file;
}

