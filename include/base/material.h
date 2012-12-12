#ifndef _BASE_MATERIAL_
#define _BASE_MATERIAL_

#include "math.h"
#include "hashmap.h"
#include "texture.h"
#include "shader.h"


namespace base {
	
	/** Advanced material - contains shader and variables */
	class Material : private SMaterial {
		public:
		Material();
		~Material();

		/** Set shadere variables */
		void setFloat(const char* name, float f);
		void setFloatv(const char* name, int v, float* fp);
		void setInt(const char* name, int i);

		/** Get shader variables */
		float getFloat(const char* name);
		int getFloatv(const char* name, float* fp);
		int getInt(const char* name);

		/** Set texture */
		void setTexture(const char* name, const Texture&);
		Texture getTexture(const char* name);

		/** Set shader */
		void setShader(const Shader& shader)	{ m_shader = shader; }
		base::Shader getShader() const { return m_shader; }

		/** Set material properties */
		void setDiffuse(const Colour& c) 			{ diffuse = c; }
		void setAmbient(const Colour& c) 			{ ambient = c; }
		void setSpecular(const Colour& c) 			{ specular = c; }
		void setShininess(float s) 					{ shininess = s; }

		void bind(int flags=0) const;

		protected:
		struct SVar { ubyte type; union { float f; int i; float* p; }; }; // Variable data
		HashMap<SVar> m_variables;
		Shader m_shader;
		uint m_textureCount;
	};

};

#endif
