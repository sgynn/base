//#define GL_GLEXT_PROTOTYPES
#include <base/opengl.h>
#include <base/shader.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>

//Extensions
#ifdef WIN32
#ifndef APIENTRYP
#define APIENTRYP __stdcall *
#endif

typedef char GLchar;	/* native character */

#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_GEOMETRY_SHADER                0x8DD9
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_INFO_LOG_LENGTH                0x8B84
#define GL_SHADING_LANGUAGE_VERSION       0x8B8C

/*
typedef GLuint (APIENTRYP PFNGLCREATESHADERPROC) (GLenum type);
typedef void (APIENTRYP PFNGLSHADERSOURCEPROC) (GLuint shader, GLsizei count, const GLchar* *string, const GLint *length);
typedef void (APIENTRYP PFNGLCOMPILESHADERPROC) (GLuint shader);
typedef GLuint (APIENTRYP PFNGLCREATEPROGRAMPROC) (void);
typedef void (APIENTRYP PFNGLATTACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (APIENTRYP PFNGLLINKPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLDELETESHADERPROC) (GLuint shader);
typedef void (APIENTRYP PFNGLDELETEPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLUSEPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLGETPROGRAMIVPROC) (GLuint program, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETSHADERIVPROC) (GLuint shader, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETPROGRAMINFOLOGPROC) (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRYP PFNGLGETSHADERINFOLOGPROC) (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRYP PFNGLGETATTACHEDSHADERSPROC) (GLuint shader, GLsizei maxCount, GLsizei *count, GLuint* shaders);

typedef GLint (APIENTRYP PFNGLGETUNIFORMLOCATIONPROC) (GLuint program, const GLchar *name);
typedef GLint (APIENTRYP PFNGLGETATTRIBLOCATIONPROC) (GLuint program, const GLchar *name);
typedef GLint (APIENTRYP PFNGLBINDATTRIBLOCATIONPROC) (GLuint program, GLuint loc, const GLchar *name);
typedef GLint (APIENTRYP PFNGLBINDFRAGDATALOCATIONPROC) (GLuint program, GLuint loc, const GLchar *name);

typedef void (APIENTRYP PFNGLUNIFORM1IVPROC) (GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRYP PFNGLUNIFORM2IVPROC) (GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRYP PFNGLUNIFORM3IVPROC) (GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRYP PFNGLUNIFORM4IVPROC) (GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRYP PFNGLUNIFORM1FVPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRYP PFNGLUNIFORM2FVPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRYP PFNGLUNIFORM3FVPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRYP PFNGLUNIFORM4FVPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRYP PFNGLUNIFORMMATRIX2FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRYP PFNGLUNIFORMMATRIX3FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRYP PFNGLUNIFORMMATRIX4FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRYP PFNGLUNIFORMMATRIX3x4FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);

typedef void (APIENTRYP PFNGLENABLEVERTEXATTRIBARRAYPROC) (GLuint index);
typedef void (APIENTRYP PFNGLDISABLEVERTEXATTRIBARRAYPROC) (GLuint index);
typedef void (APIENTRYP PFNGLVERTEXATTRIBPOINTERPROC) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
*/

static PFNGLCREATESHADERPROC   glCreateShader		= 0;
static PFNGLSHADERSOURCEPROC   glShaderSource		= 0;
static PFNGLCOMPILESHADERPROC  glCompileShader		= 0;
static PFNGLCREATEPROGRAMPROC  glCreateProgram		= 0;
static PFNGLATTACHSHADERPROC   glAttachShader		= 0;
static PFNGLLINKPROGRAMPROC    glLinkProgram		= 0;
static PFNGLDELETESHADERPROC   glDeleteShader		= 0;
static PFNGLDELETEPROGRAMPROC  glDeleteProgram		= 0;
static PFNGLUSEPROGRAMPROC     glUseProgram		= 0;
static PFNGLGETPROGRAMIVPROC   glGetProgramiv		= 0;
static PFNGLGETSHADERIVPROC    glGetShaderiv		= 0;
static PFNGLGETPROGRAMINFOLOGPROC  glGetProgramInfoLog	= 0;
static PFNGLGETSHADERINFOLOGPROC   glGetShaderInfoLog	= 0;
static PFNGLGETATTACHEDSHADERSPROC glGetAttachedShaders = 0;
static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation= 0;
static PFNGLGETATTRIBLOCATIONPROC  glGetAttribLocation = 0;
static PFNGLBINDFRAGDATALOCATIONPROC glBindFragDataLocation = 0;
static PFNGLBINDATTRIBLOCATIONPROC   glBindAttribLocation = 0;

static PFNGLUNIFORM1IVPROC         glUniform1iv = 0;
static PFNGLUNIFORM2IVPROC         glUniform2iv = 0;
static PFNGLUNIFORM3IVPROC         glUniform3iv = 0;
static PFNGLUNIFORM4IVPROC         glUniform4iv = 0;
static PFNGLUNIFORM1FVPROC         glUniform1fv = 0;
static PFNGLUNIFORM2FVPROC         glUniform2fv = 0;
static PFNGLUNIFORM3FVPROC         glUniform3fv = 0;
static PFNGLUNIFORM4FVPROC         glUniform4fv = 0;
static PFNGLUNIFORMMATRIX2FVPROC   glUniformMatrix2fv = 0;
static PFNGLUNIFORMMATRIX3FVPROC   glUniformMatrix3fv = 0;
static PFNGLUNIFORMMATRIX4FVPROC   glUniformMatrix4fv = 0;
static PFNGLUNIFORMMATRIX3X4FVPROC glUniformMatrix3x4fv = 0;

PFNGLENABLEVERTEXATTRIBARRAYPROC   glEnableVertexAttribArray  = 0;
PFNGLDISABLEVERTEXATTRIBARRAYPROC  glDisableVertexAttribArray = 0;
PFNGLVERTEXATTRIBPOINTERPROC	   glVertexAttribPointer	     = 0;
static PFNGLVERTEXATTRIBIPOINTERPROC	  glVertexAttribIPointer     = 0;
PFNGLVERTEXATTRIBDIVISORPROC              glVertexAttribDivisor      = 0;

PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = 0;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray = 0;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = 0;
PFNGLDRAWELEMENTSINSTANCEDPROC glDrawElementsInstanced = 0;
PFNGLDRAWARRAYSINSTANCEDPROC glDrawArraysInstanced = 0;

static int initialiseShaderExtensions() {
	if(glCreateShader) return 1;
	printf("SETUP OPENGL EXTENSIONS\n");
	glCreateShader       = (PFNGLCREATESHADERPROC)   wglGetProcAddress("glCreateShader");
	glShaderSource       = (PFNGLSHADERSOURCEPROC)   wglGetProcAddress("glShaderSource");
	glCompileShader      = (PFNGLCOMPILESHADERPROC)  wglGetProcAddress("glCompileShader");
	glCreateProgram      = (PFNGLCREATEPROGRAMPROC)  wglGetProcAddress("glCreateProgram");
	glAttachShader       = (PFNGLATTACHSHADERPROC)   wglGetProcAddress("glAttachShader");
	glLinkProgram        = (PFNGLLINKPROGRAMPROC)    wglGetProcAddress("glLinkProgram");
	glDeleteShader       = (PFNGLDELETESHADERPROC)   wglGetProcAddress("glDeleteShader");
	glDeleteProgram      = (PFNGLDELETEPROGRAMPROC)  wglGetProcAddress("glDeleteProgram");
	glUseProgram         = (PFNGLUSEPROGRAMPROC)     wglGetProcAddress("glUseProgram");
	glGetProgramiv       = (PFNGLGETPROGRAMIVPROC)   wglGetProcAddress("glGetProgramiv");
	glGetShaderiv        = (PFNGLGETSHADERIVPROC)    wglGetProcAddress("glGetShaderiv");
	glGetProgramInfoLog  = (PFNGLGETPROGRAMINFOLOGPROC)  wglGetProcAddress("glGetProgramInfoLog");
	glGetShaderInfoLog   = (PFNGLGETSHADERINFOLOGPROC)   wglGetProcAddress("glGetShaderInfoLog");
	glGetAttachedShaders = (PFNGLGETATTACHEDSHADERSPROC) wglGetProcAddress("glGetAttachedShaders");
	glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC) wglGetProcAddress("glGetUniformLocation");
	glGetAttribLocation  = (PFNGLGETATTRIBLOCATIONPROC)  wglGetProcAddress("glGetAttribLocation");
	glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC) wglGetProcAddress("glBindAttribLocation");
	glBindFragDataLocation = (PFNGLBINDFRAGDATALOCATIONPROC) wglGetProcAddress("glBindFragDataLocation");
	
	glUniform1iv = (PFNGLUNIFORM1IVPROC)	wglGetProcAddress("glUniform1iv");
	glUniform2iv = (PFNGLUNIFORM2IVPROC)	wglGetProcAddress("glUniform2iv");
	glUniform3iv = (PFNGLUNIFORM3IVPROC)	wglGetProcAddress("glUniform3iv");
	glUniform4iv = (PFNGLUNIFORM4IVPROC)	wglGetProcAddress("glUniform4iv");
	glUniform1fv = (PFNGLUNIFORM1FVPROC)	wglGetProcAddress("glUniform1fv");
	glUniform2fv = (PFNGLUNIFORM2FVPROC)	wglGetProcAddress("glUniform2fv");
	glUniform3fv = (PFNGLUNIFORM3FVPROC)	wglGetProcAddress("glUniform3fv");
	glUniform4fv = (PFNGLUNIFORM4FVPROC)	wglGetProcAddress("glUniform4fv");
	glUniformMatrix2fv = (PFNGLUNIFORMMATRIX2FVPROC)	wglGetProcAddress("glUniformMatrix2fv");
	glUniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC)	wglGetProcAddress("glUniformMatrix3fv");
	glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)	wglGetProcAddress("glUniformMatrix4fv");
	glUniformMatrix3x4fv = (PFNGLUNIFORMMATRIX3X4FVPROC)	wglGetProcAddress("glUniformMatrix3x4fv");
	
	glEnableVertexAttribArray  = (PFNGLENABLEVERTEXATTRIBARRAYPROC)  wglGetProcAddress("glEnableVertexAttribArray");
	glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC) wglGetProcAddress("glDisableVertexAttribArray");
	glVertexAttribPointer	   = (PFNGLVERTEXATTRIBPOINTERPROC)      wglGetProcAddress("glVertexAttribPointer");
	glVertexAttribIPointer	   = (PFNGLVERTEXATTRIBIPOINTERPROC)     wglGetProcAddress("glVertexAttribIPointer");
	glVertexAttribDivisor 	   = (PFNGLVERTEXATTRIBDIVISORPROC)      wglGetProcAddress("glVertexAttribDivisor");

	glGenVertexArrays 	   = (PFNGLGENVERTEXARRAYSPROC)      wglGetProcAddress("glGenVertexArrays");
	glBindVertexArray 	   = (PFNGLBINDVERTEXARRAYPROC)      wglGetProcAddress("glBindVertexArray");
	glDeleteVertexArrays   = (PFNGLDELETEVERTEXARRAYSPROC)      wglGetProcAddress("glDeleteVertexArrays");

	glDrawElementsInstanced   = (PFNGLDRAWELEMENTSINSTANCEDPROC)      wglGetProcAddress("glDrawElementsInstanced");
	glDrawArraysInstanced   = (PFNGLDRAWARRAYSINSTANCEDPROC)      wglGetProcAddress("glDrawArraysInstanced");
	return glCreateShader? 1: 0;
}
#else
static int initialiseShaderExtensions() { return 1; }
#endif


// ============================================================================================================ //


using namespace base;

ShaderPart::ShaderPart(ShaderType t, const char* src, const char* def) : m_type(t), m_object(0), m_compiled(0), m_changed(0), m_source(0), m_sourceRef(0) {
	if(src) setSource(src);
	if(def) setDefines(def);
}
ShaderPart::ShaderPart(const ShaderPart& s) : m_type(s.m_type), m_object(0), m_compiled(false), m_changed(true), m_source(s.m_source), m_sourceRef(s.m_sourceRef) {
	if(m_source) ++*m_sourceRef;
	for(size_t i=0; i<s.m_defines.size(); ++i) {
		m_defines.push_back( strdup(m_defines[i]) );
	}
}
ShaderPart::ShaderPart(ShaderPart&& s) : m_type(s.m_type), m_object(s.m_object), m_compiled(s.m_compiled), m_changed(s.m_changed), m_source(s.m_source), m_sourceRef(s.m_sourceRef), m_defines(s.m_defines) {
}
ShaderPart::~ShaderPart() {
	dropSource();
	for(size_t i=0; i<m_defines.size(); ++i) free(m_defines[i]);
}

ShaderPart& ShaderPart::operator=(ShaderPart& o) {
	dropSource();
	for(char* def: m_defines) free(def);
	m_defines.clear();
	m_type = o.m_type;
	m_object = 0;
	m_source = o.m_source;
	m_sourceRef = o.m_sourceRef;
	if(m_source) ++*m_sourceRef;
	for(char* def: o.m_defines) m_defines.push_back(strdup(def));
	m_changed = o.m_changed;
	return *this;
}

void ShaderPart::dropSource() {
	if(m_source && --*m_sourceRef==0) {
		free(m_source);
		delete m_sourceRef;
		m_source = 0;
	}
}

void ShaderPart::setSource(const char* s) {
	dropSource();
	if(s) {
		m_source = strdup(s);
		m_sourceRef = new int(1);
	}
	m_changed = true;
}

void ShaderPart::define(const char* def) {
	undef(def);
	m_defines.push_back( strdup(def) );
	m_changed = true;
}

void ShaderPart::undef(const char* def) {
	int i = getDefineIndex(def);
	if(i>=0) {
		free(m_defines[i]);
		m_defines[i] = m_defines.back();
		m_defines.pop_back();
		m_changed = true;
	}
}

bool ShaderPart::isDefined(const char* def) const {
	return getDefineIndex(def)>=0;
}

int ShaderPart::getDefineIndex(const char* def) const {
	int len = strlen(def);
	for(size_t i=0; i<m_defines.size(); ++i) {
		if(strncmp(m_defines[i], def, len)==0) {
			char c = m_defines[i][len];
			if(c==0 || c==' ' || c=='=' || c=='\t') return i;
		}
	}
	return -1;
}

void ShaderPart::setDefines(const char* def) {
	for(size_t i=0; i<m_defines.size(); ++i) free(m_defines[i]);
	m_defines.clear();
	if(!def || !def[0]) return;
	
	char buffer[512];
	strcpy(buffer, def);
	char* s = buffer;
	for(char* c=buffer; *c; ++c) {
		if(*c==',' || *c==';') {
			*c = 0;
			if(s<c) m_defines.push_back( strdup(s) );
			s = c+1;
		}
		else if(*c==' ' && s==c) ++s;
	}
	if(*s) m_defines.push_back( strdup(s) );
	m_changed = true;
}

// ------------------------------------------------------------ //

bool ShaderPart::compile() {
	const int glType[] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, GL_GEOMETRY_SHADER, GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER };
	m_changed = false;
	m_compiled = false;

	if(!m_source) return false;
	
	// Preprocess source
	int version = 0;
	char* temp = 0;
	preprocess(m_source, m_type, version, temp);

	// Automatic code
	char header[2048];
	int k = 0;
	#ifdef EMSCRIPTEN
	if(version>0) k=sprintf(header, "#version 300 es\n");
	k += sprintf(header+k, "precision highp float;\n");
	#else
	if(version>0) k=sprintf(header, "#version %d\n", version);
	#endif
	// Extra defines
	for(size_t i=0; i<m_defines.size(); ++i) {
		k += sprintf(header+k, "#define %s\n", m_defines[i]);
	}
	// Remove any = from defines
	for(int i=0; i<k; ++i) if(header[i]=='=') header[i]=' ';
	
	// Deprecated shader type defines
	switch(m_type) {
	case VERTEX_SHADER:   strcpy(header+k, "#define VERTEX_SHADER\n"); break;
	case FRAGMENT_SHADER: strcpy(header+k, "#define FRAGMENT_SHADER\n"); break;
	case GEOMETRY_SHADER: strcpy(header+k, "#define GEOMETRY_SHADER\n"); break;
	default: return false; // Tesselation shaders not yet implemented
	}

	// Create shader
	if(m_object==0) m_object = glCreateShader( glType[m_type] );
	if(m_object==0) return false;
	
	// Set source
	const char* src[2] = { header, temp? temp: m_source };
	glShaderSource(m_object, 2, src, NULL);

	// Compile
	glCompileShader(m_object);
	glGetShaderiv(m_object, GL_COMPILE_STATUS, &m_compiled);

	GL_CHECK_ERROR;
	if(temp) delete [] temp;
	return m_compiled;
}

int ShaderPart::getLog(char* out, int len) const {
	out[0] = 0;
	if(!m_object) return sprintf(out, "Error: program not created\n");
	int logLength = 0;
	glGetShaderiv(m_object, GL_INFO_LOG_LENGTH, &logLength);
	glGetShaderInfoLog(m_object, len, &logLength, out);
	GL_CHECK_ERROR;
	return logLength<len? logLength: len;
}

// ---------------------------------------------------------------------------------------- //

static const char* findDirective(const char* src, const char* directive) {
	while(true) {
		const char* result = strstr(src, directive);
		if(!result) return 0;
		// Validate result
		bool valid = true;
		for(const char* c=result-1; valid && c>=src && *c!='\n'; --c) {
			if(*c != ' ' && *c != '\t') valid = false;	// only allow whitespace before directive
		}
		if(valid) return result;
		src = result + 4;
	}
	return 0; // compiler
}

bool ShaderPart::preprocess(const char* code, ShaderType type, int& version, char*& output) {
	// Find version directive
	const char* versionDirective = findDirective(code, "#version");
	if(versionDirective) {
		const char* c = versionDirective + 8;
		while(*c==' ' || *c=='\t') ++c;
		version = atoi(c);
	}

	// Find pragmas: vertex_shader, fragment_shader, geometry_shader
	struct Pragma { const char* loc; ShaderType type; int line; };
	Pragma pragmas[32];
	int count = 0;
	const char* pragma = code;
	while(pragma) {
		pragma = strstr(pragma, "#pragma");
		if(pragma && (pragma[7]==' ' || pragma[7]=='\t')) {
			pragmas[count].loc = pragma;
			pragma += 8;
			while(*pragma==' ' || *pragma=='\t') ++pragma;
			// Determine type
			if(     strncmp(pragma, "vertex_shader",  13)==0) pragmas[count].type = VERTEX_SHADER;
			else if(strncmp(pragma, "fragment_shader",15)==0) pragmas[count].type = FRAGMENT_SHADER;
			else if(strncmp(pragma, "geometry_shader",15)==0) pragmas[count].type = GEOMETRY_SHADER;
			else continue;	// Something else
			// Get line
			pragmas[count].line = count>0? pragmas[count-1].line: 0;
			const char* anchor = count>0? pragmas[count-1].loc: code;
			for(const char* c=pragma; c>=anchor; --c) {
				if(*c == '\n') ++pragmas[count].line;
			}
			++count;
		}
	}

	// Nothing to change
	if(!versionDirective && count==0) return false;

	// Build new source
	size_t size = strlen(code);
	pragmas[count].loc = code + size;
	for(int i=0; i<count; ++i) {
		if(pragmas[i].type != type) size -= pragmas[i+1].loc - pragmas[i].loc;
		else size += 8;	// just in case
	}

	output = new char[size+1];
	size_t commonSize = count>0? pragmas[0].loc - code: size;
	memcpy(output, code, commonSize);
	if(versionDirective) {
		char* c = output + (versionDirective - code);
		memcpy(c, "//  --  ", 8);
	}

	char* out = output + commonSize;
	for(int i=0; i<count; ++i) {
		if(pragmas[i].type == type) {
			sprintf(out, "#line %d\n//", pragmas[i].line);
			out += strlen(out);
			size_t length = pragmas[i+1].loc - pragmas[i].loc;
			memcpy(out, pragmas[i].loc + 7, length - 7);
			out += length - 7;
		}
	}
	*out = 0; // ensure null terminated
	return true;
}


// ======================================================================================================= //


int Shader::s_supported=-1;
const Shader Shader::Null;
const Shader* Shader::s_currentShader;

int Shader::getSupportedVersion() {
	if(s_supported<0) {
		const GLubyte* version = glGetString(GL_SHADING_LANGUAGE_VERSION);
		s_supported = atoi((const char*)version);
		initialiseShaderExtensions();
	}
	return s_supported;
}

// ----------------------------------------------------------------------------------------- //

Shader::Shader() : m_entry{0,0,0,0,0}, m_object(0), m_linked(0), m_changed(0) {
}
Shader::Shader(Shader&& s) : m_shaders(s.m_shaders), m_object(s.m_object), m_linked(s.m_linked), m_changed(s.m_changed) {
}
Shader::~Shader() {
}

Shader Shader::clone() const {
	Shader copy;
	for(size_t i=0; i<m_shaders.size(); ++i) {
		copy.attach( new ShaderPart(*m_shaders[i]) );
	}
	return copy;
}

void Shader::setDefines(const char* def) {
	for(size_t i=0; i<m_shaders.size(); ++i) {
		m_shaders[i]->setDefines(def);
		m_changed = true;
	}
}

void Shader::attach(ShaderPart* p) {
	m_shaders.push_back(p);
	m_changed = true;
}

bool Shader::isCompiled() const {
	return m_linked && m_object;
}

bool Shader::needsRecompile() const {
	if(m_changed) return true;
	if(!m_linked) return false;	// failed
	for(size_t i=0; i<m_shaders.size(); ++i) {
		if(m_shaders[i]->m_changed) return true;
	}
	return false;
}

bool Shader::compile() {
	bool good = true;
	m_linked = 0;
	m_changed = false;
	for(size_t i=0; i<m_shaders.size(); ++i) {
		if(m_shaders[i]->m_changed) good &= m_shaders[i]->compile();
	}
	if(!good) return false;

	if(!m_object) m_object = glCreateProgram();
	if(!m_object) return false;
	GL_CHECK_ERROR;

	GLsizei count;
	GLuint attached[8];
	glGetAttachedShaders(m_object, 8, &count, attached);

	for(size_t i=0; i<m_shaders.size(); ++i) {
		bool alreadyAttached = false;
		for(int j=0; j<count; ++j) if(attached[j] == m_shaders[i]->m_object) alreadyAttached=true;
		if(!alreadyAttached) glAttachShader(m_object, m_shaders[i]->m_object);
		GL_CHECK_ERROR;
	}
	glLinkProgram(m_object);
	GL_CHECK_ERROR;

	glGetProgramiv(m_object, GL_LINK_STATUS, &m_linked);
	GL_CHECK_ERROR;
	return m_linked;
}

void Shader::bind() const {
	if(!m_linked) {
		glUseProgram(0);
		s_currentShader = &Null;
	} else if(s_currentShader != this) {
		glUseProgram(m_object);
		s_currentShader = this;
	}
}

int Shader::getLog(char* buffer, int size) const {
	buffer[0] = 0;
	if(m_linked) return 0;
	int r = 0;

	// compile logs
	for(size_t i=0; i<m_shaders.size() && r<size; ++i) {
		if(!m_shaders[i]->m_compiled) {
			r += m_shaders[i]->getLog(buffer + r, size - r);
		}
	}
	// linker log
	int logLength = 0;
	glGetProgramiv(m_object, GL_INFO_LOG_LENGTH, &logLength);
	glGetProgramInfoLog(m_object, size-r, &logLength, buffer+r);
	r += logLength<size-r? logLength: size-r;
	GL_CHECK_ERROR;

	buffer[r] = 0;
	return r;
}


// -------------------------------------------------------------------------------------------- //

void Shader::bindAttributeLocation(const char* name, int index) {
	if(!m_object) m_object = glCreateProgram();
	glBindAttribLocation(m_object, index, name);
	m_linked = false;
	m_changed = true;
	GL_CHECK_ERROR;
}
int Shader::getAttributeLocation(const char* name) const {
	if(!m_object) return 0;
	return glGetAttribLocation(m_object, name);
}
void Shader::setAttributePointer(int loc, int size, int type, size_t stride, AttributeMode m, const void* ptr) const {
	switch(m) {
	case SA_FLOAT:      glVertexAttribPointer(loc, size, type, false, stride, ptr); break;
	case SA_NORMALISED: glVertexAttribPointer(loc, size, type, true, stride, ptr); break;
	case SA_INTEGER:    glVertexAttribIPointer(loc, size, type, stride, ptr); break;
	case SA_DOUBLE:     /*glVertexAttribLPointer(loc, size, type, stride, ptr);*/ break;
	}
}
void Shader::enableAttributeArray(int loc) {
	glEnableVertexAttribArray(loc);
}
void Shader::disableAttributeArray(int loc) {
	glDisableVertexAttribArray(loc);
}


// -------------------------------------------------------------------------------------------- //

void Shader::bindOutput(const char* name, int index) {
	#ifndef EMSCRIPTEN
	glBindFragDataLocation(m_object, index, name);
	m_linked = false;
	GL_CHECK_ERROR;
	#endif
}

// -------------------------------------------------------------------------------------------- //

int Shader::getUniformLocation(const char* name) const {
	return glGetUniformLocation(m_object, name);
}

void Shader::setUniform1(int loc, int count, const int* data) const { glUniform1iv(loc, count, data); }
void Shader::setUniform2(int loc, int count, const int* data) const { glUniform2iv(loc, count, data); }
void Shader::setUniform3(int loc, int count, const int* data) const { glUniform3iv(loc, count, data); }
void Shader::setUniform4(int loc, int count, const int* data) const { glUniform4iv(loc, count, data); }

void Shader::setUniform1(int loc, int count, const float* data) const { glUniform1fv(loc, count, data); }
void Shader::setUniform2(int loc, int count, const float* data) const { glUniform2fv(loc, count, data); }
void Shader::setUniform3(int loc, int count, const float* data) const { glUniform3fv(loc, count, data); }
void Shader::setUniform4(int loc, int count, const float* data) const { glUniform4fv(loc, count, data); }

void Shader::setMatrix2(int loc, int count, const float* data) const {
	glUniformMatrix2fv(loc, count, false, data);
}
void Shader::setMatrix3(int loc, int count, const float* data) const {
	glUniformMatrix3fv(loc, count, false, data);
}
void Shader::setMatrix4(int loc, int count, const float* data) const {
	glUniformMatrix4fv(loc, count, false, data);
}
void Shader::setMatrix3x4(int loc, int count, const float* data) const {
	glUniformMatrix3x4fv(loc, count, false, data);
}



