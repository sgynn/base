#ifndef _BASE_PARSE_
#define _BASE_PARSE_

namespace base {
	/** Generic parsing functions. Regex would be so nice at this point */
	int  parseSpace       (const char* in);						/** Parse whitespace 			     [\s]*                         */
	int  parseInt         (const char* in, int& i);				/** Parse an integer value           -?[0-9]+                      */
	int  parseHex         (const char* in, unsigned& i);		/** Parse hexadecimal integer        [0-9a-xA-X]+                  */
	int  parseFloat       (const char* in, float& f);			/** Parse a float value              -?[0-9]+(.[0-9]*)?(e[0-9]+)?  */
	int  parseAlphaNumeric(const char* in, char* s, int l=64);	/** Parse an alphanumeric string     [a-zA-Z_][0-9a-zA-Z_]*        */
	int  parseString      (const char* in, char* s);			/** Parse quote encapsulated string  "[^"]*"|'[^']*'               */

	





	// Implement them all inline
	inline int parseSpace(const char* in) {
		int t=0;
		while(in[t]==' ' || in[t]=='\n' || in[t]=='\t' || in[t]=='\r') ++t;
		return t;
	}
	inline int parseInt(const char* in, int& v) {
		int t=0, m=1; v=0;
		// Sign
		if(*in=='+' || *in=='-') { ++t; m = 0x2c-*in; }
		// Digits
		while(in[t]>='0' && in[t]<='9') { v = v*10 + (in[t] - '0'); ++t; }
		v *= m;
		return t;
	}
	inline int parseHex(const char* in, unsigned v) {
		int t=0; v=0;
		while((in[t]>='0' && in[t]<='9') || (in[t]>='A' && in[t]<='F') || (in[t]>='a' && in[t]<='f')  ) { 
			int k = in[t]<='9'? in[t]-'0':  in[t]<='F'? in[t]-'A'+10: in[t]-'a'+10;
			v = v*0xf + k;
			++t;
		}
		return t;
	}
	inline int parseFloat(const char* in, float& v) {
		int iv=0, m=1, t=0;
		// Sign
		if(*in=='+' || *in=='-') { ++t; m = 0x2c-*in; }
		t += parseInt(in+t,iv);
		in += t;
		// Fraction
		if(*in=='.') {
			++in; ++t;
			if(*in=='-' || *in=='+') { v=iv; return t; } // Invalid +/- accepted by parseInt()
			int frac;
			int ft = parseInt(in, frac);
			v = iv + frac * pow(10, -ft);
			t += ft;
			in += ft;
		}
		// Exponent
		if(*in == 'e') {
			++in;
			int exp;
			t += parseInt(in, exp);
			v = v * pow(10, exp);
		}
		v *= m;
		return t;
	}
	inline int parseAlphaNumeric(const char* in, char* s, int lim) {
		int t=0;
		// First digit must not be a numeric
		if((*in>='A' && *in<='Z') || (*in>='a' && *in<='z') || *in=='_') { *s = *in; ++in; }
		else return 0;
		// and the rest
		while(t<lim && ((*in>='A' && *in<='Z') || (*in>='a' && *in<='z') || (*in>='0' && *in<='9') || *in=='_')) { s[++t] = *in; ++in; }
		s[++t] = 0;
		return ++t;
	}
	inline int parseString(const char* in, char* s) {
		int t=1; char q=*in;
		if(q=='\'' || q=='"') {
			while(in[t] && in[t]!=q) { s[t-1]=in[t]; ++t; }
			s[t-1] = 0;
			if(in[t]==q) return t+1;
		}
		return 0;
	}

};

#endif

