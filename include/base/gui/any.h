#pragma once

#include <typeinfo>

namespace gui {

// Allow overriding outside function
template<class T> bool equal(const T& a, const T& b) { return a==b; }

/** Class for holding generic types */
class Any {
	public:
	Any();
	Any(const Any& o);
	Any(Any&& move) noexcept;
	~Any();

	template<typename T>
	explicit Any(const T& value);

	template<typename T>
	Any& operator=(const T& value);

	Any& operator=(const Any& value) {
		if(this==&value) return *this;
		if(m_value) delete m_value;
		m_value = value.m_value? value.m_value->clone(): 0;
		return *this;
	}
	Any& operator=(Any&& value) noexcept {
		AnyValue* tmp = m_value;
		m_value = value.m_value;
		value.m_value = tmp;
		return *this;
	}

	// Three different ways of getting the value
	template<typename T> T* cast() const;			// Return value pointer. Returns null if incorrect type
	template<typename T> bool read(T& value) const;	// Sets value parameter. Returns false if incorrect type
	template<typename T> const T& getValue(const T& defaultValue) const;	// Returns the value, or default if incorrect type
	template<typename T, typename E> bool convert(E& value) const;
	template<typename T> bool isType() const;
	template<typename T> bool isType(const T&) const;
	const std::type_info& getType() const;
	
	bool isNull() const;
	void setNull();

	public:
	template<typename T> bool operator==(const T& v) const;
	template<typename T> bool operator!=(const T& v) const;

	private:
	struct AnyValue {
		virtual AnyValue* clone() const = 0;
		virtual const std::type_info& getType() const = 0;
		virtual ~AnyValue() {}
		virtual bool compare(const Any&) = 0;
	};
	template<typename V>
	struct AnyValueT : public AnyValue {
		AnyValueT(const V& value) : m_value(value) {}
		virtual AnyValue* clone() const { return new AnyValueT(m_value); }
		virtual const std::type_info& getType() const  { return typeid(V); }
		virtual bool compare(const Any& v) { V* o = v.cast<V>(); return o && equal(*o, m_value); } 
		V m_value;
	};
	AnyValue* m_value;
};

// Implementations
inline Any::Any() : m_value(0) {}
inline Any::Any(const Any& a) { m_value = a.m_value? a.m_value->clone(): 0; }
inline Any::Any(Any&& a) noexcept{ m_value = a.m_value; a.m_value = 0; }
inline Any::~Any() { if(m_value) delete m_value; }

template<typename T> Any::Any(const T& v) { m_value = new Any::AnyValueT<T>(v); }
template<typename T> Any& Any::operator=(const T& v) {
	if(m_value) delete m_value;
	m_value = new Any::AnyValueT<T>(v);
	return *this;
}
template<typename T> T* Any::cast() const {
	if(!m_value || m_value->getType() != typeid(T)) return 0;
	return &static_cast<Any::AnyValueT<T>*>(m_value)->m_value;
}
template<typename T> bool Any::read(T& value) const {
	if(!m_value || m_value->getType() != typeid(T)) return false;
	value = static_cast<Any::AnyValueT<T>*>(m_value)->m_value;
	return true;
}
template<typename T> const T& Any::getValue(const T& defaultValue) const {
	if(!m_value || m_value->getType() != typeid(T)) return defaultValue;
	return static_cast<const Any::AnyValueT<T>*>(m_value)->m_value;
}

template<typename T, typename E> bool Any::convert(E& value) const {
	T tmp;
	if(!read(tmp)) return false;
	value = tmp; // intrinsic conversion
	return true;
}


inline bool Any::isNull() const { return m_value==0; }
inline void Any::setNull() {
	if(m_value) delete m_value;
	m_value = 0;
}

template<typename T> bool Any::isType(const T&) const { return isType<T>(); }
template<typename T> bool Any::isType() const { 
	return m_value && m_value->getType() == typeid(T);
}
inline const std::type_info& Any::getType() const {
	return m_value? m_value->getType(): typeid(void);
}


template<typename T> bool Any::operator==(const T& v) const { T* t=cast<T>(); return t && equal(*t, v); }
template<typename T> bool Any::operator!=(const T& v) const { T* t=cast<T>(); return !t || !equal(*t, v); }
template<typename T> bool operator==(const T& v, const Any& a) { return a == v; }
template<typename T> bool operator!=(const T& v, const Any& a) { return a != v; }
template<> inline bool Any::operator==(const Any& v) const { return m_value->compare(v); }
template<> inline bool Any::operator!=(const Any& v) const { return !m_value->compare(v); }
inline bool operator==(const Any& v, const Any& a) { return a.operator==(v); }
inline bool operator!=(const Any& v, const Any& a) { return a.operator!=(v); }

}

