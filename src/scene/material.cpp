#include <base/material.h>
#include <base/shader.h>
#include <base/autovariables.h>
#include <base/texture.h>
#include <base/opengl.h>
#include <cstdio>

using namespace base;

#ifdef WIN32
extern PFNGLACTIVETEXTUREARBPROC glActiveTexture;
extern PFNGLBLENDFUNCSEPARATEPROC glBlendFuncSeparate;
#endif

enum VarType {	VT_AUTO, VT_FLOAT, VT_INT, VT_MATRIX };

ShaderVars::ShaderVars() : m_nextIndex(0) {}
ShaderVars::~ShaderVars() {
	for(HashMap<SVar>::iterator i=m_variables.begin(); i!=m_variables.end(); ++i) setType(i->value, 0, 0, 0);
}

void ShaderVars::setType(SVar& v, int type, int elements, int array) {
	if(v.elements==0) v.index = m_nextIndex++;
	if(v.type != type || v.elements != elements || v.array != array) {
		// delete old data
		if(v.type==VT_INT && v.elements*v.array>1) delete [] v.ip;
		else if(v.type==VT_FLOAT && v.elements*v.array>1) delete [] v.fp;
		else if(v.type==VT_MATRIX) delete [] v.fp;

		// Allocate new data
		if(type!=VT_AUTO && elements*array>1) {
			if(type==VT_INT) v.ip = new int[ elements*array ];
			else v.fp = new float[ elements*array ];
		}

		v.type = type;
		v.elements = elements;
		v.array = array;
	}
}

void ShaderVars::set(const char* name, int i) { set(name, 1, 1, &i); }
void ShaderVars::set(const char* name, float v) { set(name, 1, 1, &v); }
void ShaderVars::set(const char* name, float x, float y) { float v[]={x,y}; set(name, 2, 1, v); }
void ShaderVars::set(const char* name, float x, float y, float z) { float v[]={x,y,z}; set(name, 3, 1, v); }
void ShaderVars::set(const char* name, float x, float y, float z, float w) { float v[]={x,y,z,w}; set(name, 4, 1, v); }
void ShaderVars::set(const char* name, const vec2& v) { set(name, 2, 1, v); }
void ShaderVars::set(const char* name, const vec3& v) { set(name, 3, 1, v); }

void ShaderVars::set(const char* name, int c, int a, const int* v) {
	SVar& var = m_variables[name];
	setType(var, VT_INT, c, a);
	if(c*a==1) var.i = *v;
	else memcpy(var.ip, v, a*c*sizeof(int));
}

void ShaderVars::set(const char* name, int c, int a, const float* v) {
	SVar& var = m_variables[name];
	setType(var, VT_FLOAT, c, a);
	if(c*a==1) var.f = *v;
	else memcpy(var.fp, v, a*c*sizeof(float));
}

void ShaderVars::setMatrix(const char* name, float* v) {
	SVar& var = m_variables[name];
	setType(var, VT_MATRIX, 16, 1);
	memcpy(var.fp, v, 16*sizeof(float));
}

void ShaderVars::unset(const char* name) {
	if(m_variables.contains(name)) {
		setType(m_variables[name], 0, 0, 0);
		m_variables.erase(name);
	}
}

void ShaderVars::setAuto(const char* name, int key) {
	SVar& var = m_variables[name];
	setType(var, VT_AUTO, -1, 0);	// Use elements=-1 to flag as initialised
	var.i = key;
}

// --------------------------------- //

int ShaderVars::setFrom(const ShaderVars* s) {
	for(HashMap<SVar>::const_iterator i=s->m_variables.begin(); i!=s->m_variables.end(); ++i) {
		bool single = i->value.array * i->value.elements == 1;
		switch(i->value.type) {
		case VT_AUTO:   setAuto(i->key, i->value.i); break;
		case VT_INT:    set(i->key, i->value.elements, i->value.array, single? &i->value.i: i->value.ip); break;
		case VT_FLOAT:  set(i->key, i->value.elements, i->value.array, single? &i->value.f: i->value.fp); break;
		case VT_MATRIX: set(i->key, i->value.elements, i->value.array, single? &i->value.f: i->value.fp); break;
		}
	}
	return s->m_variables.size();
}

// --------------------------------- //

bool ShaderVars::contains(const char* name) const {
	return m_variables.contains(name);
}
int  ShaderVars::getElements(const char* name) const {
	HashMap<SVar>::const_iterator i=m_variables.find(name);
	if(i==m_variables.end()) return 0;	// does not exist
	if(i->value.type == VT_AUTO) return 0;	// unknown
	return i->value.elements * i->value.array;
}
int* ShaderVars::getIntPointer(const char* name) {
	if(!contains(name)) return 0;
	SVar& v = m_variables[name];
	if(v.type != VT_INT) return 0; // wrong type
	if(v.elements * v.array == 1) return &v.i;
	else return v.ip;
}
float* ShaderVars::getFloatPointer(const char* name) {
	if(!contains(name)) return 0;
	SVar& v = m_variables[name];
	if(v.type != VT_FLOAT && v.type != VT_MATRIX) return 0; // wrong type
	if(v.elements * v.array == 1) return &v.f;
	else return v.fp;
}

const int* ShaderVars::getIntPointer(const char* name) const {
	if(!contains(name)) return 0;
	const SVar& v = m_variables[name];
	if(v.type != VT_INT) return 0; // wrong type
	if(v.elements * v.array == 1) return &v.i;
	else return v.ip;
}
const float* ShaderVars::getFloatPointer(const char* name) const {
	if(!contains(name)) return 0;
	const SVar& v = m_variables[name];
	if(v.type != VT_FLOAT && v.type != VT_MATRIX) return 0; // wrong type
	if(v.elements * v.array == 1) return &v.f;
	else return v.fp;
}


// ============================================================================== //

int ShaderVars::getIndexCount() const {
	return m_nextIndex;
}

int ShaderVars::getLocationData(const Shader* s, int* map, int size, bool complain) const {
	int count = 0;
	s->bind();
	if(!s->isCompiled()) return 0;
	for(HashMap<SVar>::const_iterator i=m_variables.begin(); i!=m_variables.end(); ++i) {
		if(i->value.index>=size) { printf("Material needs recompiling\n");  continue; }
		map[ i->value.index ] = s->getUniformLocation( i->key );
		if(map[i->value.index]>=0) ++count;
		else if(complain) 
			printf("Missing shader variable %s\n", i->key);
	}
	return count;
}
int ShaderVars::bindVariables(const Shader* s, int* map, int size, AutoVariableSource* autos, bool autosOnly) const {
	if(!s || !map) return 0; // Not compiled
	int count = 0;
	SVar tmp;
	GL_CHECK_ERROR;
	for(HashMap<SVar>::const_iterator i=m_variables.begin(); i!=m_variables.end(); ++i) {
		if(i->value.index>=size)  {
			//printf("Material needs recompiling\n");
			continue;
		}
		if(map[i->value.index]>=0) {
			++count;
			int loc = map[i->value.index];
			const SVar* var = &(i->value);
			const float* fp = var->array==1 && var->elements==1? &var->f: var->fp;
			if(autos && var->type==VT_AUTO) {
				int e, a;
				tmp.type = autos->getData(i->value.i, e, a, fp);
				tmp.elements = e;
				tmp.array = a;
				tmp.fp = 0;
				var = &tmp;
			}
			else if(autosOnly) continue;

			switch(var->type) {
			case VT_AUTO: break;
			case VT_FLOAT:
				if(var->elements==1) s->setUniform1(loc, var->array, fp);
				else if(var->elements==2) s->setUniform2(loc, var->array, fp);
				else if(var->elements==3) s->setUniform3(loc, var->array, fp);
				else if(var->elements==4) s->setUniform4(loc, var->array, fp);
				else --count;
				break;
			case VT_INT:
				if(var->elements==1) s->setUniform1(loc, var->array, var->array>1? var->ip: &var->i);
				else if(var->elements==2) s->setUniform2(loc, var->array, var->ip);
				else if(var->elements==3) s->setUniform2(loc, var->array, var->ip);
				else if(var->elements==4) s->setUniform2(loc, var->array, var->ip);
				else --count;
				break;
			case VT_MATRIX:
				if(var->elements==4)       s->setMatrix2(loc,   var->array, fp);
				else if(var->elements==9)  s->setMatrix3(loc,   var->array, fp);
				else if(var->elements==16) s->setMatrix4(loc,   var->array, fp);
				else if(var->elements==12) s->setMatrix3x4(loc, var->array, fp);
				else --count;
				break;
			}
		}
	}
	GL_CHECK_ERROR;	// Invalid operation (502) can occur if type is wrong
	return count;
}



// ============================================================================== //



Pass::Pass(Material* m) : m_material(m), m_shader(0), m_changed(0), m_id(0) {
	addShared(&m_ownedVariables);
}
Pass::~Pass() {
	for(size_t i=0; i<m_variableData.size(); ++i)
		delete [] m_variableData[i].map;
}

Pass* Pass::clone(Material* m) const {
	Pass* p = new Pass(m);
	p->m_shader = m_shader;
	p->blend = blend;
	p->state = state;
	p->m_id = m_id;
	// copy texture list
	for(size_t i=0; i<m_textures.size(); ++i) p->m_textures.push_back( m_textures[i] );
	// copy shared list
	for(size_t i=1; i<m_variableData.size(); ++i) p->addShared(m_variableData[i].vars);
	// set variables
	p->m_ownedVariables.setFrom( &m_ownedVariables );
	return p;
}

bool Pass::hasShader() const {
	return m_shader && m_shader->isCompiled();
}
void Pass::setShader(Shader* s) {
	m_shader = s;
	m_changed = true;
}

void Pass::addShared(const ShaderVars* s) {
	if(hasShared(s)) return;
	VariableData inst;
	inst.vars = s;
	inst.size = 0;
	inst.map = 0;
	m_variableData.push_back(inst);
	m_changed = true;
}
void Pass::removeShared(const ShaderVars* s) {
	for(size_t i=0; i<m_variableData.size(); ++i) if(m_variableData[i].vars == s) {
		delete [] m_variableData[i].map;
		m_variableData.erase(m_variableData.begin()+i);
		break;
	}
}
bool Pass::hasShared(const ShaderVars* s) const {
	for(const VariableData& v: m_variableData) if(v.vars == s) return true;
	return false;
}

void Pass::setTexture(const char* name, const Texture* tex) {
	// Does this texture already have a slot set
	for(size_t i=0; i<m_variableData.size(); ++i) {
		const int* p = m_variableData[i].vars->getIntPointer(name);
		if(p) {
			setTexture(*p, 0, tex);
			return;
		}
	}
	size_t index = m_textures.size();
	for(size_t i=0; i<m_textures.size(); ++i) {
		if(m_textures[i] == tex) { index=i; break; }
		else if(!m_textures[i]) index = i;
	}
	setTexture(index, name, tex);
}
void Pass::setTexture(size_t slot, const char* name, const base::Texture* tex) {
	if(m_textures.size()<slot+1) m_textures.resize(slot+1, 0);
	m_textures[slot] = tex;
	if(name) m_ownedVariables.set(name, (int)slot);
}
const Texture* Pass::getTexture(const char* name) const {
	const int* ix = m_ownedVariables.getIntPointer(name);
	return ix? m_textures[*ix]: 0;
}

// -------------------------------- //

base::HashMap<size_t> Pass::s_names;
std::vector<const char*> Pass::s_namesList;

const char* Pass::getName() const {
	return m_id? s_namesList[m_id-1]: "";
}
void Pass::setName(const char* name) {
	m_id = Material::getPassID(name);
}

// ----------------------------------------------------------------------------- //

CompileResult Pass::compile() {
	m_changed = false;
	if(!m_shader) return COMPILE_OK;
	if(m_shader->needsRecompile()) m_shader->compile();
	if(!m_shader->isCompiled()) return COMPILE_FAILED;

	// update all variable locations
	CompileResult result = COMPILE_OK;
	for(size_t i=0; i<m_variableData.size(); ++i) {
		VariableData& data = m_variableData[i];
		int newSize = data.vars->getIndexCount();
		if(!data.map || newSize > data.size) {
			delete [] data.map;
			if(newSize > 0) data.map = new int[ newSize ];
			else data.map = 0;
			data.size = newSize;
		}
		memset(data.map, 0xff, newSize*sizeof(int));	// set all to -1
		int r = m_variableData[i].vars->getLocationData(m_shader, m_variableData[i].map, m_variableData[i].size, i==0);
		if(i==0 && r < m_variableData[i].size) result = COMPILE_WARN;
	}
	return result;
}

void Pass::bind(AutoVariableSource* autos) {
	if(m_shader) m_shader->bind();
	else Shader::Null.bind();

	// bind variables
	bindVariables(autos);
	bindTextures();
	blend.bind();
	state.bind();
}

void Pass::bindTextures() const {
	for(size_t i=0; i<m_textures.size(); ++i) {
		glActiveTexture(GL_TEXTURE0 + i);
		if(m_textures[i]) m_textures[i]->bind();
	}
	if(m_textures.size()>1) glActiveTexture(GL_TEXTURE0);
}

void Pass::bindVariables(AutoVariableSource* autos, bool onlyAuto) const {
	if(m_shader != &Shader::current()) return;
	for(size_t i=0; i<m_variableData.size(); ++i) {
		m_variableData[i].vars->bindVariables(m_shader, m_variableData[i].map, m_variableData[i].size, autos, onlyAuto);
	}
}



// ======================================================================================= //

Blend::Blend() : enabled(false), separate(false), src(0), dst(0), srcAlpha(0), dstAlpha(0) {
}
Blend::Blend(BlendMode mode) : enabled(true), separate(false), src(0), dst(0), srcAlpha(0), dstAlpha(0) {
	if(mode == BLEND_NONE) enabled = false;
	else setFromEnum(mode, src, dst);
}
Blend::Blend(BlendMode colour, BlendMode alpha) : enabled(true), separate(false), src(0), dst(0), srcAlpha(0), dstAlpha(0) {
	if(colour == BLEND_NONE && alpha == BLEND_NONE) enabled = false;
	else {
		setFromEnum(colour, src, dst);
		setFromEnum(alpha, srcAlpha, dstAlpha);
	}
}
bool Blend::operator==(const Blend& b) const {
	if(!enabled && !b.enabled) return true;
	if(enabled != b.enabled) return false;
	if(separate != b.separate) return false;
	if(src!=b.src || dst!=b.dst) return false;
	if(separate && (srcAlpha!=b.srcAlpha || dstAlpha!=b.dstAlpha)) return false;
	return true;
}

void Blend::bind() const {
	if(enabled) {
		glEnable(GL_BLEND);
		if(separate) glBlendFuncSeparate(src, dst, srcAlpha, dstAlpha);
		else glBlendFunc(src, dst);
	}
	else glDisable(GL_BLEND);
}

void Blend::setFromEnum(BlendMode mode, int& rsc, int& dst) {
	switch(mode) {
	case BLEND_NONE:
		src = GL_ONE;
		dst = GL_ZERO;
	case BLEND_ALPHA:
		src = GL_SRC_ALPHA;
		dst = GL_ONE_MINUS_SRC_ALPHA;
		break;
	case BLEND_ADD:
		src = dst = GL_ONE;
		break;
	case BLEND_MULTIPLY:
		src = GL_DST_COLOR;
		dst = GL_ZERO;
	}
}



// ======================================================================================= //

MacroState::MacroState() : cullMode(CULL_BACK), depthTest(DEPTH_LEQUAL), depthWrite(true), colourMask(MASK_ALL), wireframe(false) {
}

bool MacroState::operator==(const MacroState& m) const {
	return cullMode==m.cullMode && depthTest==m.depthTest && depthWrite==m.depthWrite && wireframe==m.wireframe && colourMask==m.colourMask;
}
void MacroState::bind() const {
	switch(cullMode) {
	case CULL_NONE: glDisable(GL_CULL_FACE); break;
	case CULL_BACK: glEnable(GL_CULL_FACE); glCullFace(GL_BACK); break;
	case CULL_FRONT: glEnable(GL_CULL_FACE); glCullFace(GL_FRONT); break;
	}

	switch(depthTest) {
	case DEPTH_ALWAYS:  glEnable(GL_DEPTH_TEST); glDepthFunc(GL_ALWAYS); break;
	case DEPTH_LESS:    glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LESS); break;
	case DEPTH_LEQUAL:  glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL); break;
	case DEPTH_GREATER: glEnable(GL_DEPTH_TEST); glDepthFunc(GL_GREATER); break;
	case DEPTH_GEQUAL:  glEnable(GL_DEPTH_TEST); glDepthFunc(GL_GEQUAL); break;
	case DEPTH_EQUAL:   glEnable(GL_DEPTH_TEST); glDepthFunc(GL_EQUAL); break;
	case DEPTH_DISABLED: glDisable(GL_DEPTH_TEST); break;
	}

	glDepthMask(depthWrite);
	glColorMask(colourMask&1, colourMask&2, colourMask&4, colourMask&8);

	#ifndef EMSCRIPTEN
	glPolygonMode(GL_FRONT_AND_BACK, wireframe? GL_LINE: GL_FILL);
	#endif
}

// ======================================================================================= //


Material::Material() {
}
Material::~Material() {
	for(int i=0; i<size(); ++i) delete m_passes[i];
}
Material* Material::clone() const {
	Material* m = new Material();
	for(int i=0; i<size(); ++i) m->addPass( m_passes[i]->clone(m) );
	return m;
}

int Material::size() const {
	return m_passes.size();
}

Pass* Material::getPass(int index) const {
	if(index<size()) return m_passes[index];
	else return 0;
}
Pass* Material::getPass(const char* name) const {
	return getPassByID( getPassID(name) );
}
Pass* Material::getPassByID(size_t id) const {
	for(int i=0; i<size(); ++i) {
		if(m_passes[i]->m_id == id) return m_passes[i];
	}
	return 0;
}
Pass* Material::addPass(const char* name) {
	m_passes.push_back( new Pass(this) );
	if(name) m_passes.back()->setName(name);
	return m_passes.back();
}
Pass* Material::addPass(const Pass* pass, const char* name) {
	m_passes.push_back(pass->clone(this));
	if(name) m_passes.back()->setName(name);
	return m_passes.back();
}
void Material::deletePass(const Pass* pass) {
	for(int i=0; i<size(); ++i) {
		if(m_passes[i] == pass) {
			delete pass;
			m_passes.erase(m_passes.begin()+i);
			return;
		}
	}
}

size_t Material::getPassID(const char* name) {
	if(!name || !name[0]) return 0;
	else {
		size_t id = Pass::s_names.get(name, Pass::s_namesList.size()+1);
		if(id > Pass::s_names.size()) {
			Pass::s_names[name] = id;
			Pass::s_namesList.push_back( Pass::s_names.find(name)->key );
		}
		return id;
	}
}
