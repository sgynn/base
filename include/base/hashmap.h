#ifndef _BASE_HASHMAP_
#define _BASE_HASHMAP_

#include <cstring>
#include <cstdlib>
#include <assert.h>

#define THRESHOLD 1.5f

namespace base {
	
	/** A quick implementation of a hashmap. Keys are c strings. */
	template <typename T>
	class HashMap {
		public:
		struct Pair { const char* const key; T value; };
		HashMap(int cap=8);
		HashMap(const HashMap& m);
		HashMap& operator=(const HashMap&);
		~HashMap();
		bool empty() const { return m_size==0; }
		unsigned int size() const { return m_size; }
		bool contains(const char* key) const;
		T& operator[](const char* key);
		const T& operator[](const char* key) const;
		const T& get(const char* key, const T& fallback) const;
		T& insert(const char* key, const T& value);
		void erase(const char* key);
		void clear();
		bool validate() const;
		/** iterator class. Works the same as stl iterators */
		private:
		template<class Map, class Pair, class PairPtr>
		class t_iterator {
			friend class HashMap;
			public:
			t_iterator() : m_item(0) {}
			t_iterator operator++(int) { t_iterator tmp=*this; next(); return tmp; }
			t_iterator operator++() { next(); return *this; }
			Pair& operator*() { return *(*m_item); }
			Pair* operator->() { return *m_item; }
			bool operator==(const t_iterator& o) const { return m_item==o.m_item; }
			bool operator!=(const t_iterator& o) const { return m_item!=o.m_item; }
			private:
			bool isItem(PairPtr p) const { return m_item>=m_map->m_data+m_map->m_capacity || *m_item; }
			void next() { ++m_item; while(!isItem(m_item)) ++m_item; }
			t_iterator(Map* map, PairPtr item) : m_item(item), m_map(map) { if(!isItem(m_item)) next(); }
			PairPtr m_item;
			Map* m_map;
		};
		friend class t_iterator<HashMap, Pair, Pair**>;
		friend class t_iterator<const HashMap, const Pair, const Pair*const*>;
		public:

		typedef t_iterator<HashMap, Pair, Pair**> iterator;
		typedef t_iterator<const HashMap, const Pair, const Pair*const*> const_iterator;

		iterator       begin()       { return iterator(this, m_data); }
		const_iterator begin() const { return const_iterator(this, m_data); }
		iterator       end()         { return iterator(this, m_data+m_capacity); }
		const_iterator end() const   { return const_iterator(this, m_data+m_capacity); }
		iterator find(const char* key) {
			unsigned int i=index(key, m_capacity);
			return i<m_capacity && m_data[i]? iterator(this, m_data+i): end();
		}
		const_iterator find(const char* key) const {
			unsigned int i=index(key, m_capacity);
			return i<m_capacity && m_data[i]? const_iterator(this, m_data+i): end();
		}
		
		private:
		unsigned int index(const char* c, unsigned int n) const;
		static unsigned int hash(const char* c);
		static bool compare(const char* a, const char* b);
		void resize(unsigned int newSize);
		Pair** m_data;
		unsigned int m_capacity, m_size;
	};
};

template<typename T> base::HashMap<T>::HashMap(int cap) : m_data(0), m_capacity(0), m_size(0) {
	if(cap>0) resize(cap);
}
template<typename T> base::HashMap<T>::HashMap(const HashMap& m) : m_capacity(m.m_capacity), m_size(m.m_size) {
	m_data = new Pair*[ m_capacity ];
	for(unsigned int i=0; i<m_capacity; i++) {
		Pair* item = m.m_data[i];
		if(item) m_data[i] = new Pair{ strdup(item->key), item->value };
		else m_data[i] = 0;
	}
}
template<typename T> base::HashMap<T>& base::HashMap<T>::operator=(const HashMap& m) {
	clear();
	delete [] m_data;
	m_capacity = m.m_capacity;
	m_size = m.m_size;
	m_data = new Pair*[ m_capacity ];
	for(unsigned int i=0; i<m_capacity; i++) {
		Pair* item = m.m_data[i];
		if(item) m_data[i] = new Pair{ strdup(item->key), item->value };
		else m_data[i] = 0;
	}
	return *this;
}
template<typename T> base::HashMap<T>::~HashMap() {
	clear();
	delete [] m_data;
}
template<typename T> bool base::HashMap<T>::contains(const char* key) const {
	unsigned int i = index(key, m_capacity);
	return i<m_capacity && m_data[i];
}
template<typename T> T& base::HashMap<T>::insert(const char* key, const T& value) {
	T& item = (*this)[key];
	item = value;
	return item;
}

template<typename T> const T& base::HashMap<T>::get(const char* key, const T& fallback) const {
	unsigned int i = index(key, m_capacity);
	if(i<m_capacity && m_data[i]) return m_data[i]->value;
	else return fallback;
}
template<typename T> T& base::HashMap<T>::operator[](const char* key) {
	// Check if extsts first to save unnessesary resize
	unsigned int i = index(key, m_capacity);
	if(!m_data[i]) {
		// Do we need to resize array
		if(m_size*THRESHOLD > m_capacity) {
			resize((unsigned int)(m_capacity*THRESHOLD));
			i = index(key, m_capacity); // index will have changed
		}
		// Create new entry
		m_data[i] = new Pair{ strdup(key), T() };
		++m_size;
	}
	validate();
	return m_data[i]->value;
}
template<typename T> const T& base::HashMap<T>::operator[](const char* key) const {
	static T invalidValue;
	return get(key, invalidValue);
}

template<typename T> void base::HashMap<T>::erase(const char* key) {
	unsigned int i = index(key, m_capacity);
	if(i<m_capacity && m_data[i]) {
		// Delete item
		free((void*)m_data[i]->key);
		delete m_data[i];
		m_data[i] = 0;
		// Problem - this gap can break other entries
		unsigned j;
		for(j=i+1; j<m_capacity && m_data[j]; ++j) {
			unsigned t = index(m_data[j]->key, m_capacity);
			if(t!=j) m_data[t] = m_data[j], m_data[j] = 0;
		}
		if(j==m_capacity) for(j=0; j<i && m_data[j]; ++j) {
			unsigned t = index(m_data[j]->key, m_capacity);
			if(t!=j) m_data[t] = m_data[j], m_data[j] = 0;
		}
	}
	validate();
}
template<typename T> void base::HashMap<T>::clear() {
	for(unsigned int i=0; i<m_capacity; i++) {
		if(m_data[i]) free((void*)m_data[i]->key);
		delete m_data[i];
		m_data[i] = 0;
	}
	m_size = 0;
}
template<typename T> unsigned int base::HashMap<T>::hash(const char* c) {
	unsigned int h=0;
	for(unsigned int i=0; c[i]; i++) h = (h<<7) + c[i] + (h>>25);
	return h;
}
template<typename T> bool base::HashMap<T>::compare(const char* a, const char* b) {
	return strcmp(a, b)==0;
}
template<typename T> unsigned int base::HashMap<T>::index(const char* c, unsigned int n) const {
	unsigned int h=hash(c)%n;
	for(unsigned int i=0; i<n; i++) {
		unsigned int k=(h+i)%n;
		if(!m_data[k] || compare(m_data[k]->key, c)) return k;
	}
	return ~0u;
}
template<typename T> void base::HashMap<T>::resize(unsigned int newSize) {
	//printf("Resize %u -> %u\n", m_capacity, newSize);
	Pair** tmp = m_data;
	m_data = new Pair*[newSize];
	memset(m_data, 0, newSize*sizeof(Pair*));
	for(unsigned int i=0; i<m_capacity; i++) if(tmp[i]) {
		unsigned int k=index( tmp[i]->key, newSize );
		m_data[k] = tmp[i];
	}
	m_capacity = newSize;
	delete [] tmp;
	validate();
}

template<typename T> bool base::HashMap<T>::validate() const {
	for(unsigned i=0; i<m_capacity; ++i) {
		if(m_data[i] && index(m_data[i]->key, m_capacity)!=i) { assert(false); return false; }
	}
	return true;
}

#undef THRESHOLD
#endif
