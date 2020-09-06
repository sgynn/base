#ifndef _BASE_COLOUR_
#define _BASE_COLOUR_

class HSV;

/** Colours */
class Colour {
	public:
	float r,g,b,a;

	Colour();
	Colour(float r, float g, float b, float a=1.0);
	Colour(const Colour&, float a);
	Colour(unsigned c, float a=1.0);
	Colour(const HSV& hsl, float a=1.0);
	void set(float r, float g, float b, float a=1.0);
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

	HSV      toHSV() const;
	Colour&  fromHSV(float h, float s, float v, float a=1.0);
	Colour&  fromHSV(const HSV&, float a=1.0);
};

class HSV {
	public:
	float hue, saturation, value;
	HSV(float h, float s, float v) : hue(h), saturation(s), value(v) {}
	HSV(const Colour& c) { *this = c.toHSV(); }
};

inline Colour::Colour(): r(0), g(0), b(0), a(1) {}
inline Colour::Colour(float r, float g, float b, float a): r(r), g(g), b(b), a(a) {}
inline Colour::Colour(const Colour& c, float a): r(c.r), g(c.g), b(c.b), a(a) {}
inline Colour::Colour(unsigned c, float alpha) { fromRGB(c); a=alpha; }
inline Colour::Colour(const HSV& hsv, float alpha) { fromHSV(hsv, alpha); }
inline void    Colour::set(float cr, float cg, float cb, float ca) { r=cr; b=cb; g=cg; a=ca; }
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
inline Colour&  Colour::fromHSV(const HSV& c, float a) { fromHSV(c.hue, c.saturation, c.value, a); return *this; }

inline Colour lerp(const Colour& a, const Colour& b, float t) { 
	return Colour(a.r+(b.r-a.r)*t, a.g+(b.g-a.g)*t, a.b+(b.b-a.b)*t, a.a+(b.a-a.a)*t);
}

// HSV conversion

inline HSV Colour::toHSV() const {
	float vmin = r<g && r<b? r: g<b? g: b;
	float vmax = r>g && r>b? r: g>b? g: b;
	float diff = vmax - vmin;
	HSV out(0, 0, vmax);
	if(diff>0) { 
		out.saturation = diff / vmax;
		if     (vmax==r) out.hue = (g-b) / diff;
		else if(vmax==g) out.hue = 2 + (b-r) / diff;
		else if(vmax==b) out.hue = 4 + (r-g) / diff;
		out.hue /= 6.f;
		out.hue -= floor(out.hue);
	}
	return out;

}
inline Colour& Colour::fromHSV(float h, float s, float v, float a) {
	if(s <= 0) set(v,v,v,a); // No saturation
	else {
		h = (h - floor(h)) * 6;
		float i = floor(h);
		float v1 = v * (1 - s);
		float v2 = v * (1 - s * (h - i));
		float v3 = v * (1 - s * (1 - (h - i)));
		switch((int)i) {
		case 0: set(v, v3, v1, a); break;
		case 1: set(v2, v, v1, a); break;
		case 2: set(v1, v, v3, a); break;
		case 3: set(v1, v2, v, a); break;
		case 4: set(v3, v1, v, a); break;
		default: set(v, v1, v2, a);
		}
	}
	return *this;
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

