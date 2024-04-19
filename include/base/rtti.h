#pragma once

// Simple RTTI implementation
//  add RTTI_BASE(ClassName) to the root class
//  and RTTI_DERIVED(Classname) to any derived classes.
// This does NOT support multiple inheritance

#include <typeinfo>

#define RTTI_TYPE(name) \
	static int staticType() { static int cid = typeid(name).hash_code(); return cid; } \
	static const char* staticName() { return #name; } \

#define RTTI_BASE(name) public: RTTI_TYPE(name); \
	typedef name ThisType; \
	virtual bool isType(int t) const { return t==staticType(); } \
	virtual int getType() const { return staticType(); } \
	virtual const char* getTypeName() const { return staticName(); } \
	template<class T> T* as() { return isType(T::staticType())? static_cast<T*>(this): nullptr; } \
	template<class T> const T* as() const { return isType(T::staticType())? static_cast<const T*>(this): nullptr; } \

#define RTTI_DERIVED(name) public: RTTI_TYPE(name); \
	typedef ThisType Super; \
	typedef name ThisType; \
	virtual bool isType(int t) const override { return t==staticType() || Super::isType(t); }; \
	virtual int getType() const override { return staticType(); } \
	virtual const char* getTypeName() const override { return staticName(); } \


template<class R, class T> const R* cast(const T* object) {
	return object && object->isType(R::staticType())? static_cast<const R*>(object): nullptr;
}
template<class R, class T> R* cast(T* object) {
	return object && object->isType(R::staticType())? static_cast<R*>(object): nullptr;
}



