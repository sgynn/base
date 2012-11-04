#ifndef _BASE_MATERIAL_
#define _BASE_MATERIAL_

#include <map>
#include <string>

#include "math.h"

#define MAX_TEXTURE_UNITS 8

namespace base {
class Texture {
	friend class Material;
	public:
	static const Texture& getTexture(const char* name);
	static const Texture createTexture(int width, int height, uint format, const void* data=0);
	static const Texture createTexture(int width, int height, uint format, uint sourceFormat, uint type, const void* data=0);
	static void reload();
	void clamp(bool edge=true) const;
	void filter(uint min, uint mag) const;
	int width() const { return m_width; }
	int height() const { return m_height; }
	int bitsPerPixel() const { return m_bpp; }
	int bind() const;
	int ready() const { return m_texture>0; }
	uint unit() const { return m_texture; } //alias of getGLTexture()
	uint getGLTexture() const { return m_texture; }
	const char* name() const { return m_name; }
	const Texture& operator=(const Texture& t);// { m_texture=t.m_texture; m_good=t.m_good; m_name=t.m_name; return *this; }
	Texture() : m_texture(0), m_good(false), m_width(0), m_height(0), m_bpp(0) {};
	private:
	int load(const char* filename);
	typedef std::map<std::string, Texture> TextureMap; //could use a hashmap instead
	static TextureMap s_textures;
	const char* m_name;
	uint m_texture;
	bool m_good;
	int m_width, m_height, m_bpp;
	static uint s_bound;
};

class Material {
	public:
	Colour diffuse;
	Colour ambient;
	Colour specular;
	float shininess;
	Texture texture[MAX_TEXTURE_UNITS];
	
	bool lighting:1;
	bool depthTest:1;
	bool depthMask:1;
	
	//Shader?
	
	Material();
	int bind() const;
	
	private:
	static uint s_flags;
};
};

#endif
