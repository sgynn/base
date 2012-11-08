#ifndef _BASE_HASHMAP_
#define _BASE_HASHMAP_

#include <cstring>

#define THRESHOLD 1.5f

namespace base {
	
	/** A quick implementation of a hashmap. Keys are c strings. */
	template <typename T>
	class HashMap {
		public:
		struct Pair { const char* key; T value; };
		HashMap(int cap=8);
		HashMap(const HashMap& m);
		HashMap& operator=(const HashMap&);
		~HashMap();
		bool empty() const { return m_size==0; }
		unsigned int size() const { return m_size; }
		bool contains(const char* key) const;
		T& operator[](const char* key);
		const T& operator[](const char* key) const;
		T& insert(const char* key, const T& value);
		void erase(const char* key);
		void clear();
		/** iterator class. Works the same as stl iterators */
		class iterator {
			friend class HashMap;
			public:
			iterator() : m_index(0), m_map(0) {}
			iterator operator++(int) { iterator tmp=*this; ++m_index; while(m_index<m_map->m_capacity && m_map->m_data[m_index].key==0) ++m_index; return tmp;}
			iterator operator++() { ++m_index; while(m_index<m_map->m_capacity && !m_map->m_data[m_index].key) ++m_index; return *this; }
			T& operator*() { return m_map->m_data[m_index].value; }
			T* operator->() { return &m_map->m_data[m_index].value; }
			const char* key() const { return m_map->m_data[m_index].key; }
			bool operator==(const iterator& o) const { return m_map==o.m_map && m_index==o.m_index; }
			bool operator!=(const iterator& o)  const { return m_map!=o.m_map || m_index!=o.m_index; }
			private:
			iterator( HashMap* map, int ix) : m_index(ix), m_map(map) {}
			unsigned int m_index;
			HashMap* m_map;
		};
		friend class iterator;
		iterator begin() { return ++iterator(this, ~0u); }
		iterator end()  { return iterator(this, m_capacity); }
		iterator find(const char* key) { 
			unsigned int i=index(key, m_capacity);
			return i<m_capacity && m_data[i].key? iterator(this, i): end();
		}
		
		private:
		unsigned int index(const char* c, unsigned int n) const;
		static unsigned int hash(const char* c);
		static bool compare(const char* a, const char* b);
		void resize(unsigned int newSize);
		Pair* m_data;
		unsigned int m_capacity, m_size;
	};
};

template<typename T> base::HashMap<T>::HashMap(int cap) : m_capacity(cap), m_size(0) {
	m_data = new Pair[cap];
	memset(m_data, 0, cap*sizeof(Pair));
}
template<typename T> base::HashMap<T>::HashMap(const HashMap& m) : m_capacity(m.m_capacity), m_size(m.m_size) {
	m_data = new Pair[ m_capacity ];
	memcpy(m_data, m.m_data, m_capacity*sizeof(Pair));
	for(unsigned int i=0; i<m_capacity; i++) if(m_data[i].key) {
		m_data[i].key = strdup(m_data[i].key);
		m_data[i].value = m.m_data[i].value;
	}
}
template<typename T> base::HashMap<T>& base::HashMap<T>::operator=(const HashMap& m) {
	clear();
	m_capacity = m.m_capacity;
	m_size = m.m_size;
	m_data = new Pair[ m_capacity ];
	memcpy(m_data, m.m_data, m_capacity*sizeof(Pair));
	for(unsigned int i=0; i<m_capacity; i++) if(m_data[i].key) {
		m_data[i].key = strdup(m_data[i].key);
		m_data[i].value = m.m_data[i].value;
	}
	return *this;
}
template<typename T> base::HashMap<T>::~HashMap() {
	clear();
}
template<typename T> bool base::HashMap<T>::contains(const char* key) const {
	unsigned int i = index(key, m_capacity);
	return i<m_capacity && m_data[i].key;
}
template<typename T> T& base::HashMap<T>::insert(const char* key, const T& value) {
	if(m_size*THRESHOLD > m_capacity) resize((unsigned int)(m_capacity*THRESHOLD));
	unsigned int i = index(key, m_capacity); //should never be invalid due to resize line above
	m_data[i].key = strdup(key);
	m_data[i].value = value;
	m_size++;
	return m_data[i].value;
}
template<typename T> T& base::HashMap<T>::operator[](const char* key) {
	if(m_size*THRESHOLD > m_capacity) resize((unsigned int)(m_capacity*THRESHOLD));
	unsigned int i = index(key, m_capacity); //should never be invalid due to resize line above
	if(m_data[i].key==0) {
		m_data[i].key = strdup(key);
		m_size++;
		//printf("Insert: %s [%d]\n", key, i);
		return m_data[i].value;
	} else {
		return m_data[i].value;
	}
}
template<typename T> const T& base::HashMap<T>::operator[](const char* key) const {
	unsigned int i = index(key, m_capacity);
	if(i<m_capacity) return m_data[i].value;
	else return m_data[0].value; //undefined behaviour if element does not exist
}

template<typename T> void base::HashMap<T>::erase(const char* key) {
	unsigned int i = index(key, m_capacity);
	if(i<m_capacity && m_data[i].key) {
		delete m_data[i].key;
		m_data[i].key = 0;
	}
}
template<typename T> void base::HashMap<T>::clear() {
	for(unsigned int i=0; i<m_capacity; i++) if(m_data[i].key) delete [] m_data[i].key;
	delete [] m_data;
	m_size = 0;
}
template<typename T> unsigned int base::HashMap<T>::hash(const char* c) {
	unsigned int h=0;
	for(unsigned int i=0; c[i]; i++) h = (h<<7) + c[i] + (h>>25);
	return h;
}
template<typename T> bool base::HashMap<T>::compare(const char* a, const char* b) {
	unsigned int i;
	for(i=0; a[i] && b[i]; i++) if(a[i]!=b[i]) return false;
	return a[i]==b[i];
}
template<typename T> unsigned int base::HashMap<T>::index(const char* c, unsigned int n) const {
	unsigned int h=hash(c)%n;
	for(unsigned int i=0; i<n; i++) {
		unsigned int k=(h+i)%n;
		if(m_data[k].key==0 || compare(m_data[k].key, c)) return k;
	}
	return ~0u;
}
template<typename T> void base::HashMap<T>::resize(unsigned int newSize) {
	//printf("Resize %u -> %u\n", m_capacity, newSize);
	Pair* tmp = m_data;
	m_data = new Pair[newSize];
	memset(m_data, 0, newSize*sizeof(Pair));
	for(unsigned int i=0; i<m_capacity; i++) if(tmp[i].key) {
		unsigned int k=index( tmp[i].key, newSize );
		memcpy(&m_data[k], &tmp[i], sizeof(Pair));
	}
	m_capacity = newSize;
	delete [] tmp;
}

#undef THRESHOLD
#endif
