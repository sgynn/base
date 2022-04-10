#pragma once
#include <cstdlib>
#include <cstring>

template<typename T>
class vector {
	struct DefaultSort { bool operator()(const T& a, const T& b) { return a<b; } };
	struct StringSort { bool operator()(const char* a, const char* b) { return strcmp(a,b)<0; } };
	public:
	vector(int s=0)                        : m_data(0), m_size(0), m_capacity(0) { if(s) reserve(s); }
	vector(const vector& v) = delete;
	~vector()                              { if(m_data) free(m_data); }

	unsigned size() const                  { return m_size; }
	unsigned capacity() const              { return m_capacity; }
	bool     empty() const                 { return m_size==0; }

	T&       operator[](int i)             { return m_data[i]; }
	const T& operator[](int i) const       { return m_data[i]; }

	void     clear()                       { m_size = 0; }
	void     reserve(unsigned s)           { resize(s); }
	void     optimise()                    { resize(m_size); }

	const T& front() const                 { return m_data[0]; }
	const T& back() const                  { return m_data[m_size-1]; }
	T&       front()                       { return m_data[0]; }
	T&       back()                        { return m_data[m_size-1]; }
	T&       push_back(const T& v)         { if(m_size==m_capacity) resize(m_size+12); m_data[m_size++]=v; return back(); }
	T&       push_front(const T& v)        { if(m_size==m_capacity) resize(m_size+12); shift(0,1); ++m_size; m_data[0]=v; return front(); }
	T&       insert(const T& v,unsigned p) { if(m_size==m_capacity) resize(m_size+12); shift(p,1); ++m_size; m_data[p]=v; return m_data[p]; }
	void     pop_back()	                   { if(m_size>0) --m_size; }
	void     pop_front()                   { if(m_size>0) { shift(1,-1); --m_size; } }
	void     erase(unsigned index)         { if(index<m_size) { shift(index,-1); --m_size; } }
	void     eraseFast(unsigned index)     { if(index<m_size) { m_data[index]=back(); pop_back(); } }
	void     remove(const T& v)            { erase(indexOf(v)); }
	void     removeFast(const T& v)        { eraseFast(indexOf(v)); }
	unsigned indexOf(const T& v, int s=0) const	{ for(unsigned i=s; i<m_size; ++i) if(m_data[i]==v) return i; return m_size; }

	void     swap(unsigned a, unsigned b)  { if(a<m_size&&b<m_size && a!=b) { T t=m_data[a]; m_data[a]=m_data[b]; m_data[b]=t; } }
	void     reverse()                     { for(int i=0; i<m_size/2; ++i) swap(i, m_size-i-1); }
	void     sort()                        { if(m_size>1) quicksort(0, m_size-1, DefaultSort()); }
	template<typename S> void sort(S& cmp) { if(m_size>1) quickSort(0, m_size-1, cmp); }

	T* begin() { return m_data; }
	T* end()   { return m_data+m_size; }

	private:
	T*       m_data;
	unsigned m_size;
	unsigned m_capacity;

	void resize(unsigned s) {
		if(s==m_capacity) return;
		else if(s==0) { free(m_data); m_data=0; }
		else {
			T* old = m_data;
			m_data = (T*) malloc(s*sizeof(T));
			memset(m_data, 0, s*sizeof(T));
			if(old) {
				memcpy(m_data, old, (s<m_capacity?s:m_capacity)*sizeof(T));
				free(old);
			}
		}
		m_capacity = s;
		if(s<m_size) m_size = s;
	}
	void shift(unsigned start, int d) {
		if(start >= m_size) return;
		if(d>0)      for(unsigned i=m_size+d-1; i>=start+d; --i) memcpy(m_data+i, m_data+i-d, sizeof(T));
		else if(d<0) for(unsigned i=start; i<m_size; ++i) memcpy(m_data+i+d, m_data+i, sizeof(T));
	}
	template<typename S>
	void quicksort(int l, int r, S cmp) {
		int i=l, j=r;
		T* p = &m_data[(l+r)>>1];
		while(i<=j) {
			while(cmp(m_data[i], *p)) ++i;
			while(cmp(*p, m_data[j])) --j;
			if(i<=j) { swap(i, j); ++i; --j; }
		}
		if(l<j) quicksort(l,j,cmp);
		if(i<r) quicksort(i,r,cmp);
	}
};

// String sort specialisation
template <> inline void vector<char*>::sort()       { if(m_size>1) quicksort(0, m_size-1, StringSort()); }
template <> inline void vector<const char*>::sort() { if(m_size>1) quicksort(0, m_size-1, StringSort()); }

