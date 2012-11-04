#ifndef _BASE_INIFILE_
#define _BASE_INIFILE_

#include "hashmap.h"

/** Dos style INI file for simple configuration */
namespace base {
class INIFile {
	public:
	INIFile();
	~INIFile();

	bool loadFile(const char* filename);
	bool setData(const char* data, size_t length);
	bool saveFile(const char* filename);
	size_t getData(char* buffer, int length);

	class Section {
		friend class INIFile;
		public:
		Section(const char* name);
		~Section();
		const char* name() const { return m_name; }
		void setValue(const char* name, const char* value);
		void setValue(const char* name, float value);
		void setValue(const char* name, int value);
		void setValue(const char* name, unsigned int value);
		int getValue(const char* name, int defaultValue) const;
		unsigned int getValue(const char* name, unsigned int defaultValue) const;
		float getValue(const char* name, float defaultValue) const;
		const char* getValue(const char* name, const char* defaultValue="") const;

		//base::HashMap<const char*>::iterator begin() { return m_data.begin(); }
		//base::HashMap<const char*>::iterator end() { return m_data.end(); }
		private:
		char m_name[32];
		base::HashMap<const char*> m_data;
	};

	Section* getSection(const char* name);

	private:
	base::HashMap<Section*> m_sections;

};
};

#endif

