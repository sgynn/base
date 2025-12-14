/* New delegate class for c++11. Uses varadic macros and supports capture lambdas.
 * Example use:
 * 	typedef Delegate<bool (int)> MyDelegate;
 * 	Mydelegate callback = MyDelegate( this, myFunction );
 * 	bool r = callback.call(3);
 * */

/* Thinks
Two types od delegate, direct and indirect.
Direct delegates call a function with the same signiture,
Indirect can have predefined parameters and references arguments.
Can convert a direct delegate into an indirect one ?
Direct delegates are an optimisation as they don't need to go through the argument list

static bind() functions are a problem with indirect delegates as we don't know the calling signiture.
*/


#pragma once

template <typename T> class Delegate {};
template <typename T> class MultiDelegate {};

/// Argument referencing custom callbacks
template<int N> struct Arg {};

/** Alternative delegate system - nicer syntax but slower */
template<typename R, typename ...A>
class Delegate<R(A...)> {
	struct Base { virtual R call(A...) = 0; virtual ~Base() {} };

	template<class T>
	struct Static : public Base {
		T func;
		R call(A...args) override { return func(args...); }
		Static(const T& f) : func(f) {}
	};

	template<class T>
	struct Member : public Base {
		T* instance;
		R(T::*func)(A...);
		R call(A...args) override { return (instance->*func)(args...); }
	};

	// Allow defining static arguments - this is getting ridiculous
	template<class...V> struct Tuple {
		template<class F, class...K>
		R call(F& func, A...args, K...values) { return func(values...); }
		template<class T, class F, class...K>
		R callM(T* inst, F& func, A...args, K...values) { return (inst->*func)(values...); }
	};
	template<class T, class...V> struct Tuple<T,V...> : public Tuple<V...> {
		T value;
		Tuple(T v, V...s) : Tuple<V...>(s...), value(v) {};
		template<class F, class...K>
		R call(F& func, A...args, K...values) { return Tuple<V...>::call(func, args..., values..., value); }
		template<class C, class F, class...K>
		R callM(C* inst, F& func, A...args, K...values) { return Tuple<V...>::callM(inst, func, args..., values..., value); }
	};

	// More insanity - this should get whatever Arg<N> references
	template<int N, class...V> struct GetN;
	template<int N, class T, class...V> struct GetN<N,T,V...> { static auto get(T, V...v) { return GetN<N-1, V...>::get(v...); } };
	template<class T, class...V> struct GetN<0,T,V...> { static T get(T v, V...) { return v; } };
	template<int N, class...V> struct Tuple<Arg<N>,V...> : public Tuple<V...> {
		Tuple(Arg<N> v, V...s) : Tuple<V...>(s...) {};
		template<class F, class...K>
		R call(F& func, A...args, K...values) { return Tuple<V...>::call(func, args..., values..., GetN<N,A...>::get(args...)); }
		template<class C, class F, class...K>
		R callM(C* inst, F& func, A...args, K...values) { return Tuple<V...>::callM(inst, func, args..., values..., GetN<N,A...>::get(args...)); }
	};

	template<class F, class...V>
	struct ArgWrapper : public Base {
		F func;
		Tuple<V...> values;
		R call(A...args) override { return values.call(func, args...); }
		ArgWrapper(const F& f, V...v) : func(f), values(v...) {}
	};
	template<class C, class F, class...V>
	struct ArgWrapperM : public Base {
		C* instance;
		F func;
		Tuple<V...> values;
		R call(A...args) override { return values.callM(instance, func, args...); }
		ArgWrapperM(C* inst, const F& f, V...v) : instance(inst), func(f), values(v...) {}
	};



	Base* delegate;
	int*  ref;
	void drop() { if(ref && --*ref==0) { delete delegate; delete ref; ref=0; } }

	public:
	Delegate(): delegate(0), ref(0) {}
	~Delegate() { drop(); }
	Delegate(const Delegate& d) : delegate(d.delegate), ref(d.ref) { if(ref) ++*ref; }
	Delegate(int i) : Delegate() {}
	Delegate(R(*func)(A...)) : delegate(0), ref(0) { bind(func); }
	template<class T> Delegate(const T& f) : delegate(0), ref(0) { bind(f); }
	const Delegate& operator=(const Delegate& d) {
		if(this==&d) return *this;
		drop();
		delegate = d.delegate;
		ref = d.ref;
		if(ref) ++*ref;
		return *this;
	}
	void operator=( R(*func)(A...) ) { bind(func); }
	void operator=(int) { unbind(); }
	template<class T> void operator=(const T& f) { bind(f); }
	
	R call(A...args) const { return delegate->call(args...); }
	R operator()(A...args) const { return delegate->call(args...); }

	/// Static functions, functors and lambdas
	template<typename T>
	void bind(const T& func) {
		drop();
		Static<T>* d = new Static<T>(func);
		delegate = d;
		ref = new int(1);
	}
	// Override this one as it fails to compile with above template version
	void bind(R(*value)(A...)) {
		drop();
		if(value) {
			Static<R(*)(A...)>* d = new Static<R(*)(A...)>(value);
			delegate = d;
			ref = new int(1);
		}
	}

	/// Member functions
	template<class T> void bind(T* inst, R(T::*func)(A...)) {
		drop();
		Member<T>* d = new Member<T>();
		d->func = func;
		d->instance = inst;
		delegate = d;
		ref = new int(1);
	}

	// Custom args
	template<class Func, class...CustomArgs>
	void bind(const Func& func, CustomArgs...args) {
		drop();
		ArgWrapper<Func, CustomArgs...>* d = new ArgWrapper<Func, CustomArgs...>(func, args...);
		delegate = d;
		ref = new int(1);
	}
	template<class T, class...CA, class...V> void bind(T* inst, R(T::*func)(CA...), V...args) {
		drop();
		ArgWrapperM<T, R(T::*)(CA...), V...>* d = new ArgWrapperM<T, R(T::*)(CA...), V...>(inst, func, args...);
		delegate = d;
		ref = new int(1);
	}


	void unbind() {
		drop();
		delegate = 0;
		ref = 0;
	}

	/// Is this delegate bound
	operator bool() const { return isBound(); }

	/// To test if delegate is bound
	bool isBound() const { return ref!=0; }
};


template<typename R, typename...A>
inline Delegate<R(A...)> bind( R(*value)(A...)) {
	Delegate<R(A...)> func;
	func.bind(value);
	return func;
}
template<class C, typename R, typename...A>
inline Delegate<R(A...)> bind(C* c, R(C::*value)(A...)) {
	Delegate<R(A...)> func;
	func.bind(c, value);
	return func;
}

// ToDo: bind functions for custom argument versions
// Return delegate type is uncertain. need support for
// Delegate::operator= with different arg setups if using custom arg versions.


// ========================================================================== //

template<typename R, typename...A>
class MultiDelegate<R(A...)> {
	public:
	typedef Delegate<R(A...)> DelegateType;
	MultiDelegate(): list(nullptr), size(0), capacity(0) {}
	~MultiDelegate() { delete [] list; }

	template<class F>
	void bind(const F& func) {
		allocate().bind(func);
	}
	template<class T> void bind(T* inst, R(T::*func)(A...)) {
		allocate().bind(inst, func);
	}
	template<class F, class...V> void bind(const F& func, V...args) {
		allocate().bind(func, args...);
	}
	template<class T, class...CA, class...V> void bind(T* inst, R(T::*func)(CA...), V...args) {
		allocate().bind(inst, func, args...);
	}

	MultiDelegate& operator+=(R(*func)(A...)) { bind(func); return *this; }
	MultiDelegate& operator+=(DelegateType& d) { allocate() = d; }

	void unbindAll() {
		for(int i=0; i<size; ++i) list[i].unbind();
		size=0;
	}

	// TODO: Unbind functions - need delegate comparison

	R call(A...args) const { 
		for(int i=0; i<size-1; ++i) list[i].call(args...);
		return list[size-1].call(args...);	// Return value of last one called
	}
	R operator()(A...args) const { return call(args...); }

	DelegateType* begin() const { return list; }
	DelegateType* end() const { return list + size; }
	int isBound() const { return size; }
	operator bool() const { return size; }

	protected:
	DelegateType* list;
	int size, capacity;
	DelegateType& allocate() {
		if(capacity>size) return list[size++];
		capacity = capacity? capacity+2: 1;
		DelegateType* old = list;
		list = new DelegateType[capacity];
		for(int i=0; i<size; ++i) list[i] = old[i];
		delete [] old;
		return list[size++];
	}
	void erase(int index) {
		for(int i=index+1; i<size; ++i) list[i-1] = list[i];
		list[size-1].unbind();
	}
};

