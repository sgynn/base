#ifndef _MATERIAL_
#define _MATERIAL_

#include <map>
#include <string>

#include "math.h"

#define MAX_TEXTURE_UNITS 8

namespace base {
class Texture {
	friend class Material;
	public:
	static const Texture& getTexture(const char* name);
	static const Texture& createTexture(int width, int height, uint format);
	static void reload();
	void clamp(bool edge=true);
	int bind() const;
	uint getGLTexture() const { return m_texture; }
	const char* name() const { return m_name; }
	const Texture& operator=(const Texture& t) { m_texture=t.m_texture; m_good=t.m_good; m_name=t.m_name; return *this; }
	Texture() : m_texture(0), m_good(false) {};
	private:
	int load(const char* filename);
	typedef std::map<std::string, Texture> TextureMap; //could use a hashmap instead
	static TextureMap s_textures;
	const char* m_name;
	uint m_texture;
	bool m_good;
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
	int bind();
	
	private:
	static uint s_flags;
};
};

#endif
