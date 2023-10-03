#pragma once

#include <vector>
#include <cstdarg>
#include <cstdio>

namespace base {
	using StringList = std::vector<class String>;

	class String {
		public:
		String(const char* s = nullptr) : m_data(0) { set(s); }
		String(const char* s, size_t len):m_data(0) { if(s&&s[0]&&len) { size_t l=strlen(s); if(len<l)l=len; m_data=(char*)malloc(l+1); memcpy(m_data,s,l); m_data[l]=0; } }
		String(String&& s) : m_data(s.m_data)       { s.m_data = nullptr; }
		String(const String& s)	: m_data(0)			{ set(s); }
		const String& operator=(const char* s)		{ set(s); return *this;}
		const String& operator=(const String& s)	{ set(s); return *this; }
		const String& operator=(String&& s)	        { if(m_data) free(m_data); m_data=s.m_data; s.m_data=0; return *this; }

		bool operator==(const char* s) const		{ return m_data==s || (!m_data&&!*s) || (m_data && strcmp(m_data, s)==0); }
		bool operator==(const String& s) const		{ return m_data==s.m_data || (m_data && strcmp(m_data, s.m_data)==0); }
		bool operator!=(const char* s) const		{ return m_data!=s && (m_data||*s) && (!m_data || strcmp(m_data, s)); }
		bool operator!=(const String& s) const		{ return m_data!=s.m_data && (!m_data || strcmp(m_data, s.m_data)); }
		friend bool operator==(const char* a, const String& s) { return s == a; }
		friend bool operator!=(const char* a, const String& s) { return s != a; }

		~String()									{ set(0); }
		String& swap(String& other)					{ char* t=m_data; m_data=other.m_data; other.m_data=t; return *this;}
		operator const char*() const				{ return m_data; }
		const char* str() const                     { return m_data? m_data: ""; }
		size_t length() const						{ return m_data? strlen(m_data): 0; }
		bool empty() const							{ return !m_data || !m_data[0]; }
		void clear()                                { set(0); }

		bool startsWith(const char* s) const        { return s && strncmp(str(), s, strlen(s))==0; }
		bool endsWith(const char* s) const          { if(!s||!m_data) return false; int sl=strlen(s); return strncmp(str() + length() - sl, s, sl) == 0; }
		const char* contains(const char* s) const   { if(!s) return nullptr; return strstr(str(), s); }

		char& operator[](unsigned i)                { return m_data[i]; }
		String& operator+=(const char* s)           { if(s&&s[0]) *this = *this + s; return *this; }
		String& operator+=(const String& s)         { if(!s.empty()) *this = *this + s; return *this; }
		String operator+(const char* s) const       { if(empty()) return String(s); if(!s||!s[0]) return *this; String r; r.m_data=(char*)malloc(length() + strlen(s) + 1); sprintf(r.m_data, "%s%s", m_data,s); return r; }
		String operator+(const String& s) const     { if(empty()) return s; if(s.empty()) return *this; String r; r.m_data=(char*)malloc(length() + s.length() + 1); sprintf(r.m_data, "%s%s", m_data,s.m_data); return r; }
		friend String operator+(const char* a, const String& s) { if(!a&&!a[0]) return s; if(s.empty()) return String(a); String r; r.m_data=(char*)malloc(strlen(a)+s.length()+1); sprintf(r.m_data, "%s%s", a, s.m_data); return r; }

		static String format(const char* format, ...) {
			char buffer[128];
			va_list v;
			va_start(v, format);
			int len = vsnprintf(buffer, 128, format, v);
			va_end(v);
			if(len<128) return String(buffer);
			char* big = (char*)malloc(len);
			va_start(v, format);
			vsprintf(big, format, v);
			va_end(v);
			String result;
			result.m_data = big;
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
					s = e = end;
				}
				else ++e;
			}
			return out;
		}

		private:
		void set(const char* s) { if(m_data==s) return; if(m_data) free(m_data); m_data = s && s[0]? strdup(s): 0; }
		char* m_data;
	};
}

