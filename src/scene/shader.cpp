//#define GL_GLEXT_PROTOTYPES
#include <base/opengl.h>
#include <base/shader.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>

using namespace base;

ShaderPart::ShaderPart(ShaderType t, const char* src, const char* def) : m_type(t), m_object(0), m_compiled(0), m_changed(0), m_source(0), m_sourceRef(0) {
	if(src) setSource(src);
	if(def) setDefines(def);
}
ShaderPart::ShaderPart(const ShaderPart& s) : m_type(s.m_type), m_object(0), m_compiled(false), m_changed(true), m_source(s.m_source), m_sourceRef(s.m_sourceRef) {
	if(m_source) ++*m_sourceRef;
	for(const char* def : s.m_defines) {
		m_defines.push_back( strdup(def) );
	}
}
ShaderPart::ShaderPart(ShaderPart&& s) : m_type(s.m_type), m_object(s.m_object), m_compiled(s.m_compiled), m_changed(s.m_changed), m_source(s.m_source), m_sourceRef(s.m_sourceRef), m_defines(s.m_defines) {
	s.m_defines.clear();
	s.m_source = 0;
}
ShaderPart::~ShaderPart() {
	dropSource();
	for(char* def: m_defines) free(def);
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
	for(char* def: m_defines) free(def);
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
	for(const char* def: m_defines) {
		k += sprintf(header+k, "#define %s\n", def);
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
	}
	return s_supported;
}

// ----------------------------------------------------------------------------------------- //

Shader* Shader::create(const char* vsrc, const char* fsrc) {
	if(!vsrc || !fsrc) return 0;
	ShaderPart* vs = new ShaderPart(VERTEX_SHADER, vsrc);
	ShaderPart* fs = new ShaderPart(FRAGMENT_SHADER, fsrc);
	Shader* shader = new Shader();
	shader->attach(vs);
	shader->attach(fs);
	shader->bindAttributeLocation( "vertex",   0 );
	shader->bindAttributeLocation( "normal",   1 );
	shader->bindAttributeLocation( "tangent",  2 );
	shader->bindAttributeLocation( "texCoord", 3 );
	shader->bindAttributeLocation( "colour",   4 );
	shader->compile();
	if(!shader->isCompiled()) {
		char buf[2048];
		shader->getLog(buf, 2048);
		puts("Error: Failed to compile internal shader:\n");
		puts(buf);
	}
	return shader;

}

// ----------------------------------------------------------------------------------------- //

Shader::Shader() : m_entry{0,0,0,0,0}, m_object(0), m_linked(0), m_changed(0) {
}
Shader::Shader(Shader&& s) : m_shaders(s.m_shaders), m_object(s.m_object), m_linked(s.m_linked), m_changed(s.m_changed) {
}
Shader::~Shader() {
}

Shader* Shader::clone() const {
	Shader* copy = new Shader();
	for(ShaderPart* part : m_shaders) {
		copy->attach( new ShaderPart(*part) );
	}
	return copy;
}

void Shader::setDefines(const char* def) {
	for(ShaderPart* part : m_shaders) {
		part->setDefines(def);
		m_changed = true;
	}
}

void Shader::define(const char* def) {
	for(ShaderPart* part : m_shaders) {
		part->define(def);
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
	for(ShaderPart* part : m_shaders) {
		if(part->m_changed) return true;
	}
	return false;
}

bool Shader::compile() {
	bool good = true;
	m_linked = 0;
	m_changed = false;
	for(ShaderPart* part : m_shaders) {
		if(part->m_changed) good &= part->compile();
	}
	if(!good) return false;

	if(!m_object) m_object = glCreateProgram();
	if(!m_object) return false;
	GL_CHECK_ERROR;

	GLsizei count;
	GLuint attached[8];
	glGetAttachedShaders(m_object, 8, &count, attached);

	for(ShaderPart* part : m_shaders) {
		bool alreadyAttached = false;
		for(int j=0; j<count; ++j) if(attached[j] == part->m_object) alreadyAttached=true;
		if(!alreadyAttached) glAttachShader(m_object, part->m_object);
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


Shader::UniformList Shader::getUniforms() const {
	int count = 0;
	GLsizei length;
	GLenum type;
	GLint size;
	UniformList list;
	glGetProgramiv(m_object, GL_ACTIVE_UNIFORMS, &count);
	for(int i=0; i<count; ++i) {
		list.emplace_back();
		glGetActiveUniform(m_object, (GLuint)i, 32, &length, &size, &type, list.back().name);
		list.back().type = type;
		list.back().size = size;
	}
	return list;
}


