#include <base/material.h>
#include <base/shader.h>
#include <base/autovariables.h>
#include <base/texture.h>
#include <base/opengl.h>
#include <cstdio>

using namespace base;


ShaderVars::ShaderVars() : m_nextIndex(0) {}
ShaderVars::~ShaderVars() {
	for(HashMap<SVar>::iterator i=m_variables.begin(); i!=m_variables.end(); ++i) setType(i->value, VT_AUTO, 0, 0);
}

void ShaderVars::setType(SVar& v, ShaderVarType type, int elements, int array) {
	if(v.elements==0) v.index = m_nextIndex++;
	if(v.type != type || v.elements != elements || v.array != array) {
		// delete old data
		if(v.type!=VT_AUTO && v.elements * v.array > 1) {
			if(v.type == VT_INT || v.type==VT_SAMPLER) delete [] v.ip;
			else delete [] v.fp;
		}

		// Allocate new data
		if(type!=VT_AUTO && elements * array > 1) {
			if(type==VT_INT || type==VT_SAMPLER) v.ip = new int[ elements*array ];
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
void ShaderVars::set(const char* name, const vec4& v) { set(name, 4, 1, v); }

void ShaderVars::setSampler(const char* name, int index) {
	SVar& var = m_variables[name];
	setType(var, VT_SAMPLER, 1, 1);
	var.i = index;
}

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
		setType(m_variables[name], VT_AUTO, 0, 0);
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
		case VT_SAMPLER:
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
template<typename T> const T* ShaderVars::getPointer(const SVar& var) const {
	bool array = var.elements * var.array > 1;
	if(var.type==VT_INT || var.type==VT_SAMPLER) return (const T*)(array? var.ip: &var.i);
	else return (const T*)(array? var.fp: &var.f);
}
template<typename T> const T* ShaderVars::getPointer(const char* name, int typeMask) const {
	auto v = m_variables.find(name);
	if(v!=m_variables.end() && ((1<<v->value.type)&typeMask)) return getPointer<T>(v->value);
	return 0;
}
int* ShaderVars::getIntPointer(const char* name) { return const_cast<int*>(getPointer<int>(name, 1<<VT_INT)); }
int* ShaderVars::getSamplerPointer(const char* name) { return const_cast<int*>(getPointer<int>(name, 1<<VT_SAMPLER)); }
float* ShaderVars::getFloatPointer(const char* name) { return const_cast<float*>(getPointer<float>(name, 1<<VT_FLOAT | 1<<VT_MATRIX)); }

const int* ShaderVars::getIntPointer(const char* name) const{ return getPointer<int>(name, 1<<VT_INT); }
const int* ShaderVars::getSamplerPointer(const char* name) const { return getPointer<int>(name, 1<<VT_SAMPLER); }
const float* ShaderVars::getFloatPointer(const char* name) const{ return getPointer<float>(name, 1<<VT_FLOAT | 1<<VT_MATRIX); }


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
				tmp.type = (ShaderVarType)autos->getData(i->value.i, e, a, fp);
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
			case VT_SAMPLER:
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

size_t Pass::getTextureSlot(const char* name) const {
	if(const int* p = m_ownedVariables.getSamplerPointer(name)) return *p;
	for(size_t i=m_variableData.size()-1; i>0; --i) {
		if(const int* p = m_variableData[i].vars->getSamplerPointer(name)) return *p;
	}
	return m_textures.size();
}

void Pass::setTexture(const char* name, const Texture* tex) {
	size_t slot = m_textures.size();
	// Texture already bound - reuse slot
	for(; slot<m_textures.size(); ++slot) {
		if(m_textures[slot].texture == tex) break; 
	}

	// Reuse existing binding unless bound to multiple variables
	size_t boundSlot = getTextureSlot(name);
	if(boundSlot < m_textures.size() && m_textures[boundSlot].uses < 2) slot = boundSlot;

	setTexture(slot, name, tex);
}
void Pass::setTexture(size_t slot, const char* name, const base::Texture* tex) {
	if(m_textures.size()<slot+1) m_textures.resize(slot+1);
	m_textures[slot].texture = tex;
	if(name) {
		size_t lastSlot = getTextureSlot(name);
		if(lastSlot < m_textures.size()) --m_textures[lastSlot].uses;
		++m_textures[slot].uses;
		m_ownedVariables.setSampler(name, (int)slot);
	}
}
const Texture* Pass::getTexture(const char* name) const {
	for(auto& data: m_variableData) {
		if(const int* ix = data.vars->getSamplerPointer(name)) {
			return m_textures[*ix].texture;
		}
	}
	return 0;
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
		if(m_textures[i].texture) m_textures[i].texture->bind();
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

Blend::Blend() : m_state(State::Disabled) {
}

Blend::Blend(BlendMode mode) {
	if(mode == BLEND_NONE) m_state = State::Disabled;
	else {
		m_state = State::Single;
		m_data.push_back({0});
		setFromEnum(mode, m_data[0].src, m_data[0].dst);
		m_data[0].srcAlpha = m_data[0].src;
		m_data[0].dstAlpha = m_data[0].dst;
	}
}

Blend::Blend(BlendMode colour, BlendMode alpha) {
	if(colour == BLEND_NONE && alpha == BLEND_NONE) m_state = State::Disabled;
	else {
		m_state = colour==alpha? State::Single: State::Separate;
		m_data.push_back({0});
		setFromEnum(colour, m_data[0].src, m_data[0].dst);
		setFromEnum(alpha, m_data[0].srcAlpha, m_data[0].dstAlpha);
	}
}

Blend::Blend(uint srcRGB, uint dstRGB, uint srcAlpha, uint dstAlpha) {
	m_state = srcRGB == srcAlpha && dstRGB == dstAlpha? State::Single: State::Separate;
	m_data.push_back({0, (uint16)srcRGB, (uint16)dstRGB, (uint16)srcAlpha, (uint16)dstAlpha});
}

void Blend::set(int buffer, BlendMode mode) {
	set(buffer, mode, mode);
}

void Blend::set(int buffer, BlendMode colour, BlendMode alpha) {
	m_state = State::IndexedSeparate;
	for(Data& d: m_data) {
		if(d.buffer == buffer) {
			setFromEnum(colour, d.src, d.dst);
			setFromEnum(alpha, d.srcAlpha, d.dstAlpha);
			return;
		}
	}
	m_data.push_back({buffer});
	setFromEnum(colour, m_data.back().src, m_data.back().dst);
	setFromEnum(alpha, m_data.back().srcAlpha, m_data.back().dstAlpha);
}

void Blend::set(int buffer, uint srcRGB, uint dstRGB, uint srcAlpha, uint dstAlpha) {
	m_state = State::IndexedSeparate;
	for(Data& d: m_data) {
		if(d.buffer == buffer) {
			d.src = (uint16)srcRGB;
			d.dst = (uint16)dstRGB;
			d.srcAlpha = (uint16)srcAlpha;
			d.dstAlpha = (uint16)dstAlpha;
			return;
		}
	}
	m_data.push_back({buffer, (uint16)srcRGB, (uint16)dstRGB, (uint16)srcAlpha, (uint16)dstAlpha});
}


bool Blend::operator==(const Blend& b) const {
	if(m_state != b.m_state) return false;
	if(m_data.size() != b.m_data.size()) return false;
	for(size_t i=0; i<m_data.size(); ++i) {
		if(m_data[i].buffer != b.m_data[i].buffer) return false;
		if(m_data[i].src != b.m_data[i].src) return false;
		if(m_data[i].dst != b.m_data[i].dst) return false;
		if(m_data[i].srcAlpha != b.m_data[i].srcAlpha) return false;
		if(m_data[i].dstAlpha != b.m_data[i].dstAlpha) return false;
	}
	return true;
}

void Blend::bind() const {
	switch(m_state) {
	case State::Disabled:
		glDisable(GL_BLEND);
		break;
	case State::Single:
		glEnable(GL_BLEND);
		glBlendFunc(m_data[0].src, m_data[0].dst);
		break;
	case State::Separate:
		glEnable(GL_BLEND);
		glBlendFuncSeparate(m_data[0].src, m_data[0].dst, m_data[0].srcAlpha, m_data[0].dstAlpha);
		break;
	case State::Indexed:
	case State::IndexedSeparate:
		glEnable(GL_BLEND);
		for(const Data& d: m_data) {
			glBlendFuncSeparatei(d.buffer, d.src, d.dst, d.srcAlpha, d.dstAlpha);
		}
		break;
	}
}

void Blend::setFromEnum(BlendMode mode, unsigned short& src, unsigned short& dst) {
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

bool MacroState::operator==(const MacroState& m) const {
	return cullMode==m.cullMode 
		&& depthTest==m.depthTest
		&& depthWrite==m.depthWrite
		&& wireframe==m.wireframe
		&& colourMask==m.colourMask
		&& depthOffset==m.depthOffset;
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

	if(depthOffset != 0) {
		glEnable(wireframe? GL_POLYGON_OFFSET_LINE: GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(depthOffset, 1);
	}
	else {
		glDisable(GL_POLYGON_OFFSET_FILL);
		#ifndef EMSCRIPTEN
		glDisable(GL_POLYGON_OFFSET_LINE);
		#endif
	}

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


bool Material::setTexture(const char* name, Texture* texture) {
	bool ok = false;
	for(Pass* pass: m_passes) {
		if(pass->getTextureSlot(name) < pass->getTextureCount()) {
			pass->setTexture(name, texture);
			ok = true;
		}
	}
	return ok;
}

