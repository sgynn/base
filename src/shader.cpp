#define GL_GLEXT_PROTOTYPES
#include "base/opengl.h"

#include "base/shader.h"
#include <iostream>
#include <string.h>
#include "base/file.h"

//Extensions
#ifndef GL_VERSION_2_0
#define APIENTRYP *
typedef char GLchar;	/* native character */

#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_INFO_LOG_LENGTH                0x8B84
#define GL_SHADING_LANGUAGE_VERSION       0x8B8C

typedef GLuint (APIENTRYP PFNGLCREATESHADERPROC) (GLenum type);
typedef void (APIENTRYP PFNGLSHADERSOURCEPROC) (GLuint shader, GLsizei count, const GLchar* *string, const GLint *length);
typedef void (APIENTRYP PFNGLCOMPILESHADERPROC) (GLuint shader);
typedef void (APIENTRYP PFNGLCOMPILESHADERPROC) (GLuint shader);
typedef GLuint (APIENTRYP PFNGLCREATEPROGRAMPROC) (void);
typedef void (APIENTRYP PFNGLATTACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (APIENTRYP PFNGLLINKPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLDELETESHADERPROC) (GLuint shader);
typedef void (APIENTRYP PFNGLDELETEPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLUSEPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLGETPROGRAMIVPROC) (GLuint program, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETSHADERIVPROC) (GLuint shader, GLenum pname, GLint *params);
typedef GLboolean (APIENTRYP PFNGLISPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLGETPROGRAMINFOLOGPROC) (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRYP PFNGLGETSHADERINFOLOGPROC) (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef GLint (APIENTRYP PFNGLGETUNIFORMLOCATIONPROC) (GLuint program, const GLchar *name);
typedef GLint (APIENTRYP PFNGLGETATTRIBLOCATIONPROC) (GLuint program, const GLchar *name);

typedef void (APIENTRYP PFNGLUNIFORM1IVPROC) (GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRYP PFNGLUNIFORM1IPROC) (GLint location, GLint v0);
typedef void (APIENTRYP PFNGLUNIFORM2IPROC) (GLint location, GLint v0, GLint v1);
typedef void (APIENTRYP PFNGLUNIFORM3IPROC) (GLint location, GLint v0, GLint v1, GLint v2);
typedef void (APIENTRYP PFNGLUNIFORM4IPROC) (GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
typedef void (APIENTRYP PFNGLUNIFORM1FVPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRYP PFNGLUNIFORM1FPROC) (GLint location, GLfloat v0);
typedef void (APIENTRYP PFNGLUNIFORM2FPROC) (GLint location, GLfloat v0, GLfloat v1);
typedef void (APIENTRYP PFNGLUNIFORM3FPROC) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void (APIENTRYP PFNGLUNIFORM4FPROC) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);

typedef void (APIENTRYP PFNGLVERTEXATTRIB1FVPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB1FPROC) (GLuint index, GLfloat x);
typedef void (APIENTRYP PFNGLVERTEXATTRIB2FPROC) (GLuint index, GLfloat x, GLfloat y);
typedef void (APIENTRYP PFNGLVERTEXATTRIB3FPROC) (GLuint index, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4FPROC) (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);

typedef void (APIENTRYP PFNGLENABLEVERTEXATTRIBARRAYPROC) (GLuint index);
typedef void (APIENTRYP PFNGLDISABLEVERTEXATTRIBARRAYPROC) (GLuint index);
typedef void (APIENTRYP PFNGLVERTEXATTRIBPOINTERPROC) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);

PFNGLCREATESHADERPROC	glCreateShader		= 0;
PFNGLSHADERSOURCEPROC	glShaderSource		= 0;
PFNGLCOMPILESHADERPROC	glCompileShader		= 0;
PFNGLCREATEPROGRAMPROC	glCreateProgram		= 0;
PFNGLATTACHSHADERPROC	glAttachShader		= 0;
PFNGLLINKPROGRAMPROC	glLinkProgram		= 0;
PFNGLDELETESHADERPROC	glDeleteShader		= 0;
PFNGLDELETEPROGRAMPROC	glDeleteProgram		= 0;
PFNGLUSEPROGRAMPROC	glUseProgram		= 0;
PFNGLGETPROGRAMIVPROC	glGetProgramiv		= 0;
PFNGLGETSHADERIVPROC	glGetShaderiv		= 0;
PFNGLISPROGRAMPROC	glIsProgram		= 0;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog	= 0;
PFNGLGETSHADERINFOLOGPROC  glGetShaderInfoLog	= 0;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation= 0;
PFNGLGETATTRIBLOCATIONPROC  glGetAttribLocation = 0;

PFNGLUNIFORM1IVPROC glUniform1iv = 0;
PFNGLUNIFORM1IPROC  glUniform1i  = 0;
PFNGLUNIFORM2IPROC  glUniform2i  = 0;
PFNGLUNIFORM3IPROC  glUniform3i  = 0;
PFNGLUNIFORM4IPROC  glUniform4i  = 0;
PFNGLUNIFORM1FVPROC glUniform1fv = 0;
PFNGLUNIFORM1FPROC  glUniform1f  = 0;
PFNGLUNIFORM2FPROC  glUniform2f  = 0;
PFNGLUNIFORM3FPROC  glUniform3f  = 0;
PFNGLUNIFORM4FPROC  glUniform4f  = 0;

PFNGLVERTEXATTRIB1FVPROC glVertexAttrib1fv= 0;
PFNGLVERTEXATTRIB1FPROC	 glVertexAttrib1f = 0;
PFNGLVERTEXATTRIB2FPROC	 glVertexAttrib2f = 0;
PFNGLVERTEXATTRIB3FPROC	 glVertexAttrib3f = 0;
PFNGLVERTEXATTRIB4FPROC	 glVertexAttrib4f = 0;

PFNGLENABLEVERTEXATTRIBARRAYPROC  glEnableVertexAttribArray  = 0;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray = 0;
PFNGLVERTEXATTRIBPOINTERPROC	  glVertexAttribPointer	     = 0;
int initialiseShaderExtensions() {
	if(glCreateShader) return 1;
	glCreateShader		= (PFNGLCREATESHADERPROC)	wglGetProcAddress("glCreateShader");
	glShaderSource		= (PFNGLSHADERSOURCEPROC)	wglGetProcAddress("glShaderSource");
	glCompileShader		= (PFNGLCOMPILESHADERPROC)	wglGetProcAddress("glCompileShader");
	glCreateProgram		= (PFNGLCREATEPROGRAMPROC)	wglGetProcAddress("glCreateProgram");
	glAttachShader		= (PFNGLATTACHSHADERPROC)	wglGetProcAddress("glAttachShader");
	glLinkProgram		= (PFNGLLINKPROGRAMPROC)	wglGetProcAddress("glLinkProgram");
	glDeleteShader		= (PFNGLDELETESHADERPROC) 	wglGetProcAddress("glDeleteShader");
	glDeleteProgram		= (PFNGLDELETEPROGRAMPROC)	wglGetProcAddress("glDeleteProgram");
	glUseProgram		= (PFNGLUSEPROGRAMPROC)		wglGetProcAddress("glUseProgram");
	glGetProgramiv		= (PFNGLGETPROGRAMIVPROC)	wglGetProcAddress("glGetProgramiv");
	glGetShaderiv		= (PFNGLGETSHADERIVPROC)	wglGetProcAddress("glGetShaderiv");
	glIsProgram		= (PFNGLISPROGRAMPROC)		wglGetProcAddress("glIsProgram");
	glGetProgramInfoLog	= (PFNGLGETPROGRAMINFOLOGPROC)	wglGetProcAddress("glGetProgramInfoLog");
	glGetShaderInfoLog	= (PFNGLGETSHADERINFOLOGPROC) 	wglGetProcAddress("glGetShaderInfoLog");
	glGetUniformLocation	= (PFNGLGETUNIFORMLOCATIONPROC)	wglGetProcAddress("glGetUniformLocation");
	glGetAttribLocation	= (PFNGLGETATTRIBLOCATIONPROC)	wglGetProcAddress("glGetAttribLocation");
	
	glUniform1iv = (PFNGLUNIFORM1IVPROC)	wglGetProcAddress("glUniform1iv");
	glUniform1i  = (PFNGLUNIFORM1IPROC)	wglGetProcAddress("glUniform1i");
	glUniform2i  = (PFNGLUNIFORM2IPROC)	wglGetProcAddress("glUniform2i");
	glUniform3i  = (PFNGLUNIFORM3IPROC)	wglGetProcAddress("glUniform3i");
	glUniform4i  = (PFNGLUNIFORM4IPROC)	wglGetProcAddress("glUniform4i");
	glUniform1fv = (PFNGLUNIFORM1FVPROC)	wglGetProcAddress("glUniform1fv");
	glUniform1f  = (PFNGLUNIFORM1FPROC)	wglGetProcAddress("glUniform1f");
	glUniform2f  = (PFNGLUNIFORM2FPROC)	wglGetProcAddress("glUniform2f");
	glUniform3f  = (PFNGLUNIFORM3FPROC)	wglGetProcAddress("glUniform3f");
	glUniform4f  = (PFNGLUNIFORM4FPROC)	wglGetProcAddress("glUniform4f");
	
	glVertexAttrib1fv= (PFNGLVERTEXATTRIB1FVPROC)	wglGetProcAddress("glVertexAttrib1fv");
	glVertexAttrib1f = (PFNGLVERTEXATTRIB1FPROC)	wglGetProcAddress("glVertexAttrib1f");
	glVertexAttrib2f = (PFNGLVERTEXATTRIB2FPROC)	wglGetProcAddress("glVertexAttrib2f");
	glVertexAttrib3f = (PFNGLVERTEXATTRIB3FPROC)	wglGetProcAddress("glVertexAttrib3f");
	glVertexAttrib4f = (PFNGLVERTEXATTRIB4FPROC)	wglGetProcAddress("glVertexAttrib4f");
	
	glEnableVertexAttribArray  = (PFNGLENABLEVERTEXATTRIBARRAYPROC)  wglGetProcAddress("glEnableVertexAttribArray");
	glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC) wglGetProcAddress("glDisableVertexAttribArray");
	glVertexAttribPointer	   = (PFNGLVERTEXATTRIBPOINTERPROC)      wglGetProcAddress("glVertexAttribPointer");
	return glCreateShader? 1: 0;
}
#else
int initialiseShaderExtensions() { return 1; }
#endif

using namespace base;

int Shader::s_supported=-1;
const Shader Shader::Null;
unsigned int Shader::s_bound = 0;
Shader Shader::s_currentShader;

int Shader::supported() {
	if(s_supported<0) {
		const GLubyte* version = glGetString(GL_SHADING_LANGUAGE_VERSION);
		printf("GLSL %s\n", version);
		initialiseShaderExtensions();
		s_supported=1; // this should reflect what is supported FIXME
	}
	return s_supported;
}

//// //// //// //// //// //// //// //// Loading / Interface //// //// //// //// //// //// //// ////
Shader Shader::load(const char* filename) {
	VertexShader   vs = loadVertex(filename);
	FragmentShader fs = loadFragment(filename);
	return link(vs,fs);
}
Shader Shader::load(const char* vertFile, const char* fragFile) {
	VertexShader   vs = loadVertex(vertFile);
	FragmentShader fs = loadFragment(fragFile);
	return link(vs,fs);
}
Shader Shader::load(const char* vertFile, const char* geomFile, const char* fragFile) {
	VertexShader   vs = loadVertex(vertFile);
	GeometryShader gs = loadGeometry(geomFile);
	FragmentShader fs = loadFragment(fragFile);
	return link(vs,gs,fs);
}


//// Individual parts ////
VertexShader   Shader::loadVertex(const char* file) {
	const char* code = loadFile(file);
	if(!code) return VertexShader();
	VertexShader vs = createVertex(&code);
	delete [] code;
	return vs;
}
FragmentShader Shader::loadFragment(const char* file) {
	const char* code = loadFile(file);
	if(!code) return FragmentShader();
	FragmentShader fs = createFragment(&code);
	delete [] code;
	return fs;
}
GeometryShader Shader::loadGeometry(const char* file) {
	const char* code = loadFile(file);
	if(!code) return GeometryShader();
	GeometryShader gs = createGeometry(&code);
	delete [] code;
	return gs;
}
//// Create ////
VertexShader   Shader::createVertex(const char** code, int l) {
	VertexShader vs;
	compile(vs, code, 1);
	return vs;
}
FragmentShader Shader::createFragment(const char** code, int l) {
	FragmentShader fs;
	compile(fs, code, 1);
	return fs;
}
GeometryShader Shader::createGeometry(const char** code, int l) {
	GeometryShader gs;
	compile(gs, code, l);
	return gs;
}

//// //// //// //// Load File //// //// //// ////

const char* Shader::loadFile(const char* filename) {
	FindFile(filename);
	FILE *file = fopen(filename, "r");
	if(file) {
		fseek(file, 0, SEEK_END);
		int size = ftell(file);
		rewind(file);
		char* buffer = new char[size+1];
		fread(buffer, 1, size, file);
		buffer[size] = 0;
		fclose(file);
		return buffer;
	} else {
		printf("Failed to load shader file %s\n", filename);
		return 0;
	}
}

//// //// //// //// //// //// //// //// Compile functions //// //// //// //// //// //// //// ////
bool Shader::compile(ShaderBase& shader, const char** code, int l) {
	if(!supported()) return 0;
	int glType[4] = { 0, GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, GL_GEOMETRY_SHADER };

	//Add #define to start of shader code
	char const ** lines = new char const*[l+1];
	switch(shader.m_type) {
		case 1: lines[0] = "#define VERTEX_SHADER\n"; break;
		case 2: lines[0] = "#define FRAGMENT_SHADER\n"; break;
		case 3: lines[0] = "#define GEOMETRY_SHADER\n"; break;
	}
	for(int i=0; i<l; i++) lines[i+1] = code[i];

	// Create and compile shader
	unsigned int s = glCreateShader( glType[shader.m_type] );
	glShaderSource(s,  l+1, lines, NULL);
	glCompileShader(s);
	//Success?
	shader.m_shader = s;
	shader.m_compiled = queryShader(s, GL_COMPILE_STATUS);
	if(!shader.m_compiled) {
		printf("Shader failed to compile\n");
		char buf[1024]; printf("-----------\n%s\n-----------\n", shader.log(buf,1024));
	}
	GL_DEBUG_ERROR;
	delete [] lines;
	return shader.m_compiled;
}

Shader Shader::link(const VertexShader& vs, const FragmentShader& fs) {
	if(!vs.m_compiled || !fs.m_compiled) return Null; // Shaders must be compiled
	Shader s;
	s.m_vertex = vs;
	s.m_fragment = fs;
	s.m_program = glCreateProgram();
	glAttachShader(s.m_program, vs.m_shader);
	glAttachShader(s.m_program, fs.m_shader);
	glLinkProgram(s.m_program);
	s.m_linked = queryProgram(s.m_program, GL_LINK_STATUS);
	if(!s.m_linked) printf("Shader failed to link\n");
	GL_DEBUG_ERROR;
	return s;
}
Shader Shader::link(const VertexShader& vs, const GeometryShader& gs, const FragmentShader& fs) {
	if(!vs.m_compiled || !gs.m_compiled || !fs.m_compiled) return Null; // Shaders must be compiled
	Shader s;
	s.m_vertex = vs;
	s.m_geometry = gs;
	s.m_fragment = fs;
	s.m_program = glCreateProgram();
	glAttachShader(s.m_program, vs.m_shader);
	glAttachShader(s.m_program, gs.m_shader);
	glAttachShader(s.m_program, fs.m_shader);
	glLinkProgram(s.m_program);
	s.m_linked = queryProgram(s.m_program, GL_LINK_STATUS);
	if(!s.m_linked) printf("Shader failed to link\n");
	return s;
}

char* ShaderBase::log(char* buf, int max) const {
	int l = Shader::queryShader(m_shader, GL_INFO_LOG_LENGTH);
	if(Shader::supported()) glGetShaderInfoLog(m_shader, l, &l, buf);
	return buf;
}

int Shader::queryShader(unsigned int shader, int param) {
	int value=0;
	if(supported()) glGetShaderiv( shader, param, &value);
	return value;
}

int Shader::queryProgram(unsigned int shader, int param) {
	int value;
	glGetProgramiv( shader, param, &value);
	return value;
}


//// //// //// //// //// //// //// //// Constructor //// //// //// //// //// //// //// ////

Shader::Shader() : m_linked(0), m_program(0) {};

int Shader::variable(const char* name, VType type) {
	if(!m_linked || s_bound!=m_program) return -1; //Program must be current
	if(m_vars.find(name)!=m_vars.end()) return m_vars[name]; //cached
	int loc = -1;
	if(type==UNIFORM) loc = glGetUniformLocation(m_program, name);
	else loc = glGetAttribLocation(m_program, name);
	m_vars[name]=loc;
	return loc;
}

void Shader::Uniform1i(const char* name, int v1) {
	int loc=variable(name, UNIFORM);
	if(loc>=0) glUniform1i(loc, v1);
}
void Shader::Uniform2i(const char* name, int v1, int v2) {
	int loc=variable(name, UNIFORM);
	if(loc>=0) glUniform2i(loc, v1, v2);
}
void Shader::Uniform3i(const char* name, int v1, int v2, int v3) {
	int loc=variable(name, UNIFORM);
	if(loc>=0) glUniform3i(loc, v1, v2, v3);
}
void Shader::Uniform4i(const char* name, int v1, int v2, int v3, int v4) {
	int loc=variable(name, UNIFORM);
	if(loc>=0) glUniform4i(loc, v1, v2, v3, v4);
}
void Shader::Uniformiv(const char* name, int size, const int *v) {
	int loc=variable(name, UNIFORM);
	if(loc>=0) glUniform1iv(loc, size, v); //glUniform2iv etc exist but i think they are not really needed.
}

void Shader::Uniform1f(const char* name, float v1) {
	int loc=variable(name, UNIFORM);
	if(loc>=0) glUniform1f(loc, v1);
}
void Shader::Uniform2f(const char* name, float v1, float v2) {
	int loc=variable(name, UNIFORM);
	if(loc>=0) glUniform2f(loc, v1, v2);
}
void Shader::Uniform3f(const char* name, float v1, float v2, float v3) {
	int loc=variable(name, UNIFORM);
	if(loc>=0) glUniform3f(loc, v1, v2, v3);
}
void Shader::Uniform4f(const char* name, float v1, float v2, float v3, float v4) {
	int loc=variable(name, UNIFORM);
	if(loc>=0) glUniform4f(loc, v1, v2, v3, v4);
}
void Shader::Uniformfv(const char* name, int size, const float *v) {
	int loc=variable(name, UNIFORM);
	if(loc>=0) glUniform1fv(loc, size, v);
}

void Shader::Attribute1f(const char* name, float v1) {
	int loc=variable(name, ATTRIBUTE);
	if(loc>=0) glVertexAttrib1f(loc, v1);
}
void Shader::Attribute2f(const char* name, float v1, float v2) {
	int loc=variable(name, ATTRIBUTE);
	if(loc>=0) glVertexAttrib2f(loc, v1, v2);
}
void Shader::Attribute3f(const char* name, float v1, float v2, float v3) {
	int loc=variable(name, ATTRIBUTE);
	if(loc>=0) glVertexAttrib3f(loc, v1, v2, v3);
}
void Shader::Attribute4f(const char* name, float v1, float v2, float v3, float v4) {
	int loc=variable(name, ATTRIBUTE);
	if(loc>=0) glVertexAttrib4f(loc, v1, v2, v3, v4);
}


void Shader::EnableAttributeArray(const char* name) {
	int loc = variable(name, ATTRIBUTE);
	if(loc) glEnableVertexAttribArray(loc);
}
void Shader::DisableAttributeArray(const char* name) {
	int loc = variable(name, ATTRIBUTE);
	if(loc) glDisableVertexAttribArray(loc);
}
void Shader::AttributePointer(const char *name, int size, int type, bool normalised, size_t stride, const void *pointer) {
	int loc = variable(name, ATTRIBUTE);
	if(loc>=0) glVertexAttribPointer(loc, size, type, normalised, stride, pointer);	
}

void Shader::bind() const {
	if(supported() && m_program != s_bound) {
		glUseProgram(m_program);
		s_bound = m_program;
		s_currentShader = *this;
	}
}

