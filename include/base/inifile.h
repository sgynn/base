#ifndef _BASE_INIFILE_
#define _BASE_INIFILE_

#include "hashmap.h"
#include <vector>

/** Dos style INI file for simple configuration */
namespace base {
class INIFile {
	public:
	INIFile();
	INIFile(INIFile&&);
	INIFile& operator=(INIFile&&);
	~INIFile();

	// usage: var = ini[section][name]; and ini[section][name]=5;
	// also INIFile::Section s = ini[section];  var=s[name];
	class Value {
		friend class INIFile;
		enum ValueType { SOURCE=0, STRING, FLOAT, BOOL, INTEGER, UNSIGNED };
		public:
		Value():              m_type(SOURCE), m_source(0) {}
		Value(const char* c): m_type(STRING), m_source(0), m_c(c) {}
		Value(float f):       m_type(FLOAT),  m_source(0), m_f(f) {}
		Value(bool b):        m_type(BOOL),   m_source(0), m_b(b) {}
		Value(int i):         m_type(INTEGER),m_source(0), m_i(i) {}
		Value(unsigned u):    m_type(UNSIGNED),m_source(0), m_u(u) {}
		const Value& operator=(const char* c) { setType(STRING);  m_c=c; return *this; }
		const Value& operator=(double f)      { setType(FLOAT);   m_f=f; return *this; }
		const Value& operator=(float f)       { setType(FLOAT);   m_f=f; return *this; }
		const Value& operator=(bool b)        { setType(BOOL);    m_b=b; return *this; }
		const Value& operator=(int i)         { setType(INTEGER); m_i=i; return *this; }
		const Value& operator=(unsigned u)    { setType(UNSIGNED);m_u=u; return *this; }
		operator const char*();
		operator const char*() const;
		operator double() const;
		operator float() const;
		operator bool() const;
		operator int() const;
		operator unsigned() const;
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
			unsigned m_u;
		};
	};

	/** Key value pair */
	struct KeyValue  { const char* key; Value value; };

	/** INI files are divided into sections */
	class Section {
		friend class INIFile;
		public:
		Section(const char* name);                   // Constructor
		~Section();                                  // Destructor
		const char* name() const { return m_name; }  // Get section name
		const Value& operator[](const char*) const;  // Get value by key
		Value& operator[](const char*);              // Get value by key
		const Value& get(const char* key) const;     // Get value by key
		void set(const char* key, const Value& v);   // Set value. Replaces any existing value of key
		void add(const char* key, const Value& v);   // Add a value. key can be a duplicate
		bool contains(const char* key) const;        // Does a key exist
		template<typename T> T get(const char* s, T d) const { return contains(s)? (T)get(s): d; }
		
		typedef std::vector<KeyValue>::const_iterator iterator;
		iterator begin() const { return m_values.begin(); }
		iterator end() const { return m_values.end(); }

		private:
		char m_name[32];
		base::HashMap<int> m_map;
		std::vector<KeyValue> m_values;
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

	/** section iterator */
	typedef base::HashMap<Section*>::const_iterator iterator;
	iterator begin() const { return m_sections.begin(); }
	iterator end() const { return m_sections.end(); }

	private:
	base::HashMap<Section*> m_sections;
	static const Section nullSection;

};
};

#endif

