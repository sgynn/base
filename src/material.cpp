#include <base/material.h>
#include <base/opengl.h>

using namespace base;

enum SHADER_VARIABLE_TYPES {
	INTEGER = 0, // Integer (or texture)
	FLOAT   = 1, // Single float
	FLOAT2  = 2, // vec2
	FLOAT3  = 3, // vec3
	FLOAT4  = 4, // vec4 or colour
	FLOATN  = 5  // Float array
};


Material::Material(): m_textureCount(0) {}
Material::~Material() { }

void Material::setFloat(const char* name, float f) {
	SVar v; v.type=FLOAT; v.f = f;
	m_variables[name] = v;
}
void Material::setFloatv(const char* name, int v, float* fp) {
	HashMap<SVar>::iterator it = m_variables.find(name);
	// Check if it exists to so we can use already allocated memory
	if(it!=m_variables.end()) {
		// Allocate new memory for array if size is different
		if(it->type!=v+FLOAT) {
			if(it->type>=FLOAT) delete [] it->p;
			it->p = new float[v];
		}
		memcpy(it->p, fp, v*sizeof(float));
	} else {
		// Allocate new variable
		SVar var; var.type = FLOAT+v;
		var.p = new float[v];
		memcpy(var.p, fp, v*sizeof(float));
		m_variables.insert(name, var);
	}
}
void Material::setInt(const char* name, int i) {
	SVar v; v.type=INTEGER; v.i = i;
	m_variables[name] = v;
}

float Material::getFloat(const char* name) {
	base::HashMap<SVar>::iterator it = m_variables.find(name);
	if(it!=m_variables.end() && it->type==INTEGER) return it->f;
	else return 0;
}
int Material::getFloatv(const char* name, float* fp) {
	base::HashMap<SVar>::iterator it = m_variables.find(name);
	if(it!=m_variables.end() && it->type>=FLOAT) {
		memcpy(fp, it->p, (it->type-FLOAT)*sizeof(float));
		return it->type-FLOAT;
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
		glBindTexture(GL_TEXTURE_2D, texture[i].unit());
	}
	if(m_textureCount>1) glActiveTexture(GL_TEXTURE0);

	// Bind shader
	m_shader.bind();

	// Set variables
	if(m_shader.ready()) {
		//FIXME const hack for now - until i rewrite shader
		Shader* s = const_cast<Shader*>(&m_shader);

		for(base::HashMap<SVar>::const_iterator it=m_variables.begin(); it!=m_variables.end(); ++it) {
			switch(it->type) {
			case INTEGER:	s->Uniform1i( it.key(), it->i ); break;
			case FLOAT:	 	s->Uniform1f( it.key(), it->f ); break;
			case FLOAT2:    s->Uniform2f( it.key(), it->p[0], it->p[1] ); break;
			case FLOAT3:    s->Uniform3f( it.key(), it->p[0], it->p[1], it->p[2] ); break;
			case FLOAT4:    s->Uniform4f( it.key(), it->p[0], it->p[1], it->p[2], it->p[4] ); break;
			default: s->Uniformfv( it.key(), it->type-FLOAT, it->p ); break;
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







