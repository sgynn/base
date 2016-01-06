#include <base/material.h>
#include <base/opengl.h>

using namespace base;

#ifdef WIN32
extern PFNGLACTIVETEXTUREARBPROC glActiveTexture;
#endif

enum SHADER_VARIABLE_TYPES {
	INTEGER = 0, // Integer (or texture)
	FLOAT1  = 1, // Single float
	FLOAT2  = 2, // vec2
	FLOAT3  = 3, // vec3
	FLOAT4  = 4, // vec4 or colour
	FLOATN  = 5  // Float array
};


Material::Material(): m_textureCount(0), m_blend(BLEND_ALPHA) {}
Material::~Material() {
	// delete any variable pointers
	for(base::HashMap<SVar>::iterator it=m_variables.begin(); it!=m_variables.end(); ++it) {
		if(it->type > FLOAT1) delete [] it->p;
	}
}
Material::Material(const Material& m) : SMaterial(m), m_variables(m.m_variables), m_shader(m.m_shader), m_textureCount(m.m_textureCount), m_blend(m.m_blend) {
	// Fix any variables that use pointers
	for(base::HashMap<SVar>::iterator it=m_variables.begin(); it!=m_variables.end(); ++it) {
		if(it->type > FLOAT1) {
			float* p = new float[ it->type ];
			memcpy(p, it->p, it->type * sizeof(float));
			it->p = p;
		}
	}
}

void Material::setFloat(const char* name, float f) {
	SVar v; v.type=FLOAT1; v.f = f;
	m_variables[name] = v;
}
void Material::setFloat2(const char* name, const float* p) { setFloatv(name, 2, p); }
void Material::setFloat3(const char* name, const float* p) { setFloatv(name, 3, p); }
void Material::setFloat4(const char* name, const float* p) { setFloatv(name, 4, p); }

void Material::setFloatv(const char* name, int v, const float* fp) {
	HashMap<SVar>::iterator it = m_variables.find(name);
	int type = FLOAT1 + v - 1;
	// Check if it exists to so we can use already allocated memory
	if(it!=m_variables.end()) {
		// Allocate new memory for array if size is different
		if(it->type!=type) {
			if(it->type > FLOAT1) delete [] it->p;
			if(type > FLOAT1) it->p = new float[v];
			it->type = type;
		}
		if(type == FLOAT1) it->f = *fp;
		else memcpy(it->p, fp, v*sizeof(float));
	} else {
		// Allocate new variable
		SVar var;
		var.type = type;
		if(type == FLOAT1) var.f = *fp;
		else {
			var.p = new float[v];
			memcpy(var.p, fp, v*sizeof(float));
		}
		m_variables.insert(name, var);
	}
}
void Material::setInt(const char* name, int i) {
	SVar v; v.type=INTEGER; v.i = i;
	m_variables[name] = v;
}

float Material::getFloat(const char* name) {
	base::HashMap<SVar>::iterator it = m_variables.find(name);
	if(it!=m_variables.end() && it->type==FLOAT1) return it->f;
	else return 0;
}
int Material::getFloatv(const char* name, float* fp) {
	base::HashMap<SVar>::iterator it = m_variables.find(name);
	if(it!=m_variables.end() && it->type>=FLOAT1) {
		if(it->type == FLOAT1) *fp = it->f;
		else memcpy(fp, it->p, it->type*sizeof(float));
		return it->type;
	} else return 0;
}
int Material::getInt(const char* name) {
	base::HashMap<SVar>::iterator it = m_variables.find(name);
	if(it!=m_variables.end() && it->type==INTEGER) return it->i;
	else return 0;
}

void Material::setTexture(const char* name, const Texture& tex) {
	base::HashMap<SVar>::iterator it = m_variables.find(name);
	if(it!=m_variables.end() && it->type==INTEGER) texture[ it->i ] = tex;
	else {
		// Add new texture
		texture[ m_textureCount ] = tex;
		setInt(name, m_textureCount);
		++m_textureCount;
	}
}
Texture Material::getTexture(const char* name) {
	base::HashMap<SVar>::iterator it = m_variables.find(name);
	if(it!=m_variables.end() && it->type==INTEGER) return texture[ it->i ];
	else return Texture();
}

void Material::bind(int flags) const {
	// Set properties
	glMaterialfv(GL_FRONT, GL_DIFFUSE,   diffuse);
	glMaterialfv(GL_FRONT, GL_AMBIENT,   ambient);
	glMaterialfv(GL_FRONT, GL_SPECULAR,  specular);
	glMaterialf (GL_FRONT, GL_SHININESS, shininess);

	// bind textures
	for(uint i=0; i<m_textureCount; ++i) {
		glActiveTexture(GL_TEXTURE0+i);
		texture[i].bind();
	}
	if(m_textureCount>1) glActiveTexture(GL_TEXTURE0);

	// Blend mode
	static BlendMode sBlend = BLEND_NONE;
	if(m_blend!=sBlend) {
		sBlend = m_blend;
		switch(m_blend) {
		case BLEND_NONE:  glDisable(GL_BLEND); break;
		case BLEND_ALPHA: 
			glEnable(GL_BLEND); 
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
			break;
		case BLEND_ADD:
			glEnable(GL_BLEND); 
			glBlendFunc(GL_ONE, GL_ONE); 
			break;
		}
	}

	// Bind shader
	m_shader.bind();

	// Set variables
	if(m_shader.ready()) {
		//FIXME const hack for now - until i rewrite shader
		Shader* s = const_cast<Shader*>(&m_shader);

		for(base::HashMap<SVar>::const_iterator it=m_variables.begin(); it!=m_variables.end(); ++it) {
			switch(it->type) {
			case INTEGER:	s->Uniform1i( it.key(), it->i ); break;
			case FLOAT1: 	s->Uniform1f( it.key(), it->f ); break;
			case FLOAT2:    s->Uniform2f( it.key(), it->p[0], it->p[1] ); break;
			case FLOAT3:    s->Uniform3f( it.key(), it->p[0], it->p[1], it->p[2] ); break;
			case FLOAT4:    s->Uniform4f( it.key(), it->p[0], it->p[1], it->p[2], it->p[3] ); break;
			default: s->Uniformfv( it.key(), it->type-FLOAT1, it->p ); break;
			}
		}
	}
}



//// //// //// //// //// //// //// //// LOADING //// //// //// //// //// //// //// ////
/*

#include <base/material.h>

Material Material::loadXML(const char* string) {
	Material mat;
	base::XML xml;
	xml.loadData(string);
	if(strcmp(xml.getRoot().name(), "material")!=0) {
		printf("Invalid material descriptor [%s]\n", xml.getRoot().name());
	} else {
		for(base::XML::iterator i = xml.getRoot().begin(); i!=xml.getRoot().end(); ++i) {
			if(strcmp(i->name(), "texture")==0) {
				// Load texture TODO resource manager to load texture
				printf("Load texture %s [%s]\n", i->attribute("name"), i->attribute("file"));
				base::Texture tex = base::Texture::getTexture( i->attribute("file") );
				mat.setTexture(i->attribute("name"), tex.getGLTexture());

			} else if(strcmp(i->name(), "shader")==0) {
				// load shader - TODO resource manager to load shader
				printf("Load shader [%s]\n", i->attribute("file"));
				mat.m_shader = base::Shader::getShader( i->attribute("file") );

			} else if(strcmp(i->name(), "variable")==0) {
				// Switch type
				const char* name = i->attribute("name");
				const char* t = i->attribute("type");
				if(strcmp(t, "int")==0) mat.setInt(name, i->attribute("value", 0));
				else if(strcmp(t, "float")==0) mat.setFloat(name, i->attribute("value", 0.0f));
				else if(strncmp(t, "vec", 3)==0 && t[4]==0) {
					printf("Not readibg vectors yet\n");
					// Could be {x,y,z,w,r,g,b,a,u,v, ...} or value string space separated
				}

			} else if(strcmp(i->name(), "shininess")==0) {
				mat.setShininess( i->attribute("value", 128.0f) );

			} else if(strcmp(i->name(), "diffuse")==0 ||  strcmp(i->name(), "specular")==0) {
				// Read colour from hex code
				Colour col;
				const char* cs = i->attribute("colour", "");
				if(!cs[0]) cs = i->attribute("color", "");
				if(cs[0]) {
					uint hex = 0;
					for(const char* c=cs; *c; ++c) {
						hex = hex<<4;
						if(*c>='0' && *c<='9') hex += *c-'0';
						else if(*c>='a' && *c<='f') hex += *c-'a'+10;
						else if(*c>='A' && *c<='F') hex += *c-'A'+10;
						else continue; // Error
					}
					col = Colour(hex);
				} else {	// Colour as rgb
					col.r = i->attribute("r", 1.0);
					col.g = i->attribute("g", 1.0);
					col.b = i->attribute("b", 1.0);
				}
				col.a = i->attribute("alpha", 1.0f);
				if(i->name()[0]=='d') mat.setDiffuse(col);
				else mat.setSpecular(col);
			}
		}
	}
	return mat;
}
*/







