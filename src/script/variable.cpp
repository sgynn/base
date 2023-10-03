#include <base/script.h>
#include <assert.h>
#include <algorithm>
#include <cstring>
#include <cstdio>


#define NAME_LIMIT 128

using namespace script;

namespace script { Variable nullVar; }
bool Variable::isValid() const { return this != &nullVar; }
struct FixNullVar { FixNullVar() { nullVar.lock(); } } FixNullVarHack;


// ------------------------------------------------ //
static base::HashMap<uint> nameLookup;
static std::vector<const char*> reverseLookup;
uint Variable::lookupName(const char* name) {
	assert(name);
	base::HashMap<uint>::iterator it = nameLookup.find(name);
	if(it!=nameLookup.end()) return it->value;
	else {
		assert(!strchr(name, '.')); // ERROR: Name contains '.' - Probably a get() instead of find()
		nameLookup[name] = reverseLookup.size();
		reverseLookup.push_back( nameLookup.find(name)->key );
		return nameLookup.size() - 1;
	}
}
const char* Variable::lookupName(uint id) {
	return id<reverseLookup.size()? reverseLookup[id]: "";
}

uint Variable::lookupName_const(const char* name) {
	base::HashMap<uint>::iterator it = nameLookup.find(name);
	if(it!=nameLookup.end()) return it->value;
	else return ~0u;
}

// ------------------------------------------------ //

VariableName::VariableName(const char* name) {
	if(name && name[0]) {
		char* tmp = strdup(name);
		char* n = strtok(tmp, ".");
		while(n) {
			parts.push_back(Variable::lookupName(n));
			n = strtok(NULL, ".");
		}
		free(tmp);
	}
}
String VariableName::toString() const {
	if(parts.empty()) return String();
	String s = Variable::lookupName(parts[0]);
	for(uint i=1; i<parts.size(); ++i) s = s + "." + Variable::lookupName(parts[i]);
	return s;
}
VariableName::operator bool() const {
	return !parts.empty();
}
bool VariableName::operator==(const VariableName& o) const {
	if(parts.size()!=o.parts.size()) return false;
	for(uint i=0; i<parts.size(); ++i) if(parts[i] != o.parts[i]) return false;
	return true;
}
bool VariableName::operator!=(const VariableName& o) const {
	return !(*this == o);
}

VariableName& VariableName::operator+=(uint id) { parts.push_back(id); return *this; }
VariableName& VariableName::operator+=(const char* name) { parts.push_back(Variable::lookupName(name)); return *this; }
VariableName& VariableName::operator+=(const VariableName& name) { parts.insert(parts.end(), name.parts.begin(), name.parts.end()); return *this; }
VariableName VariableName::operator+(uint id) const { VariableName n=*this; n += id; return n; }
VariableName VariableName::operator+(const char* name) const { VariableName n=*this; n += name; return n; }
VariableName VariableName::operator+(const VariableName& name) const { VariableName n=*this; n += name; return n; }
VariableName operator+(uint id, const VariableName& name) { VariableName n; n += id; n += name; return n; }
VariableName operator+(const char* pre, const VariableName& name) { VariableName n; n += pre; n += name; return n; }

// ------------------------------------------------ //


Variable::Variable() : type(0) { }
Variable::~Variable() {
	// Delete data
	if((isObject() || isVector() || isArray()) && --obj->ref==0) delete obj;
	if(isFunction() && func && --func->ref==0) delete func;
	if(type == STRING && s) free((char*)s);
}
//// Move constructor ////
Variable::Variable(Variable&& v) : type(v.type) {
	obj = v.obj;
	callback = v.callback;
	v.obj = 0;
	v.type = 0;
	v.callback.unbind();
}
Variable& Variable::operator=(Variable&& v) {
	obj = v.obj;
	type = v.type;
	callback = v.callback;
	v.obj = 0;
	v.type = 0;
	v.callback.unbind();
	return *this;
}

//// Duplication ////
Variable::Variable(const Variable& v) : type(0) { *this = v; }
const Variable& Variable::operator=(const Variable& v) {
	if(this==&v) return v;
	if(isConst()) return *this;
	if(setType(v.type&0xf)) {
		// Objects set by reference
		if(isObject() || isArray()) {
			if(obj && --obj->ref==0) delete obj;
			obj = v.obj;
			++obj->ref;
		}
		else if(isVector()) {
			for(iterator i=v.begin(); i!=v.end(); ++i) {
				_set(i->id, i->value);
			}
		}
		else if(type==STRING)		  s  = strdup(v.type&LINK?*v.sp:v.s);
		else if(type==(LINK|STRING)) *sp = strdup(v.type&LINK?*v.sp:v.s);
		else if((type&0xf)==DOUBLE) {	// The only 64bit value
			if(type&LINK) *dp = v.type&LINK? *v.dp: v.d;
			else            d = v.type&LINK? *v.dp: v.d;
		}
		else if(isNumber()) { // 32bit values
			if(type&LINK) *up = v.type&LINK? *v.up: v.u;
			else            u = v.type&LINK? *v.up: v.u;
		}
		else if(isBoolean()) { // 8bit values
			if(type&LINK) *bp = v.type&LINK? *v.bp: v.u;
			else            u = v.type&LINK? *v.bp: v.u;
		}
		// Functions also reference counted
		else if(isFunction()) {
			if(func && --func->ref==0) delete func;
			func = v.func;
			++func->ref;
		}
	} else if(~type&CONST) {
		bool k = v.type&LINK;
		switch(v.type&0xf) {
		case BOOL: setValue(k?*v.bp:v.i); break;
		case INT:  setValue(k?*v.ip:v.i); break;
		case UINT: setValue(k?*v.up:v.u); break;
		case FLOAT: setValue(k?*v.fp:v.f); break;
		case DOUBLE: setValue(k?*v.dp:v.d); break;
		default: printf("Unsupported type conversion\n");
		}
		// ToDo - object conversions?
		
	}
	
	// Keep some flags
	if(v.type & EXPLICIT) type |= EXPLICIT;

	return *this;
}

Variable Variable::copy(uint depth) const {
	if(depth==0) return *this;
	if(isObject()) {
		Variable out;
		out.makeObject();
		for(auto& i: *this) out.set(i.id, i.value.copy(depth-1));
		return out;
	}
	if(isArray()) {
		Variable out;
		out.makeArray();
		for(auto& i: *this) out.set(i.id, i.value.copy(depth-1));
		return out;
	}
	return *this;
}

//// Get values ////
template<typename T> T Variable::getValue() const {
	switch(type&0x9f) {
	case BOOL:   return i!=0;
	case INT:    return i;
	case UINT:   return u;
	case FLOAT:  return f;
	case DOUBLE: return d;
	case STRING: return s&&s[0];
	case LINK|BOOL:   return *bp;
	case LINK|INT:    return *ip;
	case LINK|UINT:   return *up;
	case LINK|FLOAT:  return *fp;
	case LINK|DOUBLE: return *dp;
	case LINK|STRING: return *sp && (*sp)[0];
	default: return 0;
	}
}
Variable::operator bool()   const { return isObject() || isArray() || isVector() || getValue<bool>(); }
Variable::operator int()    const { return getValue<int>(); }
Variable::operator uint()   const { return getValue<uint>(); }
Variable::operator float()  const { return getValue<float>(); }
Variable::operator double() const { return getValue<double>(); }
Variable::operator vec2() const {
	if(isObject() || isVector()) return vec2( get("x"), get("y") );
	if(isNumber()) return vec2( getValue<float>() );
	else return vec2();
}
Variable::operator vec3() const {
	if(isObject() || isVector()) return vec3( get("x"), get("y"), get("z") );
	if(isNumber()) return vec3( getValue<float>() );
	else return vec3();
}
Variable::operator vec4() const {
	if(isObject() || isVector()) return vec4( get("x"), get("y"), get("z"), get("w") );
	if(isNumber()) return vec4( getValue<float>() );
	else return vec4();
}
Variable::operator String() const { return toString(); }
Variable::operator const char*() const {
	if(!isString()) return 0;
	else return type&LINK? *sp: s;
}
Variable::operator Function*() const { return isFunction()? func: 0; }

//// Set value ////
bool Variable::setType(uint t) {
	if((type&0xf)==(t&0xf)) return true; // Already correct type
	if(type & (FIXED|CONST|LINK)) return false; // Cant change type

	// Delete old data
	if((isObject() || isVector() || isArray()) && --obj->ref==0) delete obj;
	if(isFunction() && func && --func->ref==0) delete func;
	if(type == STRING && s) free((char*)s);
	type = t;

	// Initialise new data
	s = 0;
	if(isObject() || isVector() || isArray()) { obj = new Object; obj->ref=1; }
	if(isVector()) {
		_set( lookupName("x"), nullVar );
		_set( lookupName("y"), nullVar );
		if(t>=VEC3) _set( lookupName("z"), nullVar);
		if(t==VEC4) _set( lookupName("w"), nullVar);
	}
	return true;
}

template<typename T> bool Variable::setValue(const T& value) {
	if(isConst()) return false;
	switch(type&0xdf) {
	case BOOL:   i = value!=0; return true;
	case INT:    i = value; return true;
	case UINT:   u = value; return true;
	case FLOAT:  f = value; return true;
	case DOUBLE: d = value; return true;
	case LINK|BOOL:   *bp = value!=0; return true;
	case LINK|INT:    *ip = value; return true;
	case LINK|UINT:   *up = value; return true;
	case LINK|FLOAT:  *fp = value; return true;
	case LINK|DOUBLE: *dp = value; return true;
	default: return false;
	}
}
bool   Variable::operator=(bool v)   { setType(BOOL);   setValue(v); return v; }
int    Variable::operator=(int v)    { setType(INT);    setValue(v); return v; }
uint   Variable::operator=(uint v)   { setType(UINT);   setValue(v); return v; }
float  Variable::operator=(float v)  { setType(FLOAT);  setValue(v); return v; }
double Variable::operator=(double v) { setType(DOUBLE); setValue(v); return v; }
const char* Variable::operator=(const String& v) { *this = (const char*)v; return v;  }
const char* Variable::operator=(const char* v) {
	setType(STRING);
	if(type==STRING) {
		if(s) free((void*)s);
		s = strdup(v? v: "");
	} else if(type==(LINK|STRING)) {
		if(*sp) free((void*)*sp);
		*sp = v? strdup(v): "";
	}
	return v;
}
const vec2& Variable::operator=(const vec2& v) {
	if(setType(VEC2) || (type&0xf)==VEC3 || (type&0xf)==VEC4) {
		get("x") = v.x;
		get("y") = v.y;
	}
	return v;
}
const vec3& Variable::operator=(const vec3& v) {
	if(setType(VEC3) || (type&0xff)==VEC4) {
		get("x") = v.x;
		get("y") = v.y;
		get("z") = v.z;
	}
	return v;
}
const vec4& Variable::operator=(const vec4& v) {
	if(setType(VEC4)) {
		get("x") = v.x;
		get("y") = v.y;
		get("z") = v.z;
		get("w") = v.w;
	}
	return v;
}
Function* Variable::operator=(Function* f) {
	if(setType(FUNCTION)) {
		func = f;
		++f->ref;
	}
	return f;
}

//// LINKING ////

template<typename T> bool Variable::linkValue(uint t, T*& p, T& v, int f) {
	unlink();
	T currentValue = *this;
	if(setType(t)) {
		type |= LINK;
		p = &v;
		if(f&LINK_READONLY) type |= CONST;
		if(f&LINK_SET) v = currentValue;
		return true;
	} else return false;
}
inline bool Variable::linkVector(uint type, float* v, int f) {
	unlink();
	if(setType(type)) {
		static const char* c[4] = {"x", "y", "z", "w" };
		int n = type - (int)VEC2 + 2;
		for(int i=0; i<n; ++i) get(c[i]).link(v[i], f);
		return true;
	}
	else return false;
}

bool Variable::link(bool& v, int f)   { return linkValue(BOOL,   bp, v, f); }
bool Variable::link(int& v, int f)    { return linkValue(INT,    ip, v, f); }
bool Variable::link(uint& v, int f)   { return linkValue(UINT,   up, v, f); }
bool Variable::link(float& v, int f)  { return linkValue(FLOAT,  fp, v, f); }
bool Variable::link(double& v, int f) { return linkValue(DOUBLE, dp, v, f); }
bool Variable::link(const char*& v, int f) { return linkValue(STRING, sp, v, f); }
bool Variable::link(vec2& v, int f)   { return linkVector(VEC2, v, f); }
bool Variable::link(vec3& v, int f)   { return linkVector(VEC3, v, f); }
bool Variable::link(vec4& v, int f)   { return linkVector(VEC4, v, f); }



void Variable::unlink() {
	if(type&LINK) {
		switch(type&0xf) {
		case BOOL:   i = *bp?1:0; break;
		case INT:    i = *ip; break;
		case UINT:   u = *up; break;
		case FLOAT:  f = *fp; break;
		case DOUBLE: d = *dp; break;
		case VEC2:   get("x").unlink(); get("y").unlink(); break;
		case VEC3:   get("x").unlink(); get("y").unlink(); get("z").unlink(); break;
		case VEC4:   get("x").unlink(); get("y").unlink(); get("z").unlink(); get("w").unlink(); break;
		default: break;
		}
		type &= ~LINK;
		type |= FIXED;
	}
}

bool Variable::isLinked() const {
	return type & LINK;
}
bool Variable::isExplicit() const {
	return type & EXPLICIT;
}

void Variable::setCallback(const VariableCallback& c) {
	callback = c;
	// Vector types need the callback on child elements
	switch(type&0xf) {
	case VEC4: get("w").setCallback(c);
	case VEC3: get("z").setCallback(c);
	case VEC2: get("y").setCallback(c);
	           get("x").setCallback(c);
			   break;
	}
}
void Variable::fireCallback() {
	if(callback) callback(*this);
}

bool Variable::operator==(const Variable& v) const {
	if((v.type&0xf) != (type&0xf)) return false;
	switch(type&0xf) {
	case OBJECT: return v.obj == obj;
	case ARRAY: return v.obj == obj;
	case VEC2: return operator vec2() == v;
	case VEC3: return operator vec3() == v;
	case VEC4: return operator vec4() == v;
	case BOOL: return getValue<bool>() == (bool)v;
	case INT: return getValue<int>() == (int)v;
	case UINT: return getValue<uint>() == (uint)v;
	case FLOAT: return getValue<float>() == (float)v;
	case DOUBLE: return getValue<double>() == (double)v;
	case STRING: return strcmp((const char*)*this, (const char*)v)==0;
	case FUNCTION: return func == v.func;
	default: return false;
	}
}
bool Variable::operator!=(const Variable& v) const { return !operator==(v); }

bool Variable::operator==(const char* string) const {
	if(!isString()) return false;
	const char* s = *this;
	if(s == string) return true;
	if(!s || !string) return false;
	return strcmp(string, *this) == 0;
}
bool Variable::operator!=(const char* string) const {
	return !operator==(string);
}



//// Utilities ////
bool Variable::isObject() const { return (type&0xf)==OBJECT; }
bool Variable::isArray() const { return (type&0xf)==ARRAY; }
bool Variable::isVector() const { return (type&0xf)==VEC2 || (type&0xf)==VEC3 || (type&0xf)==VEC4; }
bool Variable::isNumber() const { return (type&0xf)>=INT && (type&0xf)<=DOUBLE; }
bool Variable::isBoolean() const { return (type&0xf)==BOOL; }
bool Variable::isString() const { return (type&0xf)==STRING; }
bool Variable::isFunction() const { return (type&0xf)==FUNCTION; }
bool Variable::isConst() const { return type&CONST; }
bool Variable::isNull() const { return (type&0x1f)==0; }
bool Variable::isRef() const { return type&LINK; }



// ------------------------------------------------------------- //

inline int Variable::_contains(uint id) const {
	return id<obj->lookup.size() &&  obj->lookup[id]!=0xff;
}
inline void Variable::_erase(uint id) {
	if(!_contains(id)) return;
	uint index = obj->lookup[id];
	obj->lookup[id] = 0xff;
	if(index < obj->keys.size() - 1) {
		obj->lookup[ obj->keys.back() ] = index;
		obj->items[index] = std::move(obj->items.back());
		obj->keys[index] = obj->keys.back();
	}
	obj->items.pop_back();
	obj->keys.pop_back();
}
inline void Variable::_eraseArray(uint index, bool keepOrder) {
	if(isArray() && index<obj->items.size()) {
		if(keepOrder) obj->items.erase(obj->items.begin() + index);
		else {
			obj->items[index] = obj->items.back();
			obj->items.pop_back();
		}
	}
}
inline Variable& Variable::_set(uint id, const Variable& var) {
	assert(obj->keys.size()<0xfe); // Limit number of child variables as lookup uses uint8
	// resize lookup array
	if(id >= obj->lookup.size())
		obj->lookup.resize(id+1, 0xff);
	// Update existing item
	int index = obj->lookup[id];
	if(index!=0xff) {
		obj->items[index] = var;
		return obj->items[index];
	}
	// Add item
	obj->lookup[id] = obj->items.size();
	obj->keys.push_back(id);
	// Vector resize breaks linked variables as that is lost when using operator=
	// So here is a horrible solution to resize the vector using memcpy() instead
	// Vector uses operator=() so not really a way around this - Actually, move semantics can work
	if(!obj->items.empty() && obj->items.size() == obj->items.capacity()) {
		std::vector<Variable> temp;
		temp.swap(obj->items);
		obj->items.resize(temp.size(), nullVar);
		obj->items.push_back(var);
		memcpy(&obj->items[0], &temp[0], temp.size() * sizeof(Variable)); // May throw warnings - ignore them
		memset(&temp[0], 0, temp.size() * sizeof(Variable)); // make sure reference counters dont screw up
	}
	else obj->items.push_back(var);
	return obj->items.back();
}

// --------------------- //

int Variable::getReferenceCount() const {
	if(isObject() || isVector()) return obj->ref;
	return 1;
}

int Variable::size() const {
	switch(type&0x1f) {
	case OBJECT: 
	case ARRAY:return obj->items.size();
	case VEC2: return 2;
	case VEC3: return 3;
	case VEC4: return 4;
	case STRING: return strlen(type&LINK? *sp: s);
	default: return 0;
	}
}

void Variable::erase(const char* v) { if(isObject() && !(type&FIXED)) _erase( lookupName_const(v) ); }
void Variable::erase(uint id)       { if(isObject() && !(type&FIXED)) _erase(id); else if(isArray()) _eraseArray(id, true); }
int Variable::contains(const char* name) const { return isObject() || isVector()? _contains( lookupName_const(name) ): 0; }
int Variable::contains(uint id) const          { return isObject() || isVector()? _contains( id ): isArray()? id<obj->items.size(): 0; }
int Variable::contains(const VariableName& name) const { return get_const(name).isValid(); }

Variable& Variable::get_const(const char* name) const  { return isArray()? nullVar: get( lookupName_const(name) ); }
Variable& Variable::get(const char* name) const        { return isArray()? nullVar: get( lookupName_const(name) ); }
Variable& Variable::get(const char* name)              { return isArray()? nullVar: get( lookupName(name) ); }
Variable& Variable::get_const(uint id) const           { return get( id ); }
Variable& Variable::find_const(const char* name) const { return find(name); }
Variable& Variable::get_const(const VariableName& name) const { return get(name); }

//Variable& Variable::get_const(const Variable& v) const { return isArray() && v.isNumber()? get((int)v): get((const char*)v); }
//Variable& Variable::get(const Variable& v) { return isArray() && v.isNumber()? get((int)v): get((const char*)v); }

Variable& Variable::get(uint id) const {
	if(!contains(id)) return nullVar;
	if(isArray()) return obj->items[id];
	return obj->items[ obj->lookup[id] ];
}
Variable& Variable::get(uint id) {
	if(isArray()) {
		if(id>=obj->items.size()) obj->items.resize(id+1);
		return obj->items[id];
	}
	else if(contains(id)) return obj->items[ obj->lookup[id] ];
	else if(setType(OBJECT)) return _set(id, Variable());
	else return nullVar;
}


static uint part(const char* s, const char*& e, bool add) {
	char tmp[NAME_LIMIT]; // need a copy so we can null terminate a substring
	e = strchr(s, '.');
	if(e) {
		size_t length = e - s;
		assert(length < NAME_LIMIT);
		memcpy(tmp, s, length);
		tmp[length] = 0;
		s = tmp;
	}
	else {
		static const char end = 0;
		e = &end;
	}
	if(add) return Variable::lookupName(s);
	else return Variable::lookupName_const(s);
}

Variable& Variable::find(const char* name) const {
	if(!isObject() && !isVector()) return nullVar;
	if(!name || !name[0]) return nullVar;
	const char* end;
	uint id = part(name, end, false);
	if(end==name) return nullVar; // invalid name
	Variable& var = get(id);
	if(var.isValid() && *end=='.') return var.find_const(end + 1);
	else return var;
}
Variable& Variable::find(const char* name) {
	if(isArray()) return nullVar;
	const char* end;
	uint id = part(name, end, true);
	if(end==name) return nullVar; // invalid name
	if((type&EXPLICIT) && *end=='.' && !contains(id)) return nullVar;
	Variable& var = get(id);
	if(var.isValid() && *end=='.') return var.find(end + 1);
	else return var;
}

Variable& Variable::get(const VariableName& name) {
	if(name.parts.empty()) return nullVar;
	Variable* var = this;
	for(uint i: name.parts) {
		if(var->type & EXPLICIT) var = &var->get_const(i);
		else var = &var->get(i);
	}
	return *var;
}
Variable& Variable::get(const VariableName& name) const {
	if(name.parts.empty()) return nullVar;
	Variable* var = &get(name.parts[0]);
	for(uint i=1; i<name.parts.size(); ++i) var = &var->get_const(name.parts[i]);
	return *var;
}

// ------------------------------------------------------------- //


bool Variable::makeObject() {
	// Convert array to object - keep items
	if(isArray() && !obj->items.empty() && !(type&(FIXED|CONST))) {
		Variable tmp = *this;
		if(!setType(OBJECT)) return false;
		char buffer[4];
		for(uint i=0; i<obj->items.size(); ++i) {
			snprintf(buffer, 4, "%u", i);
			set(buffer, tmp.obj->items[i]);
		}
		return true;
	}
	return setType(OBJECT);
}
bool Variable::makeArray(int initialSize) {
	// Convert object into an array - order may vary
	if(isObject() && !obj->items.empty() && !(type&(FIXED|CONST))) {
		obj->lookup.clear();
		obj->keys.clear();
		type = ARRAY;
	}
	if(!setType(ARRAY)) return false;
	if(size() < initialSize) obj->items.resize(initialSize);
	return true;
}

void Variable::lock() {
	type |= CONST;
}
void Variable::setExplicit(bool e) {
	if(e) type |= EXPLICIT;
	else type &= ~EXPLICIT;
}

String Variable::toString(int depth, bool quotes, bool multiLine, int indent) const {
	char buffer[512];
	const char* b = buffer;
	switch(type & ~(FIXED|CONST|EXPLICIT)) {
	case 0: sprintf(buffer, "null"); break;	// Null variable
	case OBJECT:
		if(obj->items.empty()) b = "{}";
		else if(depth) {
			// Sort output
			std::vector<const char*> tmp;
			for(auto& i: *this) tmp.push_back(i.key);
			std::sort(tmp.begin(), tmp.end(), [](const char* a, const char* b) { return strcmp(a,b) < 0; });

			String s("{");
			for(unsigned i=0; i<tmp.size(); ++i) {
				if(multiLine) sprintf(buffer, "\n%*s%s = ", 2*indent+2, "", tmp[i]);
				else snprintf(buffer, 512, " %s = ", tmp[i]);
				s = s + buffer + get_const(tmp[i]).toString(depth-1, true, multiLine, indent+1);
			}
			if(tmp.empty()) s = s + "}";
			else if(!multiLine) s = s + " }";
			else snprintf(buffer, 512, "\n%*s}", 2*indent, ""), s = s + buffer;
			return s;
		} else b = "{...}";
		break;
	case ARRAY:
		if(obj->items.empty()) b = "[]";
		else if(depth) {
			String s("[");
			for(const Variable& v: obj->items) s = s + v.toString(depth-1, true, false) + ", ";
			int len = s.length();
			s[len-2]=']', s[len-1]=0;
			return s;
		}
		else b = "[...]";
		break;
	case FUNCTION:
		if(func) return func->toString(false);
		else b = "";
		break;
	case BOOL:   b = i? "true": "false";   break;
	case INT:    sprintf(buffer, "%d", i); break;
	case UINT:   sprintf(buffer, "%#x", u); break;
	case FLOAT:  sprintf(buffer, "%g", f); break;
	case DOUBLE: sprintf(buffer, "%g", d); break;
	case STRING: if(!quotes) b=s; else return String("\"") + s + "\""; break;

	case LINK|BOOL:   b = *bp? "true": "false";   break;
	case LINK|INT:    sprintf(buffer, "%d", *ip); break;
	case LINK|UINT:   sprintf(buffer, "%#x", *up); break;
	case LINK|FLOAT:  sprintf(buffer, "%g", *fp); break;
	case LINK|DOUBLE: sprintf(buffer, "%g", *dp); break;
	case LINK|STRING: if(!quotes) b=*sp; else return String("\"") + *sp + "\""; break;

	case VEC2:
	case LINK|VEC2:   sprintf(buffer, "(%g, %g)", (float)get("x"), (float)get("y")); break;
	case VEC3:
	case LINK|VEC3:   sprintf(buffer, "(%g, %g, %g)", (float)get("x"), (float)get("y"), (float)get("z")); break;
	case VEC4:
	case LINK|VEC4:   sprintf(buffer, "(%g, %g, %g, %g)", (float)get("x"), (float)get("y"), (float)get("z"), (float)get("w")); break;
	default: return String();
	}
	return String(b);
}


Variable::iterator Variable::begin() const {
	if(isObject() || isVector() || isArray()) return iterator(&obj->keys[0], &obj->items[0], isArray());
	else return iterator(0,0,false);
}
Variable::iterator Variable::end() const {
	if(isObject() || isVector() || isArray()) {
		size_t s = obj->items.size();
		return iterator(&obj->keys[0] + s, &obj->items[0] + s, isArray());
	}
	else return iterator(0,0,false);
}

