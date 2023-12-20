#include <base/model.h>
#include <base/shader.h>
#include <base/material.h>
#include <base/resources.h>
#include <base/autovariables.h>
#include <base/hardwarebuffer.h>
#include <base/compositor.h>
#include <base/bmloader.h>
#include <base/texture.h>
#include <base/particles.h>
#include <base/particledef.h>
#include <base/script.h>
#include <base/file.h>
#include <base/png.h>
#include <base/dds.h>
#include <base/xml.h>
#include <base/thread.h>
#include <base/opengl.h>
#include <cstdio>
#include <list>

using namespace base;

Resources* Resources::s_instance = 0;

// Threading
Thread resourceThread;
Mutex resourceMutex;

//// ResourceManagerBase functions ////

ResourceManagerBase::~ResourceManagerBase() {
	for(size_t i=0; i<m_paths.size(); ++i) free(m_paths[i]);
}
void ResourceManagerBase::addPath(const char* path) {
	if(path && path[0]) m_paths.push_back( strdup(path) );
}

bool ResourceManagerBase::findFile(const char* name, char* out, size_t s) const {
	if(m_paths.empty()) {
		FILE* fp = fopen(name, "rb");
		if(fp) {
			fclose(fp);
			strcpy(out, name);
			return true;
		}
	}
	else for(size_t i=0; i<m_paths.size(); ++i) {
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

void Resources::addPath(const char* path) {
	models.addPath(path);
	textures.addPath(path);
	materials.addPath(path);
	shaders.addPath(path);
	particles.addPath(path);
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
	ResourceLoadProgress update() override;
	void updateT() override;
	protected:
	struct LoadMessage { Texture* target; char* file; PNG png; };
	std::list<LoadMessage> requests;
	std::list<LoadMessage> completed;
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
		// Lets try hacking in a quick threaded image loader
		if(resourceThread.running()) {
			auto suffix = [filename, ext](const char* s, int n) { // /.*[\._- ]n(:?orm(?:al)).png/
				if(ext - n - 1 <= filename) return false;
				char separator = ext[-n-1];
				if(separator != '.' && separator != '-' && separator != '_' && separator != ' ') return false;
				return strncmp(ext - n, s, n)==0;
			};
			uint hex = suffix("n", 1) || suffix("norm", 4) || suffix("normal", 6)? 0xff8080: 0xffffff;
			Texture* tex = new Texture(Texture::create(Texture::TEX2D, 1, 1, 1, Texture::RGB8, &hex));
			MutexLock lock(resourceMutex);
			requests.push_back({tex, strdup(filename)});
			return tex;
		}
		else {
			PNG png = PNG::load(filename);
			if(png.data && png.bpp<=32) {
				bool mipmaps = png.width == png.height;
				Texture::Format format = (Texture::Format) (png.bpp / 8);
				Texture tex = Texture::create(png.width, png.height, format, png.data, mipmaps);
				if(mipmaps) tex.setFilter( Texture::TRILINEAR );
				return new Texture(tex);
			}
			else {
				printf("Resource Error: Invalid png file '%s'\n", filename);
			}
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

void TextureLoader::updateT() {
	LoadMessage msg;
	{
	MutexLock lock(resourceMutex);
	if(requests.empty()) return;
	msg = requests.front();
	requests.pop_front();
	}

	printf("Loading %s\n", msg.file);
	msg.png = PNG::load(msg.file);

	MutexLock lock(resourceMutex);
	completed.push_back(std::move(msg));
}

ResourceLoadProgress TextureLoader::update() {
	MutexLock lock(resourceMutex);
	for(LoadMessage& msg: completed) {
		printf("Finished loading %s\n", msg.file);
		PNG& png = msg.png;
		if(png.data && png.bpp<=32) {
			bool mipmaps = png.width == png.height;
			Texture::Format format = (Texture::Format) (png.bpp / 8);
			Texture tex = Texture::create(png.width, png.height, format, png.data, mipmaps);
			if(mipmaps) tex.setFilter( Texture::TRILINEAR );
			*msg.target = tex;
		}
		else {
			printf("Resource Error: Invalid png file '%s'\n", msg.file);
		}
		free(msg.file);
	}
	ResourceLoadProgress r { (uint)completed.size(), (uint)requests.size() };
	completed.clear();
	return r;
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
	ShaderType type = FRAGMENT_SHADER;
	if(name[1]=='S' && name[2] == ':') switch(name[0]) {
	case 'F': type = FRAGMENT_SHADER; name+=2; break;
	case 'V': type = VERTEX_SHADER;   name+=2; break;
	case 'G': type = GEOMETRY_SHADER; name+=2; break;
		printf("Error: Invalid type specifier on shader part '%s'\n", name);
		return 0;
	}
	else if(strstr(name, ".vert") || strstr(name, ".vs")) type = VERTEX_SHADER;
	else if(strstr(name, ".frag") || strstr(name, ".fs")) type = FRAGMENT_SHADER;
	else if(strstr(name, ".geom") || strstr(name, ".gs")) type = GEOMETRY_SHADER;
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
	if(!file) {
		printf("Error: Missing shader file: '%s'\n", name);
		return 0;
	}

	// Create shader
	ShaderPart* part = new ShaderPart(type, file, defines);
	return part;
}

bool ShaderPartLoader::reload(const char* name, ShaderPart* data, Manager* manager) {
	ShaderPart* part = create(name, manager);
	if(part) *data = *part;
	delete part;
	return part != 0;
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
			// Name being a single glsl file, possibly with defines
			char vsKey[256], fsKey[256];
			sprintf(vsKey, "VS:%s", name);
			sprintf(fsKey, "FS:%s", name);
			ShaderPart* vs = m_partManager.getIfExists(vsKey);
			ShaderPart* fs = m_partManager.getIfExists(fsKey);

			if(!vs && !fs) {
				const char* defines = nullptr;
				if(const char* e = strchr(name, '|')) {
					strncpy(vsKey, name, e-name);
					vsKey[e-name] = 0;
					name = vsKey;
					defines = e + 1;
				}

				ResourceFile file(name, mgr);
				if(file) {
					vs = new ShaderPart(VERTEX_SHADER, file, defines);
					fs = new ShaderPart(FRAGMENT_SHADER, file, defines);
					//m_partManager.add(fsKey, fs);
					//m_partManager.add(vsKey, vs);
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
static MacroState parseMacroState(const XMLElement&);

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
// ----------------------------------------------------------------------------------- //

class ParticleLoader : public ResourceLoader<particle::System> {
	public:
	particle::System* create(const char*, Manager*) override;
	void destroy(particle::System* m) override { delete m; }
};
particle::System* ParticleLoader::create(const char* name, Manager* manager) {
	// Resolve filename
	char filename[1024];
	if(!manager->findFile(name, filename, 1024)) {
		printf("ParticleLoader: File not found %s\n", name);
		return 0;
	}

	script::Variable data;
	script::Script script;
	script.parse(File(filename).data());
	script.run(data);
	particle::System* system = particle::loadSystem(data.get("system"));
	return system;
}

// =================================================================================== //


static bool compileShader(Shader* shader, const char* name) {
	if(shader->isCompiled()) return true;

	int mask = 0;
	for(ShaderPart* p: shader->getParts()) mask |= 1 << p->getType(); 
	if((mask&3) != 3)
	{
		printf("Incomplete shader %s\n", name);
	}
	else if(!shader->compile()) {
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

template<int N, typename T> T enumValue(const char* value, const char* (&strings)[N], T defaultValue) {
	for(int i=0; i<N; ++i) if(strcmp(value, strings[i])==0) return (T)i;
	if(value && value[0]) printf("Error: Invalid resource enum value '%s'\n", value);
	return defaultValue;
}
template<int N, typename T=int> T enumValue(const char* value, const char* (&strings)[N]) {
	return enumValue(value, strings, (T)-1);
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
		else printf("Error: Shader file not found: %s\n", filename);
	}
	// additional source files
	for(const XMLElement& i: e) {
		if(i=="source") {
			filename = i.attribute("file");
			const char* shaderTypes[] = { "vertex", "fragment", "geometry" };
			int type = enumValue(i.attribute("type"), shaderTypes);
			ResourceFile file(filename, &resources.shaders);
			if(type<0) printf("Invalid shader attachment type %s\n", i.attribute("type"));
			else if(!file) printf("Error: Shader file not found: %s\n", filename);
			else {
				ShaderPart* ss = new ShaderPart((ShaderType)type, file, i.attribute("defines", defines));
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
		else if(i=="state") {
			pass->state = parseMacroState(i);
		}
	}
	pass->blend = parseBlendState(e);
}

static Blend parseBlendState(const XMLElement& material) {
	Blend blend;
	const char* presets[] = { "none", "alpha", "add", "multiply" };
	const char* constants[] = {
		"ZERO",
		"ONE",
		"SRC_COLOR",
		"ONE_MINUS_SRC_COLOR",
		"DST_COLOR",
		"ONE_MINUS_DST_COLOR",
		"SRC_ALPHA",
		"ONE_MINUS_SRC_ALPHA",
		"DST_ALPHA",
		"ONE_MINUS_DST_ALPHA",
		"SRC_ALPHA_SATURATE",
	};
	const uint values[] = {
		GL_ZERO,
		GL_ONE,
		GL_SRC_COLOR,
		GL_ONE_MINUS_SRC_COLOR,
		GL_DST_COLOR,
		GL_ONE_MINUS_DST_COLOR,
		GL_SRC_ALPHA,
		GL_ONE_MINUS_SRC_ALPHA,
		GL_DST_ALPHA,
		GL_ONE_MINUS_DST_ALPHA,
		GL_SRC_ALPHA_SATURATE,
	};

	for(const XMLElement& i: material) {
		if(i == "blend") {
			int buffer = i.attribute("buffer", -1);
			if(i.hasAttribute("mode")) {
				BlendMode mode      = enumValue(i.attribute("mode"), presets, BLEND_NONE);
				BlendMode alpha = enumValue(i.attribute("alpha"), presets, mode);
				if(buffer<0) return Blend(mode, alpha);
				else blend.set(buffer, mode, alpha);
			}
			else {
				int srcRGB = enumValue(i.attribute("src"), constants, 1);
				int dstRGB = enumValue(i.attribute("dst"), constants, 1);
				int srcAlpha = enumValue(i.attribute("srcAlpha"), constants, srcRGB);
				int dstAlpha = enumValue(i.attribute("dstAlpha"), constants, dstRGB);
				if(buffer<0) return Blend(values[srcRGB], values[dstRGB], values[srcAlpha], values[dstAlpha]);
				else blend.set(buffer, values[srcRGB], values[dstRGB], values[srcAlpha], values[dstAlpha]);
			}
		}
	}
	return blend;
}

static MacroState parseMacroState(const XMLElement& i) {
	MacroState state;
	const char* cullModes[] = { "none", "back", "front" };
	state.cullMode = enumValue(i.attribute("cull"), cullModes, CULL_BACK);
	const char* depthModes[] = { "always", "less", "lequal", "greater", "gequal", "equal", "disabled" };
	state.depthTest = enumValue(i.attribute("depth"), depthModes, DEPTH_LEQUAL);
	state.depthWrite = i.attribute("depthWrite", 1);	// perhaps use mask="RGBD"
	state.wireframe  = i.attribute("wireframe", 0);
	state.depthOffset = i.attribute("offset", 0.f);
	if(const char* mask = i.attribute("colour", nullptr)) {
		state.colourMask = MASK_NONE;
		if(strchr(mask, 'r')) state.colourMask |= MASK_RED;
		if(strchr(mask, 'g')) state.colourMask |= MASK_GREEN;
		if(strchr(mask, 'b')) state.colourMask |= MASK_BLUE;
		if(strchr(mask, 'a')) state.colourMask |= MASK_ALPHA;
	}
	return state;
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

			if((rel&&(fw==0||fh==0)) || (!rel&&(iw==0||ih==0))) printf("Error: Invalid resolution for buffer %s of %s\n", name, e.attribute("name"));

			// Buffer slots can be a format, or an input texture
			auto readSlot = [&i](Compositor::BufferAttachment& slot, const char* key, const char* key2=nullptr) {
				const char* value = i.attribute(key);
				if(!value[0] && key2) value = i.attribute(key2);
				Texture::Format f = enumValue(value, formats, Texture::NONE);
				if(f) slot.format = f;
				else if(value[0]) { // buffer:part
					slot.format = Texture::NONE;
					const char* split = strchr(value, ':');
					if(split) {
						if(split == value) return;
						if(strcmp(split+1, "depth")==0 || strcmp(split+1, "d")==0) slot.part = -1;
						else if(split[1]>='0' && split[1]<='3' && split[2]==0) slot.part = split[1] - '0';
						else return; // invalid
					}
					else split = value + strlen(value);
					int len = split - value;
					slot.input = new char[len+1];
					strncpy(slot.input, value, len);
					slot.input[len] = 0;
				}
			};

			Compositor::Buffer* buffer;
			if(rel) buffer = c->addBuffer(name, fw, fh, Texture::NONE);
			else buffer = c->addBuffer(name, iw, ih, Texture::NONE);
			buffer->unique = unique;
			readSlot(buffer->depth, "depth");
			readSlot(buffer->colour[0], "slot0", "format");
			readSlot(buffer->colour[1], "slot1");
			readSlot(buffer->colour[2], "slot2");
			readSlot(buffer->colour[3], "slot3");
			// Check buffer validity
			if(buffer->depth.format==0 && buffer->depth.input==0 && buffer->colour[0].format==0 && buffer->colour[0].input==0) {
				printf("Error: Invalid format for buffer %s of %s\n", name, e.attribute("name"));
			}

			if(i.hasAttribute("format1")) printf("Error: Old style compositor buffer format no longer supported!\n");
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
				CompositorPassScene* pass = new CompositorPassScene(iw, ih, material, technique);
				pass->setCamera(i.attribute("camera", nullptr));
				c->addPass(target, pass);
				// Additional material override properties
				if(i.find("blend")) pass->setBlend(parseBlendState(i));
				if(const XMLElement& e = i.find("state")) pass->setState(parseMacroState(e));
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
					if(defines[0]) sprintf(tmp, "quad.vert,%s|%s", shader, defines);
					else sprintf(tmp, "quad.vert,%s", shader);

					mat = new Material();
					Pass* mpass = mat->addPass("default");
					mpass->setShader( resources.shaders.get(tmp) );
					pass->setMaterial(mat, 0, true);
					compileShader(mpass->getShader(), shader);
				}
				else if(material[0]) {
					Material* m = resources.materials.get(material);
					if(m) pass->setMaterial(m, 0, false);
				}

				if(!shader && !material[0]) printf("Error: Compositior %s pass %s has no shader\n", e.attribute("name"), target);

				// textures
				if(mat) {
					Pass* mpass = mat->getPass(0);
					loadMaterialPass(i, mpass);
					compilePass(mpass, e.attribute("name"), "(compositor pass)");
				}
				// Load textures if material not yet found ?
				else for(const XMLElement& t: i) {
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
				}
			}
			else if(strcmp(type, "clear")==0) {
				ClearBits flags = (ClearBits)0;
				if(i.hasAttribute("stencil")) flags = flags | CLEAR_STENCIL;
				if(i.hasAttribute("colour")) flags = flags | CLEAR_COLOUR;
				if(i.hasAttribute("depth")) flags = flags | CLEAR_DEPTH;
				float depth = i.attribute("depth", 1.f);
				uint colour = i.attribute("colour", 0u);
				CompositorPassClear* clear = new CompositorPassClear(flags, colour, depth);
				clear->setColour(0, i.attribute("buffer0", colour));
				clear->setColour(1, i.attribute("buffer1", colour));
				clear->setColour(2, i.attribute("buffer2", colour));
				clear->setColour(3, i.attribute("buffer3", colour));
				c->addPass(target, clear);
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
	static const size_t nope = -1;
	CompositorGraph* chain = new CompositorGraph();

	HashMap<size_t> lookup;
	auto getCompositor = [this, chain, &lookup] (const char* name) {
		size_t r = lookup.get(name, nope);
		if(r == nope) {
			if(Compositor* c = resources.compositors.getIfExists(name)) {
				lookup[name] = r = chain->add(c);
			}
		}
		return r;
	};
	
	const char* out = e.attribute("output", "out");
	lookup[out] = chain->add(Compositor::Output);

	for(const XMLElement& i: e) {
		if(i=="link") {
			const char* a = i.attribute("a");
			const char* b = i.attribute("b");
			size_t ia = getCompositor(a);
			size_t ib = getCompositor(b);

			if(ia==nope) printf("Error: Missing compositor %s\n", a);
			if(ib==nope) printf("Error: Missing compositor %s\n", b);

			if(ia!=nope && ib!=nope) {
				const char* key = i.attribute("key");
				const char* sa = i.attribute("sa");
				const char* sb = i.attribute("sb");
				const Compositor* ca = chain->getCompositor(ia);
				const Compositor* cb = chain->getCompositor(ib);
				if(sa[0] && sb[0]) {
					if(ca->getOutput(sa)<0) printf("Error: Compositor %s has no output %s\n", a, sa);
					if(cb->getInput(sb)<0) printf("Error: Compositor %s has no input %s\n", b, sb);
					chain->link(ia, ib, sa, sb);
				}
				else if(key[0]) {
					if(ca->getOutput(key)<0) printf("Error: Compositor %s has no output %s\n", a, key);
					if(cb->getInput(key)<0) printf("Error: Compositor %s has no input %s\n", b, key);
					chain->link(ia, ib, key);
				}
				else chain->link(ia, ib);
			}
		}
		else if(i=="alias") {
			const char* name = i.attribute("name");
			const char* target = i.attribute("compositor");
			if(Compositor* c = resources.compositors.getIfExists(target)) {
				lookup[name] = chain->add(c);
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
	if(xml.getRoot().size()==0) {
		printf("Error: Failed to load resource file: %s\n", file);
		return false;
	}
	XMLResourceLoader loader(this);
	loader.load(xml, file);
	return true;
}



// ================================================================================== //

Resources::Resources() {
	s_instance = this;
	Shader::getSupportedVersion();
	// Setup default loaders
	textures.setDefaultLoader( new TextureLoader() );
	shaders.setDefaultLoader( new ShaderLoader(shaderParts) );
	shaderParts.setDefaultLoader( new ShaderPartLoader(shaders) );
	materials.setDefaultLoader( new MaterialLoader(this) );
	models.setDefaultLoader( new ModelLoader(this));
	particles.setDefaultLoader( new ParticleLoader() );
}

int Resources::update() {
	if(!resourceThread.running()) resourceThread.begin([this](int){
		while(true) {
			textures.getDefaultLoader()->updateT();
			Thread::sleep(1);
		}
	},0);

	ResourceLoadProgress r = textures.getDefaultLoader()->update();
	if(r.completed == 0 && r.remaining == 0) m_progress = 0;
	else if(r.completed && r.remaining == 0) m_progress = 1;
	else if(r.completed) m_progress += (1-m_progress) * (float)r.completed / (r.completed + r.remaining);
	return r.completed;
}

