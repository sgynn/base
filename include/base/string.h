#pragma once

#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdarg>
#include <cstdio>

namespace base {
	using StringList = std::vector<class String>;

	class String {
		public:
		String(const char* s = nullptr) : m_data(0) { set(s); }
		String(const char* s, size_t len):m_data(0) { if(s&&s[0]&&len) { size_t l=strlen(s); if(len<l)l=len; m_data=(char*)malloc(l+1); memcpy(m_data,s,l); m_data[l]=0; } }
		String(String&& s)noexcept:m_data(s.m_data) { s.m_data = nullptr; }
		String(const String& s)	: m_data(0)			{ set(s); }
		const String& operator=(const char* s)		{ set(s); return *this;}
		const String& operator=(const String& s)	{ set(s); return *this; }
		const String& operator=(String&& s)	        { if(m_data) free(m_data); m_data=s.m_data; s.m_data=0; return *this; }

		bool operator==(const char* s) const		{ return m_data==s || (!m_data&&!*s) || (m_data && s && strcmp(m_data, s)==0); }
		bool operator==(const String& s) const		{ return operator==(s.m_data); }
		bool operator!=(const char* s) const		{ return m_data!=s && (m_data||*s) && (!m_data || !s || strcmp(m_data, s)); }
		bool operator!=(const String& s) const		{ return operator!=(s.m_data); }
		friend bool operator==(const char* a, const String& s) { return s == a; }
		friend bool operator!=(const char* a, const String& s) { return s != a; }

		~String()									{ set(0); }
		String& swap(String& other)					{ char* t=m_data; m_data=other.m_data; other.m_data=t; return *this;}
		operator const char*() const				{ return m_data; }
		const char* str() const                     { return m_data? m_data: ""; }
		size_t length() const						{ return m_data? strlen(m_data): 0; }
		bool empty() const							{ return !m_data || !m_data[0]; }
		void clear()                                { set(0); }

		bool startsWith(const char* s) const        { return !s || strncmp(str(), s, strlen(s))==0; }
		bool endsWith(const char* s) const          { if(!s) return true; if(!m_data) return false; int sl=strlen(s); return strncmp(str() + length() - sl, s, sl) == 0; }
		const char* contains(const char* s) const   { if(!s) return nullptr; return strstr(str(), s); }
		bool match(const char* pattern) const       { return match(str(), pattern); }

		char operator[](unsigned i) const           { return m_data[i]; }
		char& operator[](unsigned i)                { return m_data[i]; }
		String& operator+=(const char* s)           { if(s&&s[0]) *this = *this + s; return *this; }
		String& operator+=(const String& s)         { if(!s.empty()) *this = *this + s; return *this; }
		String operator+(const char* s) const       { return cat(m_data, length(), s, s?strlen(s):0); }
		String operator+(const String& s) const     { return cat(m_data, length(), s, s.length()); }
		friend String operator+(const char* a, const String& s) { return cat(a, a?strlen(a):0, s, s.length()); }

		String& toUpper() {
			if(m_data) for(char* c=m_data; *c; ++c) if(*c>96) *c &= 0xdf;
			return *this;
		}
		String& toLower() {
			if(m_data) for(char* c=m_data; *c; ++c) if(*c>64) *c |= 0x20;
			return *this;
		}
		String& toTitle() {
			if(m_data) for(char* c=m_data; *c; ++c) {
				if((c==m_data || c[-1]==' ') && *c>96) *c &= 0xdf;
			}
			return *this;
		}

		// Wildcard match - supports * and ? wildcards
		static bool match(const char* s, const char* pattern, int limit=-1) {
			const char* rollback = nullptr;
			const char* p = pattern;
			for(const char* c = s; *c; ++c) {
				if(p==rollback) {
					if(p[1]=='?') rollback = ++p;
					if(*c == p[1]) p+=2;
				}
				else if(*p=='?' || *p==*c) ++p;
				else if(*p=='*') {
					while(p[1]=='*') ++p;
					if(p[1]=='?') ++p; // Essentially swap *? to ?*
					rollback = p;
					if(*c == p[1]) p+=2;
				}
				else if(rollback) p = rollback;
				else return false;
				if(--limit==0) break;
			}
			if(p==rollback) ++p;
			while(*p=='*') ++p;
			return *p==0;
		}


		static String format(const char* format, ...) {
			char buffer[128];
			va_list v;
			va_start(v, format);
			int len = vsnprintf(buffer, 128, format, v);
			va_end(v);
			if(len<128) return String(buffer);
			char* big = (char*)malloc(len+1);
			va_start(v, format);
			vsprintf(big, format, v);
			va_end(v);
			String result;
			result.m_data = big;
			return result;
		}

		static String cat(const char* a, const char* b) { return cat(a, a?strlen(a):0, b, b?strlen(b):0); }
		static String cat(const char* a, const char* b, const char* c) { return cat(a, a?strlen(a):0, b, b?strlen(b):0, c, c?strlen(c):0); }
		static String cat(const char* a, int alen, const char* b, int blen, const char* c=0, int clen=0) {
			String r;
			r.m_data = (char*)malloc(alen + blen + clen + 1);
			memcpy(r.m_data, a, alen);
			memcpy(r.m_data+alen, b, blen);
			memcpy(r.m_data+alen+blen, c, clen);
			r.m_data[alen+blen+clen] = 0;
			return r;
		}

		template<class List>
		static String join(const List& list, const char* separator) {
			String result;
			bool first = true;
			for(auto& i: list) {
				if(first) first = false;
				else result += separator;
				result += i;
			}
			return result;
		}

		template<class List, class Func>
		static String join(const List& list, const char* separator, const Func& func) {
			String result;
			bool first = true;
			for(auto& i: list) {
				if(first) first = false;
				else result += separator;
				result += func(i);
			}
			return result;
		}


		static StringList split(const char* src, const char* tok, bool keepEmpty=false, const char* trim=" ") {
			StringList out;
			if(!src || !tok || !tok[0]) return out; // Invalid input
			if(trim && !trim[0]) trim = 0;
			
			const char* s = src;
			if(trim) while(strchr(trim, *s)) ++s;
			const char* e = s;
			while(*s) {
				if(!*e || strchr(tok, *e)) {
					const char* end = e;
					if(trim) while(e>s&&strchr(trim, e[-1])) --e;
					if(keepEmpty || e>s) out.push_back(String(s, e-s));
					if(*end) ++end;
					if(*end && trim) while(strchr(trim, *end)) ++end;
					s = e = end;
				}
				else ++e;
			}
			return out;
		}

		// Allocate uninitialised string memory to avoid unnessesary copies
		char* allocateUnsafe(int len) { free(m_data); m_data = (char*)malloc(len); return m_data; }

		private:
		friend class StringView;
		void set(const char* s) { if(m_data==s) return; if(m_data) free(m_data); m_data = s && s[0]? strdup(s): 0; }
		char* m_data;
	};


	// A wrapper for char* that does NOT make a copy, Just points to an existing char array. Careful with scope
	class StringView {
		public:
		StringView(const char* s = nullptr) : m_data(s), m_length(s?strlen(s):0) {}
		StringView(const char* s, size_t len)   : m_data(s), m_length(len) {}
		StringView(const StringView& s) : m_data(s.m_data), m_length(s.m_length) {}
		const StringView& operator=(const char* s)  { m_data=s; m_length=s? strlen(s): 0; return *this; }

		operator String() const { return String(m_data, m_length); }
		bool operator==(const char* s) const        { return (empty() && (!s||!s[0])) || (m_data && s && strncmp(m_data, s, m_length)==0); }
		bool operator==(const StringView& s) const  { return m_length==s.m_length && (m_length==0 || m_data==s.m_data || strncmp(m_data, s.m_data, m_length)==0); }
		bool operator==(const String& s) const      { return *this == s.str(); }
		bool operator!=(const char* s) const        { return !operator==(s); }
		bool operator!=(const StringView& s) const  { return !operator==(s); }
		bool operator!=(const String& s) const      { return !operator==(s.str()); }
		friend bool operator==(const char* a, const StringView& s) { return s == a; }
		friend bool operator!=(const char* a, const StringView& s) { return s != a; }
		friend bool operator==(const String& a, const StringView& s) { return s.operator==(a.str()); }
		friend bool operator!=(const String& a, const StringView& s) { return !s.operator==(a.str()); }

		size_t length() const                       { return m_length; }
		bool empty() const                          { return m_length==0; }
		void clear()                                { m_data=nullptr; m_length=0; }

		bool startsWith(const char* s) const        { if(!s) return true; size_t l=strlen(s); return l<=m_length && strncmp(m_data, s, l)==0; }
		bool endsWith(const char* s) const          { if(!s) return true; size_t l=strlen(s); return l<m_length && strncmp(m_data + m_length - l, s, l) == 0; }
		const char* contains(const char* s) const   {
			if(!s || !s[0] || m_length==0) return nullptr;
			const char* end = m_data + m_length - strlen(s);
			for(const char* c = m_data; c<=end; ++c) if(strcmp(c, s)==0) return c;
			return nullptr;
		}
		bool match(const char* pattern)				{ return String::match(m_data? m_data: "", pattern, m_length); }

		char operator[](unsigned i) const           { return m_data[i]; }
		String operator+(const char* s) const       { return String::cat(m_data, m_length, s, s?strlen(s):0); }
		String operator+(const StringView& s) const { return String::cat(m_data, m_length, s.m_data, s.m_length); }
		friend String operator+(const char* a, const StringView& s) { return String::cat(a, a? strlen(a): 0, s.m_data, s.m_length); }
		friend String operator+(const String& a, const StringView& s) { return String::cat(a.str(), a.length(), s.m_data, s.m_length); }
		private:
		const char* m_data;
		size_t m_length;
	};
}

