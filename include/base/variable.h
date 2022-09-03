#ifndef _SCRIPT_VARIABLE_
#define _SCRIPT_VARIABLE_

#include <base/math.h>
#include <base/hashmap.h>
#include <base/gui/delegate.h>
#include <initializer_list>
#include <vector>

#undef CONST // Some windows thing - how dare it!

namespace script {
	class Function;
	class VariableName;
	typedef std::vector<class String> StringList;

	/** Managed string class */
	class String {
		char* s;
		int* ref;
		void drop();
		void makeUnique();
		public:
		String(const char* s=0);
		String(const char* s, uint length);
		String(std::initializer_list<const char*> parts);
		String(const String&);
		String(String&&);
		~String();
		bool empty() const;
		uint length() const;
		const char* str() const;
		const String& operator=(const String&);
		const String& operator=(String&&);
		const String& operator=(const char*);
		operator const char*() const;
		String operator+(const String&) const;
		String operator+(const char*) const;
		friend String operator+(const char*, const String&);
		bool operator==(const char*) const;
		bool operator!=(const char*) const;
		char operator[](int) const;
		char& operator[](int);
		static StringList split(const char* s, const char* tok, bool keepEmpty=false, const char* trim=" ");
		static String cat(const char* a, const char* b);
	};


	/** Console Variables. Can be stured internally, or a pointer to external data */
	class Variable {
		friend class Expression;
		enum Types { OBJECT=1, ARRAY, BOOL, INT, UINT, FLOAT, DOUBLE, VEC2, VEC3, VEC4, STRING, FUNCTION, FIXED=0x20, CONST=0x40, LINK=0x80, EXPLICIT=0x100 };
		struct Object {
			std::vector<uint8> lookup;		// map of nameID -> item index
			std::vector<uint> keys;			// list of sub variable names
			std::vector<Variable> items;	// list of sub variables
			uint ref;
		};
		uint type;
		union {
			float f; double d; int i; uint u; const char* s;						// Local
			float* fp; double* dp; int* ip; uint* up; bool* bp; const char** sp;	// Linked
			Function* func;															// Function
			Object* obj;															// Object
		};
		Delegate<void(Variable&)> callback;	// Called when variable changed by a script
		template<typename T> T getValue() const;
		template<typename T> bool setValue(const T&);
		template<typename T> bool linkValue(uint t, T*& p, T& v, int flags);
		bool linkVector(uint type, float* value, int flags);
		bool setType(uint type);

		/// Object functions
		int  _contains(uint id) const;
		void _erase(uint id);
		void _eraseArray(uint index, bool keepOrder);
		Variable& _get(uint id) const;
		Variable& _set(uint id, const Variable&);
		public:
		static uint lookupName_const(const char* name);
		static uint lookupName(const char* name);
		static const char* lookupName(uint id);

		public:
		Variable();								// Constructor - creates null object
		~Variable();							// Destructor
		Variable(const Variable&);				// Copy constructor
		Variable(Variable&&);					// Move constructor
		const Variable& operator=(const Variable&);
		Variable& operator=(Variable&&);		// Move operator
		Variable copy(uint depth) const;		// Make a deep copy

		template<typename T>Variable(T v): type(0) { *this=v; }

		operator bool() const;
		operator float() const;					// Get value as float
		operator double() const;				// Get value as double
		operator int() const;					// Get value as int
		operator uint() const;					// Get value as uint
		operator vec2() const;
		operator vec3() const;
		operator vec4() const;
		operator String() const;				// Get value as string
		operator const char*() const;			// Get string value. Returns 0 if not a string
		operator Function*() const;				// Get as function. Returns 0 if not a function

		bool   operator=(bool);
		float  operator=(float);
		double operator=(double);
		int    operator=(int);
		uint   operator=(uint);
		Function* operator=(Function*);
		const vec2& operator=(const vec2&);
		const vec3& operator=(const vec3&);
		const vec4& operator=(const vec4&);
		const char* operator=(const char*);
		const char* operator=(const String&);

		bool operator==(const Variable&) const;
		bool operator!=(const Variable&) const;

		enum LinkFlags { LINK_DEFAULT, LINK_READONLY=1, LINK_SET=2 }; // LINK_SET uses value from script
		bool link(bool&,   int flags=0);
		bool link(float&,  int flags=0);
		bool link(double&, int flags=0);
		bool link(int&,    int flags=0);
		bool link(uint&,   int flags=0);
		bool link(vec2&,   int flags=0);
		bool link(vec3&,   int flags=0);
		bool link(vec4&,   int flags=0);
		bool link(const char*&, int flags=0);
		bool isLinked() const;
		void unlink();							// Unlink variable
		void lock();							// Make value read only
		void setExplicit(bool=true);			// Object child variables must be explicitly created.
		bool isExplicit() const;

		template<typename T> void set(const char* n, const T& v) { find(n) = v; } // Alternative syntax
		template<typename T> void set(uint n, const T& v) { get(n) = v; }
		template<typename T> bool link(const char* n, T& v, int f=0) { return find(n).link(v,f); } // Alternative syntax
		template<typename T> bool link(uint n, T& v, int f) { return get(n).link(v,f); }

		typedef Delegate<void(Variable&)> VariableCallback;
		void setCallback( const VariableCallback& );
		void fireCallback();

		bool isObject() const;					// Is this variable an object
		bool isArray() const;					// Is this variable an array
		bool isVector() const;					// Is it a vector (special object)
		bool isNumber() const;					// Is this variable a number
		bool isBoolean() const;					// Is this a boolean value
		bool isString() const;					// Is this a string value
		bool isFunction() const;				// Is this a function
		bool isConst() const;					// Is is const
		bool isNull() const;					// Is this a null variable
		bool isRef() const;						// Is this a linked variable
		void erase(const char* var);			// Erase sub variable
		void erase(uint id);					// Erase sub variable
		int contains(const char* name) const;	// Does this object contain a variable
		int contains(uint id) const;			// Does this object contain a variable
		int size() const;						// Number of elements if an object type
		int getReferenceCount() const;			// Get reference count of object types

		Variable& get(const char* name) const;	// Get sub variable
		Variable& get(const char* name);		// Get/create sub variable
		Variable& get_const(const char*) const;	// Get sub variable - doesn't create it if missing

		//Variable& get_const(const Variable& v) const;
		//Variable& get(const Variable& v) const { return get_const(v); }
		//Variable& get(const Variable& v);

		Variable& get(uint id) const;
		Variable& get(uint id);
		Variable& get_const(uint id) const;

		Variable& get(const VariableName&) const;
		Variable& get(const VariableName&);
		Variable& get_const(const VariableName&) const;

		Variable& find_const(const char* name) const;
		Variable& find(const char* name) const;
		Variable& find(const char* name);

		String toString(int d=1, bool quotes=false, bool multiLine=true, int indent=0) const;	// Get string representation
		bool isValid() const;					// Is this a valid variable. Use to check result of get().
		bool makeObject();						// Turn this variable into an object if not already
		bool makeArray();						// Turn this variable into an array type

		// A linear value to use as a key in maps
		size_t getObjectHash() const { return isObject() || isVector() || isArray()? (size_t)obj: 0; }
		
		// Object iterator
		struct SubItem { const char* const key; const uint id; Variable& value; };
		class iterator {
			friend class Variable;
			public:
			~iterator() { delete m_current; }
			iterator(const iterator& i) : m_key(i.m_key), m_value(i.m_value), m_current(0) { }
			iterator operator++() { iterator r=*this; ++m_key; ++m_value; return r; }
			iterator& operator++(int) { ++m_key; ++m_value; return *this; }
			iterator& operator=(const iterator& i) { m_key=i.m_key; m_value=i.m_value; return *this; }
			bool operator==(const iterator& i) const { return i.m_value==m_value; }
			bool operator!=(const iterator& i) const { return i.m_value!=m_value; }
			SubItem& operator*() const { updateCurrent(); return *m_current; }
			SubItem* operator->() const { updateCurrent(); return m_current; }
			private:
			iterator(uint* key, Variable* var, bool array) : m_key(key), m_value(var), m_array(array), m_current(0) { }
			void updateCurrent() const {
				if(m_current && ((m_array&&m_current->id==((size_t)m_key)/sizeof(uint)) || (!m_array&&m_current->id!=*m_key))) {
					delete m_current;
					m_current = 0;
				}
				if(m_value) {
					if(m_array) m_current = new SubItem{ 0, ((uint)(size_t)m_key)/4, *m_value };
					else m_current = new SubItem{ Variable::lookupName(*m_key), *m_key, *m_value };
				}
			}
			uint*     m_key;
			Variable* m_value;
			bool      m_array;
			mutable SubItem* m_current;
		};
		iterator begin() const;
		iterator end() const;
	};
	// Specialisation to allow ranged iterators to be on variables rather than subitems
	template<> inline Variable::Variable(Variable::SubItem v): type(0) { *this=v.value; }

	
	/// Variable name with pre-looked up strings
	class VariableName {
		friend class Variable;
		friend class Expression;
		public:
		VariableName(const char* = 0);
		String toString() const;
		bool operator==(const VariableName&) const;
		bool operator!=(const VariableName&) const;
		operator bool() const;
		protected:
		std::vector<uint> parts;
	};
}

#endif

