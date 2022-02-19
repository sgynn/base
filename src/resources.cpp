#include <base/model.h>
#include <base/shader.h>
#include <base/material.h>
#include <base/resources.h>
#include <base/autovariables.h>
#include <base/hardwarebuffer.h>
#include <base/compositor.h>
#include <base/bmloader.h>
#include <base/texture.h>
#include <base/png.h>
#include <base/dds.h>
#include <base/xml.h>
#include <cstdio>

using namespace base;

Resources* Resources::s_instance = 0;

//// ResourceManagerBase functions ////

ResourceManagerBase::~ResourceManagerBase() {
	for(size_t i=0; i<m_paths.size(); ++i) free(m_paths[i]);
}
void ResourceManagerBase::addPath(const char* path) {
	if(path && path[0]) m_paths.push_back( strdup(path) );
}

bool ResourceManagerBase::findFile(const char* name, char* out, size_t s) const {
	for(size_t i=0; i<m_paths.size(); ++i) {
		snprintf(out, s, "%s/%s", m_paths[i], name);
		out[s-1] = 0;
		FILE* fp = fopen(out, "rb");
		if(fp) {
			fclose(fp);
			return true;
		}
	}
	return false;
}

void ResourceManagerBase::error(const char* msg, const char* name) const {
	printf("%s %s\n", msg, name);
}

// ----------------------------------------------------------------------------------- //

// Helper for loading a raw file
class ResourceFile {
	char* m_data;
	public:
	ResourceFile(const char* name, ResourceManagerBase* manager) : m_data(0) {
		char filename[1024];
		if(manager->findFile(name, filename, 1024)) {
			FILE* fp = fopen(filename, "r");
			if(fp) {
				fseek(fp, 0, SEEK_END);
				size_t length = ftell(fp);
				rewind(fp);
				m_data = new char[length+1];
				fread(m_data, 1, length, fp);
				m_data[length] = 0;
				fclose(fp);
			}
		}
	}
	~ResourceFile() { delete [] m_data; }
	operator const char*() const { return m_data; }
	operator bool() const { return m_data; }
};

// ----------------------------------------------------------------------------------- //


// Interface for loading resourced defines in xml files.
class XMLResourceLoader {
	Resources& resources;
	public:
	XMLResourceLoader(Resources* r) : resources(*r) {}
	Shader*          loadShader(const XMLElement&);
	void             loadMaterialPass(const XMLElement&, Pass* pass);
	Material*        loadMaterial(const XMLElement&, const char* name="");
	ShaderVars*      loadShaderVars(const XMLElement&);
	Compositor*      loadCompositor(const XMLElement&);
	CompositorGraph* loadGraph(const XMLElement&);
	void load(const XML&, const char* path=0);
};

// ----------------------------------------------------------------------------------- //


class TextureLoader : public ResourceLoader<Texture> {
	public:
	Texture* create(const char*, Manager*) override;
	bool reload(const char* name, Texture* object, Manager*) override;
	void destroy(Texture*) override;
};

Texture* TextureLoader::create(const char* name, Manager* manager) {
	// Static colour
	if(name[0]=='#') {
		char* end = 0;
		uint hex = strtoul(name+1, &end, 16);
		int len = end-name-1;
		if(len>0 && *end==0) {
			hex = (hex&0xff00ff00) | ((hex>>16)&0xff) | (hex&0xff)<<16; // ABGR -> ARGB
			Texture tex = Texture::create(Texture::TEX2D, 1, 1, 1, len>6? Texture::RGBA8: Texture::RGB8, &hex);
			return new Texture(tex);
		}
	}

	// Resolve filename
	char filename[1024];
	if(!manager->findFile(name, filename, 1024)) return 0;

	// Check extension
	const char* ext = strrchr(filename, '.');
	if(strcmp(ext, ".dds")==0) {
		DDS dds = DDS::load(filename);
		if(dds.format) {
			static const Texture::Format formats[] = { Texture::NONE, Texture::R8, Texture::RG8, Texture::RGB8, Texture::RGBA8, Texture::BC1, Texture::BC2, Texture::BC3, Texture::BC4, Texture::BC5 };
			static const Texture::Type types[] = { Texture::TEX2D, Texture::CUBE, Texture::TEX3D, Texture::ARRAY2D };
			Texture tex = Texture::create(types[dds.mode], dds.width, dds.height, 1, formats[dds.format], (void**)dds.data, dds.mipmaps+1);
			if(dds.mipmaps) tex.setFilter( Texture::TRILINEAR );
			return new Texture(tex);
		} else {
			printf("Resource Error: Invalid png file '%s'\n", filename);
		}
	}
	else if(strcmp(ext, ".png")==0) {
		PNG png = PNG::load(filename);
		if(png.data && png.bpp<=32) {
			Texture::Format format = (Texture::Format) (png.bpp / 8);
			Texture tex = Texture::create(png.width, png.height, format, png.data, true);
			return new Texture(tex);
		}
		else {
			printf("Resource Error: Invalid png file '%s'\n", filename);
		}
	}
	else {
		printf("Resource Error: Invalid image format '%s'\n", filename);
	}
	return 0;
}
bool TextureLoader::reload(const char* name, Texture* object, Manager* manager) {
	Texture* tex = create(name, manager);
	if(tex) {
		object->destroy();
		*object = *tex;
		delete tex;
		return true;
	}
	return false;
}
void TextureLoader::destroy(Texture* tex) {
	tex->destroy();
	delete tex;
}


// ----------------------------------------------------------------------------------- //

class ShaderVarsLoader : public ResourceLoader<ShaderVars> {
	public:
	ShaderVars* create(const char*, Manager*) override { return new ShaderVars(); }
	void destroy(ShaderVars* s) override { delete s; }
};


// ----------------------------------------------------------------------------------- //
class ShaderPartLoader : public ResourceLoader<ShaderPart> {
	public:
	ShaderPartLoader(ResourceManager<Shader>& usePathsFrom): m_shaders(usePathsFrom) {}
	ShaderPart* create(const char*, Manager*) override;
	bool reload(const char* name, ShaderPart*, Manager*) override;
	void destroy(ShaderPart* s) override { delete s; }
	protected:
	ResourceManager<Shader>& m_shaders;
};

ShaderPart* ShaderPartLoader::create(const char* name, Manager* manager) {
	// Format: 'file|DEFINES' use frag/geom/vert extensions or prefix with FS:/VS:/GS:
	ShaderType type;
	if(name[1]=='S' && name[2] == ':') switch(name[0]) {
	case 'F': type = FRAGMENT_SHADER; name+=2; break;
	case 'V': type = VERTEX_SHADER;   name+=2; break;
	case 'G': type = GEOMETRY_SHADER; name+=2; break;
		printf("Error: Invalid type specifier on shader part '%s'\n", name);
		return 0;
	}
	else if(strstr(name, ".vert")) type = VERTEX_SHADER;
	else if(strstr(name, ".frag")) type = FRAGMENT_SHADER;
	else if(strstr(name, ".geom")) type = GEOMETRY_SHADER;
	else {
		printf("Error: Could not determine type of shader part '%s'\n", name);
		return 0;
	}


	// Separate defines
	char temp[256];
	const char* sep = strchr(name, '|');
	const char* defines = sep? sep+1: 0;
	if(defines) {
		strncpy(temp, name, sep-name), temp[sep-name] = 0;
		name = temp;
	}

	// See if shader exists with no defines
	if(defines && manager->exists(name)) {
		ShaderPart* s = manager->get(name);
		ShaderPart* part = new ShaderPart(*s);
		part->setDefines(defines);
		return part;
	}

	ResourceFile file(name, &m_shaders);
	if(!file) return 0;

	// Create shader
	ShaderPart* part = new ShaderPart(type, file, defines);
	return part;
}

bool ShaderPartLoader::reload(const char* name, ShaderPart* data, Manager* manager) {
	ShaderPart* part = create(name, manager);
	if(part) *data = *part;
	delete part;
	return part;
}

// ----------------------------------------------------------------------------------- //
//
//
// Design: default shader loader will be like the old system: file.glsl:DEFINES
// New system will be named from material, and store the material filename
// Shader parts will be Vfile.glsl:DEFINES.
//

class ShaderLoader : public ResourceLoader<Shader> {
	ResourceManager<ShaderPart>& m_partManager;
	public:
	ShaderLoader(ResourceManager<ShaderPart>& parts) : m_partManager(parts) {}
	Shader* create(const char*, Manager*) override;
	bool reload(const char*, Shader*, Manager*) override;
	void destroy(Shader* s) override { delete s; }
};


Shader* ShaderLoader::create(const char* name, Manager* mgr) {
	// This one is a complete mess
	// the name is all the parts concatenated
	// Shader with no parts is valid - assembled elsewhere.
	
	Shader* shader = new Shader();
	int attachments = 0;

	if(name) {
		if(strchr(name, ',')==0) {
			// Name being a single glsl file
			char vsKey[256], fsKey[256];
			sprintf(vsKey, "VS:%s", name);
			sprintf(fsKey, "FS:%s", name);
			ShaderPart* vs = m_partManager.getIfExists(vsKey);
			ShaderPart* fs = m_partManager.getIfExists(fsKey);

			if(!vs && !fs) {
				ResourceFile file(name, mgr);
				if(file) {
					vs = new ShaderPart(VERTEX_SHADER, file);
					fs = new ShaderPart(FRAGMENT_SHADER, file);
					m_partManager.add(fsKey, fs);
					m_partManager.add(vsKey, vs);
					attachments += 2;
				}
			}

			if(vs && fs) {
				shader->attach(vs);
				shader->attach(fs);
			}
			else {
				printf("Failed to create shader %s\n", name);
				delete shader;
				return 0;
			}
		}
		else {
			// Name being a list of parts
			char* buffer = strdup(name);
			char* partName = strtok(buffer, ",");
			while(partName) {
				ShaderPart* part = m_partManager.get(partName);
				if(part) shader->attach(part);
				if(part) ++attachments;
				partName = strtok(0, ",");
			}
			free(buffer);
		}
	}

	shader->bindAttributeLocation( "vertex",   0 );
	shader->bindAttributeLocation( "normal",   1 );
	shader->bindAttributeLocation( "tangent",  2 );
	shader->bindAttributeLocation( "texCoord", 3 );
	shader->bindAttributeLocation( "colour",   4 );
	shader->bindAttributeLocation( "indices",  5 );
	shader->bindAttributeLocation( "weights",  6 );
	shader->bindOutput("buf0", 0);
	shader->bindOutput("buf1", 1);

	return shader;
}


bool ShaderLoader::reload(const char* name, Shader* data, Manager* manager) {
	// Reload parts
	bool partReloaded = false;
	if(strchr(name, ',')==0) {
		char partName[256];
		sprintf(partName, "VS:%s", name);
		partReloaded |= m_partManager.reload(partName);
		sprintf(partName, "FS:%s", name);
		partReloaded |= m_partManager.reload(partName);
	}
	else {
		char* buffer = strdup(name);
		char* partName = strtok(buffer, ",");
		while(partName) {
			partReloaded |= m_partManager.reload(partName);
			partName = strtok(0, ",");
		}
		free(buffer);
	}
	if(!partReloaded) return false; // No point rebuilding shader if nothing changed

	Shader* shader = create(name, manager);
	*data = *shader;
	data->compile();
	return true;
}


// ----------------------------------------------------------------------------------- //

class MaterialLoader : public ResourceLoader<Material> {
	Resources* resources;
	public:
	MaterialLoader(Resources* res) : resources(res) {}
	void destroy(Material* m) override { delete m; }
	Material* create(const char*, Manager*) override;
};

static void parseShaderVariable(ShaderVars& vars, const XMLElement& e);
static Blend parseBlendState(const XMLElement&);

Material* MaterialLoader::create(const char* name, Manager* manager) {
	// Materials are complicated
	
	// Resolve filename
	char filename[1024];
	if(!manager->findFile(name, filename, 1024)) {
		printf("MaterialLoader: File not found %s\n", name);
		return 0;
	}
	XML xml = XML::load(filename);
	XMLResourceLoader loader(resources);

	// load materials
	if(xml.getRoot() == "material") {
		return loader.loadMaterial(xml.getRoot(), name);
	}
	else {
		// Multiple materials in file
		loader.load(xml);

		// return first material
		for(const XMLElement& e: xml.getRoot()) {
			if(e == "material") return resources->materials.getIfExists(e.attribute("name"));
		}
		return 0;
	}
}

// ----------------------------------------------------------------------------------- //

class ModelLoader : public ResourceLoader<Model> {
	Resources* resources;
	public:
	ModelLoader(Resources* res) : resources(res) {}
	Model* create(const char*, Manager*) override;
	void destroy(Model* m) override { delete m; }
};
Model* ModelLoader::create(const char* name, Manager* manager) {
	// Resolve filename
	char filename[1024];
	if(!manager->findFile(name, filename, 1024)) {
		printf("ModelLoader: File not found %s\n", name);
		return 0;
	}
	XML xml = XML::load(filename);
	XMLResourceLoader loader(resources);

	// Materials defined here need to be loaded into the material manager somehow.
	for(const XMLElement& e: xml.getRoot()) {
		if(e=="material") {
			char matName[128];
			snprintf(matName, 128, "%s:%s", name, e.attribute("name"));
			Material* m = loader.loadMaterial(e, matName);
			resources->materials.add(matName, m); // ToDo: Custom loader here too, or additional file data
		}
	}
	
	// Load model
	Model* m = BMLoader::loadModel(xml.getRoot());

	if(m) printf("Loaded %s\n", filename);
	else printf("Failed to load %s\n", filename);

	// Create vbo's
	if(m) {
		for(int i=0; i<m->getMeshCount(); ++i) {
			Mesh* mesh = m->getMesh(i);
			if(mesh->getVertexBuffer()) mesh->getVertexBuffer()->createBuffer();
			if(mesh->getSkinBuffer())   mesh->getSkinBuffer()->createBuffer();
			if(mesh->getIndexBuffer())  mesh->getIndexBuffer()->createBuffer();
			mesh->calculateBounds();
		}
	}
	
	return m;
}


// =================================================================================== //


static bool compileShader(Shader* shader, const char* name) {
	if(!shader->compile()) {
		char buffer[2048];
		shader->getLog(buffer, sizeof(buffer));
		printf("Errors compiling %s:\n", name);
		printf("%s", buffer);
	}
	return shader->isCompiled();
}

static bool compilePass(Pass* pass, const char* name, const char* scheme=0) {
	switch(pass->compile()) {
	case COMPILE_OK: return true;
	case COMPILE_FAILED:
		printf("Failed to compile material %s\n", name);
		return false;
	case COMPILE_WARN:
		printf("Warnings compiling material %s\n", name);
		return true;
	}
	return false;
}


// ----------------------------------------------------------------------------------- //



static int floatArray(const char* s, float* array, int max) {
	int count;
	char* end;
	for(count=0; count<max; ++count) {
		while(*s==' ' || *s=='\t' || *s=='\n' || *s == '\r') ++s; //whitespace
		if(*s==0) break; // End of string
		array[count] = strtof(s, &end);
		if(s == end) break;
		s = end;
	}
	return count;
}

static int enumValue(const char* value, int size, const char** strings) {
	for(int i=0; i<size; ++i) if(strcmp(value, strings[i])==0) return i;
	return -1;
}
template<typename T> T enumValueT(const char* value, int size, const char** strings, T defaultValue) {
	int r = enumValue(value, size, strings);
	if(r<0) return defaultValue;
	else return (T)r;
}

static void parseShaderVariable(ShaderVars& vars, const XMLElement& e) {
	const char* name = e.attribute("name");
	const char* type = e.attribute("type");

	if(strcmp(type, "int")==0) {
		vars.set(name, e.attribute("value", 0));
	}
	else if(strcmp(type, "float")==0) {
		vars.set(name, e.attribute("value", 0.f));
	}
	else if(strncmp(type, "vec", 3)==0 && type[4]==0) {
		int size = type[3] - '0';
		if(size < 2 || size > 4) printf("Material Error: Invalid variable type '%s'\n", type);
		else {
			float v[4] = { 0, 0, 0, 0 };
			if(!floatArray( e.attribute("value", ""), v, size)) {
				v[0] = e.attribute("x", v[0]);
				v[1] = e.attribute("y", v[1]);
				v[2] = e.attribute("z", v[2]);
				v[3] = e.attribute("w", v[3]);

				v[0] = e.attribute("r", v[0]);
				v[1] = e.attribute("g", v[1]);
				v[2] = e.attribute("b", v[2]);
				v[3] = e.attribute("a", v[3]);
			}
			vars.set(name, size, 1, v);
		}
	}
	else if(strcmp(type, "mat4") == 0) {
		float v[16] = { 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 };
		floatArray( e.attribute("value"), v, 16 );
		vars.setMatrix(name, v);
	}
	else if(strcmp(type, "auto")==0) {
		bool found = false;
		const char* keyString = e.attribute("value");
		for(int key=0; AutoVariableSource::getKeyString(key); ++key) {
			if(strcmp(AutoVariableSource::getKeyString(key), keyString)==0) {
				vars.setAuto(name, key);
				found = true;
				break;
			}
		}
		if(!found) printf("Material Error: No such auto variable '%s'\n", keyString);
	}
	else printf("Error: Invalid shader variable type %s for %s\n", type, name);
}

Shader* XMLResourceLoader::loadShader(const XMLElement& e) {
	const char* name = e.attribute("name");
	Shader* s = resources.shaders.getIfExists(name);
	if(s) return s;

	// Construct shader
	s = resources.shaders.getDefaultLoader()->create(0, &resources.shaders);

	// Parts
	const char* filename = e.attribute("file"); // vs+fs
	const char* defines = e.attribute("defines");
	if(filename[0]) {
		if(!name[0]) name = filename;
		ResourceFile file(filename, &resources.shaders);
		if(file) {
			ShaderPart* vs = new ShaderPart(VERTEX_SHADER, file, defines);
			ShaderPart* fs = new ShaderPart(FRAGMENT_SHADER, file, defines);
			s->attach(vs);
			s->attach(fs);
			// TODO: Add to shader part resoure manager
		}
	}
	// additional source files
	for(const XMLElement& i: e) {
		if(i=="source") {
			filename = i.attribute("file");
			const char* shaderTypes[] = { "vertex", "fragment", "geometry" };
			int type = enumValue(i.attribute("type"), 3, shaderTypes);
			ResourceFile file(filename, &resources.shaders);
			if(type<0) printf("Invalid shader attachment type %s\n", i.attribute("type"));
			else if(!file) printf("Error: Shader file not found: %s\n", filename);
			else {
				ShaderPart* ss = new ShaderPart((ShaderType)type, file, i.attribute("defines"));
				s->attach(ss);
			}
		}
	}
	compileShader(s, name);
	return s;
};

void XMLResourceLoader::loadMaterialPass(const XMLElement& e, Pass* pass) {
	const char* nullString = 0;
	for(const XMLElement& i: e) {
		if(i == "texture") {
			const char* name = i.attribute("name");
			const char* buffer = i.attribute("buffer", nullString);
			if(buffer) {
				int mrt = i.attribute("index", 0);
				if(mrt==0 && i.attribute("index")[0]=='d') mrt = -1;
				pass->setTexture(name, CompositorTextures::getInstance()->get(buffer, mrt));
			}
			else {
				Texture* tex = resources.textures.get(i.attribute("file"));
				if(tex) pass->setTexture( i.attribute("name"), tex );
				else printf("Error: Texture '%s' not found\n", i.attribute("file"));
			}
		}
		else if(i == "shader") {
			// Need shader manager
			Shader* s = loadShader(i);
			if(s) pass->setShader(s);
			else printf("Material Error: Shader '%s' not found\n", i.attribute("name", i.attribute("file")));
		}
		else if(i == "variable") {
			// Owned shader variable
			parseShaderVariable(pass->getParameters(), i);
		}
		else if(i=="shared") {
			// Shared variable object
			const char* name = i.attribute("name");
			ShaderVars* shared = resources.shaderVars.get(name);
			if(shared) pass->addShared(shared);
			else printf("Error: Missing shared shadervars %s\n", name);
		}
		else if(i=="blend") {
			pass->blend = parseBlendState(i);
		}
		else if(i=="state") {
			const char* cullModes[] = { "none", "back", "front" };
			pass->state.cullMode = enumValueT(i.attribute("cull"), 3, cullModes, CULL_BACK);
			const char* depthModes[] = { "ignore", "less", "lequal", "greater", "gequal", "equal" };
			pass->state.depthTest = enumValueT(i.attribute("depth"), 6, depthModes, DEPTH_LEQUAL);
			pass->state.depthWrite = i.attribute("depthWrite", 1);	// perhaps use mask="RGBD"
			pass->state.wireframe  = i.attribute("wireframe", 0);
		}
	}
}

static Blend parseBlendState(const XMLElement& e) {
	Blend blend(BLEND_NONE);
	// Predefined modes
	const char* modes[] = { "none", "alpha", "add", "multiply" };
	int mode      = enumValue(e.attribute("mode"), 4, modes);
	int alphaMode = enumValue(e.attribute("alpha"), 4, modes);
	if(alphaMode<0) alphaMode = mode;
	if(mode>=0) blend = Blend((BlendMode)mode, (BlendMode)alphaMode);
	else {
		// Hmm, how to specify this? so many options
		// src, dst, srcalpha, dstalpha, operation
		// src*1 + (1-dst)
		mode = 0;
	}
	// Basic version for now
	return (BlendMode) mode;
}

Material* XMLResourceLoader::loadMaterial(const XMLElement& e, const char* name) {
	Material* m = 0;
	Pass* pass = 0;
	if(!name || !name[0]) name = e.attribute("name");

	// Extend an existing material
	const char* extend = e.attribute("extend");
	if(extend[0]) {
		Material* base = resources.materials.get(extend);
		if(base) {
			m = base->clone();
			pass = m->getPass(0);
			if(!pass) pass = m->addPass("default");
		} 
		else {
			printf("Error: Failed to find base material '%s' for material '%s'\n", extend, name);
			return 0;
		}
	}
	else {
		m = new Material();
		pass = m->addPass("default");
	}
	
	loadMaterialPass(e, pass);
	compilePass(pass, name);

	// Other passes
	for(const XMLElement& s : e) {
		if(s == "scheme") {
			pass = 0;
			const char* scheme = s.attribute("name");
			const char* external = s.attribute("material", (const char*)0);
			if(external && external[0]) {
				Material* ref = resources.materials.get(external);
				if(!ref) { printf("Could not locate material %s referenced by material %s\n", external, name); continue; }
				else pass = m->addPass(ref->getPass(0), scheme);
			}
			else if(external) { // set material="" to create blank pass
				pass = m->addPass(scheme);
			}
			else {	// omit material attribute to clone default pass
				pass = m->addPass(m->getPass(0), scheme);
			}
			loadMaterialPass(s, pass);
			compilePass(pass, name, scheme);
		}
	}

	return m;
}


Compositor* XMLResourceLoader::loadCompositor(const XMLElement& e) {
	int iw=0, ih=0;
	float fw=0, fh=0;
	const char* nullString = 0;
	// This should probably be defined in texture in case of changes
	static const char* formats[] = { "", "R8", "RG8", "RGB8", "RGBA8", "BC1", "BC2", "BC3", "BC4", "BC5",
									"R32F", "RG32F", "RGB32F", "RGBA32F", "R16F", "RG16F", "RGB16F", "RGBA16F",
									"R11G11B10F", "R5G6B5", "D16", "D24", "D32", "D24S8" };


	Compositor* c = new Compositor(e.attribute("name"));
	for(const XMLElement& i: e) {
		if(i=="buffer") {
			const char* name = i.attribute("name");
			const char* size = i.attribute("size", "1.0");
			bool unique = i.attribute("unique", 0);
			// size can be "2" "2 2", "2.0", "2.0 2.0"
			bool rel = strchr(size, '.');
			int n = rel? sscanf(size, "%g %g", &fw, &fh): sscanf(size, "%d %d", &iw, &ih);
			if(n==0) printf("Error: Invalid compositor buffer size for %s in %s\n", name, e.attribute("name"));
			else if(n==1) fh = fw, ih = iw;

			int f1 = enumValue(i.attribute("format1", i.attribute("format")), 24, formats);
			int f2 = enumValue(i.attribute("format2"), 24, formats);
			int f3 = enumValue(i.attribute("format3"), 24, formats);
			int f4 = enumValue(i.attribute("format4"), 24, formats);
			int fd = enumValue(i.attribute("depth"), 24, formats);

			if((rel&&(fw==0||fh==0)) || (!rel&&(iw==0||ih==0))) printf("Error: Invalid resolution for buffer %s of %s\n", name, e.attribute("name"));
			if(f1<0 || (f1==0 && fd==0)) printf("Error: Invalid format for buffer %s if %s\n", name, e.attribute("name"));

			if(rel) c->addBuffer(name, fw, fh, f1,f2,f3,f4,fd, unique);
			else c->addBuffer(name, iw, ih, f1,f2,f3,f4,fd, unique);
		}
		else if(i=="texture") {
			Texture* tex = resources.textures.get(i.attribute("source"));
			if(tex) c->addTexture(i.attribute("name"), tex);
			else printf("Error: Failed to load texture '%s' for compositor '%s'\n", i.attribute("source"), e.attribute("name"));
		}
		else if(i=="pass") {
			const char* type = i.attribute("type");
			const char* target = i.attribute("target");
			if(strcmp(type, "scene")==0) {
				iw = ih = i.attribute("queue", 0);
				iw = i.attribute("first", iw);
				ih = i.attribute("last", ih);
				const char* material = i.attribute("material", nullString);
				const char* technique = i.attribute("technique", nullString);
				c->addPass(target, new CompositorPassScene(iw, ih, material, technique));
			}
			else if(strcmp(type, "quad")==0) {
				const char* material = i.attribute("material");
				const char* technique = i.attribute("technique", nullString);
				CompositorPassQuad* pass = new CompositorPassQuad(material, technique);
				c->addPass(target, pass);
				
				Material* mat = 0;
				const char* shader = i.attribute("shader");
				if(shader[0]) {
					char tmp[512];
					const char* defines = i.attribute("defines");
					if(defines[0]) sprintf(tmp, "quad.vert,%s:%s", shader, defines);
					else sprintf(tmp, "quad.vert,%s", shader);

					mat = new Material();
					Pass* mpass = mat->addPass();
					mpass->setShader( resources.shaders.get(tmp) );
					pass->setMaterial(mat, 0, true);
					compileShader(mpass->getShader(), shader);
				}

				if(!shader && !material[0]) printf("Error: Compositior %s pass %s has no shader\n", e.attribute("name"), target);

				// textures
				for(const XMLElement& t: i) {
					if(t=="texture") {
						const char* name = t.attribute("name");
						const char* buffer = t.attribute("buffer", nullString);
						if(buffer) {
							int mrt = t.attribute("index", 0);
							if(mrt==0 && t.attribute("index")[0]=='d') mrt = -1;
							pass->setTexture(name, CompositorTextures::getInstance()->get(buffer, mrt), false);
						}
						else {
							buffer = t.attribute("file");
							Texture* tex = resources.textures.get(buffer);
							if(tex) pass->setTexture(name, tex, false);
						}
					}
					else if(mat && t=="variable") {
						parseShaderVariable(mat->getPass(0)->getParameters(), t);
					}
					else if(mat && t=="shared") {
						const char* name = t.attribute("name");
						ShaderVars* shared = resources.shaderVars.get(name);
						if(shared) mat->getPass(0)->addShared(shared);
					}
					else if(mat && t=="blend") {
						mat->getPass(0)->blend = parseBlendState(t);
					}
				}

				if(mat) compilePass(mat->getPass(0), e.attribute("name"), "(compositir pass)");
			}
			else if(strcmp(type, "clear")==0) {
				int flags = 0;
				if(i.attribute("colour", 1)) flags |= 1;
				if(i.attribute("depth", 1)) flags |= 2;
				if(i.attribute("stencil", 1)) flags |= 4;
				float depth = i.attribute("depth", 1.f);
				uint colour=i.attribute("value", 0u);
				c->addPass(target, new CompositorPassClear(flags, colour, depth));
			}
			else if(strcmp(type, "copy")==0) {
				c->addPass(target, new CompositorPassCopy(i.attribute("source")));
			}
			else {
				printf("Invalid compositor pass type '%s' in %s\n", type, e.attribute("name"));
			}
		}
		else if(i=="input") {
			const char* buffer = i.attribute("buffer");
			const char* name = i.attribute("name", buffer);
			if(!buffer[0]) buffer = name;
			if(!buffer[0]) printf("Compositor %s input has no buffer\n", e.attribute("name"));
			c->addInput(name, buffer);
		}
		else if(i=="output") {
			const char* buffer = i.attribute("buffer");
			const char* name = i.attribute("name", buffer);
			if(!buffer[0]) buffer = name;
			if(!buffer[0]) printf("Compositor %s output has no buffer\n", e.attribute("name"));
			c->addOutput(name, buffer);
		}
	}
	return c;
}

CompositorGraph* XMLResourceLoader::loadGraph(const XMLElement& e) {
	// TODO: aliasing for multiple instances of a composior
	CompositorGraph* chain = new CompositorGraph();
	const char* out = e.attribute("output", "out");
	for(const XMLElement& i: e) {
		if(i=="link") {
			const char* a = i.attribute("a");
			const char* b = i.attribute("b");
			Compositor* ca = resources.compositors.getIfExists(a);
			Compositor* cb = resources.compositors.getIfExists(b);
			if(strcmp(b, out)==0) cb = Compositor::Output;
			if(!ca || !cb) printf("Error: Missing compositor %s\n", ca? b: a);
			else {
				const char* key = i.attribute("key");
				const char* sa = i.attribute("sa");
				const char* sb = i.attribute("sb");
				if(sa[0] && sb[0]) {
					if(ca->getOutput(sa)<0) printf("Error: Compositor %s has no output %s\n", a, sa);
					if(cb->getInput(sb)<0) printf("Error: Compositor %s has no input %s\n", b, sb);
					chain->link(ca, cb, sa, sb);
				}
				else if(key[0]) {
					if(ca->getOutput(key)<0) printf("Error: Compositor %s has no output %s\n", a, key);
					if(cb->getInput(key)<0) printf("Error: Compositor %s has no input %s\n", b, key);
					chain->link(ca, cb, key);
				}
				else chain->link(ca, cb);
			}
		}
	}
	return chain;
}

// ================================================================================== //

void XMLResourceLoader::load(const XML& xml, const char* path) {
	for(const XMLElement& e: xml.getRoot()) {
		if(e.type()!=XML::TAG) continue;
		if(e=="include") {
			const char* file = e.attribute("file");
			char buffer[512];
			const char* end = strrchr(path, '/');
			if(end) snprintf(buffer, 512, "%.*s/%s", int(end-path), path, file);
			if((!end || !resources.loadFile(buffer)) && !resources.loadFile(file))
				printf("Failed to load resource file %s\n", file);
			continue;
		}

		const char* name = e.attribute("name");
		if(e=="material") {
			if(!name[0]) printf("Warning: Unnamed %s in %s\n", e.name(), path);
			Material* mat = loadMaterial(e, name[0]? name: path);
			resources.materials.add(name, mat);
		}
		else if(e=="compositor") {
			if(!name[0]) printf("Warning: Unnamed %s in %s\n", e.name(), path);
			Compositor* c = loadCompositor(e);
			resources.compositors.add(name, c);
		}
		else if(e=="graph") {
			if(!name[0]) printf("Warning: Unnamed %s in %s\n", e.name(), path);
			CompositorGraph* graph = loadGraph(e);
			resources.graphs.add(name, graph);
		}
		else if(e == "shared") {
			if(!name[0]) printf("Warning: Unnamed %s in %s\n", e.name(), path);
			bool changed = false;
			ShaderVars* vars = resources.shaderVars.getIfExists(name);
			if(!vars) {
				vars = new ShaderVars();
				resources.shaderVars.add(name, vars);
				changed = true;
			}
			for(const XMLElement& j: e) {
				parseShaderVariable(*vars, j);
				changed = true;
			}
			if(changed) {
				for(const auto& m: resources.materials) {
					for(Pass* p: *m.value) if(p->hasShared(vars)) compilePass(p, m.key, p->getName());
				}
			}
		}
		else {
			printf("Warning: Unknown resource type %s\n", e.name());
		}
	}
}
bool Resources::loadFile(const char* file) {
	XML xml = XML::load(file);
	if(xml.getRoot().size()==0) return false;
	XMLResourceLoader loader(this);
	loader.load(xml, file);
	return true;
}



// ================================================================================== //

Resources::Resources() {
	s_instance = this;
	// Setup default loaders
	textures.setDefaultLoader( new TextureLoader() );
	shaders.setDefaultLoader( new ShaderLoader(shaderParts) );
	shaderParts.setDefaultLoader( new ShaderPartLoader(shaders) );
	materials.setDefaultLoader( new MaterialLoader(this) );
	models.setDefaultLoader( new ModelLoader(this));
}

