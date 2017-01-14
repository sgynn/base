#ifndef _BASE_COLOUR_
#define _BASE_COLOUR_

/** Colours */
class Colour {
	public:
	float r,g,b,a;

	Colour();
	Colour(float r, float g, float b, float a=1.0);
	Colour(const Colour&, float a);
	Colour(unsigned c, float a=1.0);
	operator float*();
	operator const float*() const;
	operator unsigned() const;
	bool     operator==(const Colour&) const;
	bool     operator!=(const Colour&) const;
	Colour&  operator=(unsigned);

	unsigned toRGB() const;
	unsigned toARGB() const;
	Colour&  fromRGB(unsigned);
	Colour&  fromARGB(unsigned);

};

inline Colour::Colour(): r(0), g(0), b(0), a(1) {}
inline Colour::Colour(float r, float g, float b, float a): r(r), g(g), b(b), a(a) {}
inline Colour::Colour(const Colour& c, float a): r(c.r), g(c.g), b(c.b), a(a) {}
inline Colour::Colour(unsigned c, float a) { fromRGB(c); this->a=a; }
inline Colour& Colour::operator=(unsigned c) { return fromRGB(c); }
inline Colour::operator float*()             { return &r; }
inline Colour::operator const float*() const { return &r; }
inline Colour::operator unsigned() const     { return toRGB(); }
inline bool    Colour::operator==(const Colour& c) const { return r==c.r && g==c.g && b==c.b && a==c.a; }
inline bool    Colour::operator!=(const Colour& c) const { return r!=c.r || g!=c.g || b!=c.b || a!=c.a; }

inline unsigned Colour::toRGB() const { return ((unsigned)(r*255)<<16) + ((unsigned)(g*255)<<8) + ((unsigned)(b*255)); }
inline unsigned Colour::toARGB() const { return toRGB() | (unsigned)(a*255)<<24; }
inline Colour&  Colour::fromRGB(unsigned c) { r=((c&0xff0000)>>16)/255.0f; g=((c&0xff00)>>8)/255.0f; b=(c&0xff)/255.0f; a=1.0; return *this; }
inline Colour&  Colour::fromARGB(unsigned c) { fromRGB(c); a=((c&0xff000000)>>24)/255.0f; return *this; }

inline Colour lerp(const Colour& a, const Colour& b, float t) { 
	return Colour(a.r+(b.r-a.r)*t, a.g+(b.g-a.g)*t, a.b+(b.b-a.b)*t, a.a+(b.a-a.a)*t);
}

// Some constants
const Colour black   = Colour(0,0,0);
const Colour white   = Colour(1,1,1);
const Colour red     = Colour(1,0,0);
const Colour blue    = Colour(0,0,1);
const Colour green   = Colour(0,1,0);
const Colour cyan    = Colour(0,1,1);
const Colour magenta = Colour(1,0,1);
const Colour yellow  = Colour(1,1,0);
const Colour orange  = Colour(1,0.5f,0);
const Colour purple  = Colour(1,0,0.5f);
const Colour grey    = Colour(0.5f,0.5f,0.5f);
const Colour transparent = Colour(0,0,0,0);


#endif

