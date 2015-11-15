#ifndef _BASE_INIFILE_
#define _BASE_INIFILE_

#include "hashmap.h"

/** Dos style INI file for simple configuration */
namespace base {
class INIFile {
	public:
	INIFile();
	~INIFile();

	// usage: var = ini[section][name]; and ini[section][name]=5;
	// also INIFile::Section s = ini[section];  var=s[name];
	class Value {
		friend class INIFile;
		enum ValueType { SOURCE=0, STRING, FLOAT, BOOL, INTEGER };
		public:
		Value():              m_type(SOURCE), m_source(0) {}
		Value(const char* c): m_type(STRING), m_source(0), m_c(c) {}
		Value(float f):       m_type(FLOAT),  m_source(0), m_f(f) {}
		Value(bool b):        m_type(BOOL),   m_source(0), m_b(b) {}
		Value(int i):         m_type(INTEGER),m_source(0), m_i(i) {}
		const Value& operator=(const char* c) { setType(STRING);  m_c=c; return *this; }
		const Value& operator=(double f)      { setType(FLOAT);   m_f=f; return *this; }
		const Value& operator=(float f)       { setType(FLOAT);   m_f=f; return *this; }
		const Value& operator=(bool b)        { setType(BOOL);    m_b=b; return *this; }
		const Value& operator=(int i)         { setType(INTEGER); m_i=i; return *this; }
		operator const char*();
		operator const char*() const;
		operator double() const;
		operator float() const;
		operator bool() const;
		operator int() const;
		private:
		void setType(int t);
		void setString();
		int m_type;
		char* m_source;
		union {
			const char* m_c;
			float m_f;
			bool  m_b;
			int   m_i;
		};
	};

	/** INI files are divided into sections */
	class Section {
		friend class INIFile;
		public:
		Section(const char* name);                   // Constructor
		~Section();                                  // Destructor
		const char* name() const { return m_name; }  // Get section name
		const Value& operator[](const char*) const;  // Get value
		Value& operator[](const char*);              // Get value
		const Value& get(const char* s) const;       // Get value
		void set(const char* s, const Value& v);     // Set value
		bool contains(const char* s) const;          // Does a key exist
		template<typename T> T get(const char* s, T d) const { return contains(s)? (T)get(s): d; }
		private:
		char m_name[32];
		base::HashMap<Value> m_values;
		static const Value nullValue;
	};


	/** Load ini file */
	static INIFile load(const char* file);
	/** Parse ini file string */
	static INIFile parse(const char* string);
	/** Save ini file */
	bool save(const char* file);
	/** Get file as string */
	size_t toString(char* buffer, int max);

	/** Get ini section using [] */
	const Section& operator[](const char* name) const;
	Section& operator[](const char* name);

	/** Get section pointer */
	Section* section(const char* name);
	/** Does a section exist? */
	bool containsSection(const char* name) const;

	private:
	base::HashMap<Section*> m_sections;
	static const Section nullSection;

};
};

#endif

