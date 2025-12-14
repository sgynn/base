#pragma once

#include <base/hashmap.h>
#include <base/assert.h>
#include <vector>

namespace base {
class VirtualFileSystem;

class ResourceManagerBase {
	public:
	virtual ~ResourceManagerBase();
	void  error(const char* message, const char* name) const; /// error message
};

struct ResourceLoadProgress { unsigned completed; unsigned remaining; };
enum class ResourceState { Invalid, Unloaded, Loaded, Loading, Failed };

template<class T> class ResourceManager;
template<class T>
class ResourceLoader {
	public:
	typedef ResourceManager<T> Manager;
	virtual T* create(const char* name, Manager* manager) = 0;
	virtual bool reload(const char* name, T* object, Manager* manager) { return false; }
	virtual void destroy(T* item) = 0;
	virtual ResourceLoadProgress update() { return {0,0}; }
	virtual bool isBeingLoaded(const T* item) const { return false; }
	virtual void updateT() {}
	virtual ~ResourceLoader() {}
};

template<class T>
class ResourceManager : public ResourceManagerBase {
	public:
	typedef ResourceLoader<T> Loader;
	ResourceManager(Loader* defaultloader=0) : m_defaultLoader(defaultloader) {}
	~ResourceManager()	{ clear(); delete m_defaultLoader; }
	void    setDefaultLoader(Loader*);
	Loader* getDefaultLoader() const;

	T*    get(const char* name);          /// Get a resource. Tries to load it if it is new.
	T*    getIfExists(const char* name);  /// Get resource if it exists
	int   exists(const char* name) const; /// Is a resource in the manager
	bool  reload(const char* name);       /// Reload a resource if it exists and is possible.

	ResourceState  getState(const T* item) const;    /// Get teh state of a resource
	ResourceState  getState(const char* name) const; /// Get the state of a resource

	bool  rename(const char* oldName, const char* newName);
	void  alias(const char* existing, const char* name); /// Create an alias of a resource
	void  add(const char* name, T* item, Loader* loader=0, int ref=1); /// Add resource to manager
	int   drop(const char* name);         /// Drop reference to resource
	int   drop(const T* item);            /// Drop reference to resource
	void  clear();                        /// Delete everything

	const char* getName(const T* item) const; /// Get registered name if a resource. Returns null if not found.

//	T*    create(const char* name) const;   /// Create a resource using default loader. Does not add to manager



	private:
	struct Resource { T* object; int ref; Loader* loader; const char* name; };
	base::HashMap<Resource*> m_resources;
	static const int FAILED = -999;
	Loader* m_defaultLoader;
	bool drop(Resource*, bool force);
	ResourceState getState(const Resource*) const;

	public: // Iterators
	struct IValue { const char* key; T* value; operator T*(){ return value; } };
	class Iterator {
		public:
		typename base::HashMap<Resource*>::const_iterator iter;
		IValue operator*() { return IValue{iter->key, iter->value->object}; }
		Iterator& operator++(int) { ++iter; return *this; }
		Iterator  operator++() { Iterator i=*this; ++iter; return i; }
		bool operator!=(const Iterator& i) const { return iter!=i.iter; }
	};
	Iterator begin() const { return Iterator{m_resources.begin()}; }
	Iterator end() const { return Iterator{m_resources.end()}; }
};

// ----------------------------------------------------------------------------- //

/*
template<class T>
T* ResourceManager<T>::create(const char* name) const {
	if(m_defaultLoader) return m_defaultLoader->create(name, this);
	else return 0;
}*/

template<class T>
ResourceLoader<T>* ResourceManager<T>::getDefaultLoader() const { return m_defaultLoader; }
template<class T>
void ResourceManager<T>::setDefaultLoader(Loader* l) { m_defaultLoader = l; }

template<class T>
ResourceState ResourceManager<T>::getState(const char* name) const {
	if(!name || !name[0]) return ResourceState::Invalid;
	typename base::HashMap<Resource*>::iterator it = m_resources.find(name);
	return it != m_resources.end()? getState(*it): ResourceState::Unloaded;
}

template<class T>
ResourceState ResourceManager<T>::getState(const T* item) const {
	if(!item) return ResourceState::Invalid;
	for(auto i : m_resources) {
		if(i.value->object == item) return getState(i.value);
	}
	return ResourceState::Unloaded;
}

template<class T>
ResourceState ResourceManager<T>::getState(const Resource* resource) const {
	if(!resource) return ResourceState::Unloaded;
	if(resource->ref == FAILED) return ResourceState::Failed;
	if(!resource->object) return ResourceState::Unloaded;
	if(resource->loader && resource->loader->isBeingLoaded(resource->object)) return ResourceState::Loading;
	return ResourceState::Loaded;
}

template<class T>
T* ResourceManager<T>::get(const char* name) {
	if(!name || !name[0]) return 0; // Invalid name
	typename base::HashMap<Resource*>::iterator it = m_resources.find(name);
	if(it != m_resources.end()) {
		Resource& resource = *it->value;
		if(resource.ref == FAILED) return 0;	// failed to load
		++resource.ref;
		// Load now if unloaded
		if(!resource.object && resource.loader) {
			resource.object = resource.loader->create(name, this);
		}
		return resource.object;
	}
	else if(m_defaultLoader) {
		T* resource = m_defaultLoader->create(name, this);
		add(name, resource, m_defaultLoader, resource? 0: FAILED);
		return resource;
	}
	return 0;
}

template<class T>
int ResourceManager<T>::exists(const char* name) const {
	if(!name || !name[0]) return 0; // Invalid name
	typename base::HashMap<Resource*>::const_iterator it = m_resources.find(name);
	return it!=m_resources.end() && it->value->ref != FAILED;
}

template<class T>
T* ResourceManager<T>::getIfExists(const char* name) {
	if(!name || !name[0]) return 0; // Invalid name
	typename base::HashMap<Resource*>::iterator it = m_resources.find(name);
	if(it==m_resources.end() || it->value->ref == FAILED) return 0;
	++it->value->ref;
	return it->value->object;
}

template<class T>
void ResourceManager<T>::add(const char* name, T* resource, Loader* loader, int ref) {
	typename base::HashMap<Resource*>::iterator it = m_resources.find(name);
	// Check if resource exists
	if(it != m_resources.end()) {
		assert(it->value->ref==0 || it->value->ref==FAILED); // Resource already exists and is referenced by something
		drop(it->value, false);
		m_resources.erase(name);
	}

	// Check for alias
	if(resource) {
		for(it=m_resources.begin(); it!=m_resources.end(); ++it) if(it->value->object==resource) break;
		if(it != m_resources.end()) {
			m_resources.insert(name, it->value);
			return;
		}
	}
	// Add new
	Resource* r = new Resource{ resource, ref, loader, 0 };
	m_resources.insert(name, r);
	r->name = m_resources.find(name)->key;
}

template<class T>
bool ResourceManager<T>::rename(const char* oldName, const char* newName) {
	if(m_resources.contains(newName)) return false;
	Resource* resource = m_resources.get(oldName, nullptr);
	if(!resource) return false;
	m_resources[newName] = resource;
	m_resources.erase(oldName);
	return true;
}

template<class T>
bool ResourceManager<T>::drop(Resource* r, bool force) {
	if(r->ref == FAILED || !r->object) return true;
	if(--r->ref<=0 || force) {
		if(r->loader) r->loader->destroy(r->object);
		else if(m_defaultLoader) m_defaultLoader->destroy(r->object);
		else assert(false); // Failed to delete item
		r->object = 0;
		r->ref = 0;
		return true;
	}
	return false;
}

template<class T>
int ResourceManager<T>::drop(const char* name) {
	typename base::HashMap<Resource*>::iterator it = m_resources.find(name);
	if(it == m_resources.end()) return 0;
	if(drop(it->value, false)) return 0;
	return it->value->ref;
}
template<class T>
int ResourceManager<T>::drop(const T* resource) {
	if(!resource) return 0;
	for(auto i : m_resources) {
		if(i.value->object == resource) {
			if(drop(i.value, false)) return 0;
			else return i.value->ref;
		}
	}
	return 0;
}
template<class T>
void ResourceManager<T>::clear() {
	std::vector<Loader*> uniqueLoaders;
	for(auto i : m_resources) {
		drop(i.value, true);
		// Detect aliases - resource may be in the map multiple times
		bool alias = false; 
		for(auto j=++m_resources.find(i.key); j!=m_resources.end(); ++j) {
			if(i.value == j->value) { alias = true; break; }
		}
		if(!alias) drop(i.value, true);

		// Delete any custom loaders
		Loader* loader = i.value->loader;
		if(loader == m_defaultLoader) loader = nullptr;
		else if(loader) for(Loader* l: uniqueLoaders) if(l==loader) { loader=nullptr; break; }
		if(loader) uniqueLoaders.push_back(loader);
	}
	for(Loader* loader: uniqueLoaders) delete loader;
	m_resources.clear();
}

template<class T>
bool ResourceManager<T>::reload(const char* name) {
	typename base::HashMap<Resource*>::iterator it = m_resources.find(name);
	if(it == m_resources.end() || !it->value->loader) return false;
	return it->value->loader->reload(it->key, it->value->object, this);
}

template<class T>
const char* ResourceManager<T>::getName(const T* item) const {
	for(const auto& i: m_resources) {
		if(i.value->object == item) return i.key;
	}
	return 0;
}

}

