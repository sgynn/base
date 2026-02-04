#include <base/world/object.h>
#include <base/world/objectworld.h>
#include <base/scene.h>
#include <base/model.h>
#include <base/shader.h>
#include <base/material.h>
#include <base/drawablemesh.h>
#include <base/animationcontroller.h>
#include <base/autovariables.h>
#include <base/resources.h>
#include <base/bmloader.h>
#include <base/string.h>
#include <base/xml.h>
#include <set>

using namespace base;
using namespace world;

/// Model extension for storing animationbanks
class AnimationBankExtension : public ModelExtension {
	BASE_MODEL_EXTENSION;
	AnimationBank* animations;
	AnimationBankExtension(base::AnimationBank* a) : animations(a) {}
	~AnimationBankExtension() { delete animations; }
};

class AnimationPropertyExtension : public base::ModelExtension {
	BASE_MODEL_EXTENSION;
	public:
	enum AnimationType { Action, Idle, Move };
	AnimationPropertyExtension(const base::XMLElement& animation) {
		strncpy(name, animation.attribute("name"), 63);
		const base::XMLElement& props = animation.find("properties");
		const char* typeName = props.attribute("type");
		if(strcmp(typeName, "move")==0) type = Move;
		else if(strcmp(typeName, "idle")==0) type = Idle;
		else type = Action;
	}
	char name[64];
	AnimationType type;
};


void world::storeAnimationBank(AnimationBank* bank, Model* model) {
	if(model->getExtension<AnimationBankExtension>()) printf("Error: Model already contains an animation bank\n");
	model->addExtension(new AnimationBankExtension(bank));
}

AnimationBank* world::getAnimationBank(const char* filename) {
	if(Model* m = Resources::getInstance()->models.get(filename))
		return getAnimationBank(m);
	return nullptr;
}

AnimationBank* world::getAnimationBank(Model* model, bool create) {
	AnimationBankExtension* data = model->getExtension<AnimationBankExtension>();
	if(!data) {
		if(!create) return nullptr;
		if(model->getAnimationCount()==0) return nullptr; // No animations to create
		AnimationBank* bank = new AnimationBank("root");
		data = new AnimationBankExtension(bank);
		model->addExtension(data);
		addAnimationsFromModel(bank, model);
	}
	return data->animations;
}

int world::addAnimationsFromModel(AnimationBank* bank, Model* model, bool replace) {
	if(!model) return 0;
	std::set<AnimationKey> added;
	for(size_t i=0; i<model->getAnimationCount(); ++i) {
		Animation* a = model->getAnimation(i);
		AnimationPropertyExtension::AnimationType type = AnimationPropertyExtension::Action;
		for(ModelExtension* e: model->getExtensions()) {
			if(AnimationPropertyExtension* prop = e->as<AnimationPropertyExtension>()) {
				if(strcmp(prop->name, a->getName())==0) {
					type = prop->type;
					break;
				}
			}
		}
		AnimationKey name = a->getName();
		if(replace && added.insert(name).second) bank->remove(name);
		bank->add(name, a, ~0u, 1, type == AnimationPropertyExtension::Move);
		if(type == AnimationPropertyExtension::Idle) bank->add("Idle", a);
	}
	return model->getAnimationCount();
}

int world::addAnimationsFromModel(AnimationBank* bank, const char* file, bool replace) {
	return addAnimationsFromModel(bank, Resources::getInstance()->models.get(file), replace);
}

AnimationKey getIdleAnimation(Model* model) {
	for(ModelExtension* e: model->getExtensions()) {
		if(AnimationPropertyExtension* prop = e->as<AnimationPropertyExtension>()) {
			if(prop->type == AnimationPropertyExtension::Idle) return AnimationKey(prop->name);
		}
	}
	return AnimationKey();
}

Drawable* world::attachMesh(SceneNode* node, Mesh* mesh, Material* material, int queue, float* customData) {
	DrawableMesh* d = new DrawableMesh(mesh, material);
	node->attach(d);
	d->setRenderQueue(queue);
	if(customData) d->setCustom(customData);
	return d;
}

Drawable* world::attachMesh(SceneNode* node, Mesh* mesh, const char* material, int queue, float* customData) {
	return attachMesh(node, mesh, loadMaterial(material), queue, customData);
}


// --------------------------------------------------------------------------- //


MaterialSettingsDef base::world::MaterialSettings = {
	{{"object.mat", nullptr, 0}},
	{{"diffuseMap", "%.*s.png"}, {"normalMap", "%.*s_n.png"}} 
};

void world::addBaseMaterial(const char* mat, const char* pattern, int queue) {
	if(pattern && strcmp(pattern, "*")==0) pattern = nullptr;
	for(auto& i: MaterialSettings.materials) {
		if(i.pattern == pattern || (i.pattern && pattern && strcmp(i.pattern, pattern)==0)) {
			i.file = mat;
			return;
		}
	}
	MaterialSettings.materials.push_back({mat, pattern, queue});
}

void world::addTextureSearchPattern(const char* var, const char* pattern) {
	assert(strstr(pattern, "%.*s")); // Texture search pattern must be of the form "%.*s_suffix.png"
	for(auto& i: MaterialSettings.textures) {
		if(strcmp(i.pattern, pattern)==0) {
			i.name = var;
			return;
		}
	}
	MaterialSettings.textures.push_back({var, pattern});
}

Material* world::loadMaterial(const char* name, int weights, const char* base) {
	Resources& res = *Resources::getInstance();
	if(name && name[0]) {
		char temp[64];
		const char* matName = name;
		Material* mat;
		if(strstr(name, ".mat")) return res.materials.get(name);
		else {
			if(weights > 0) { // Ensure weight count cannot be wrong
				sprintf(temp, "%s_W%d", name, weights);
				matName = temp;
			}
			mat = res.materials.getIfExists(matName);
		}

		if(!mat) {
			if(!base) for(const auto& p: MaterialSettings.materials) {
				if(!p.pattern || String::match(name, p.pattern)) base = p.file;
			}
			mat = loadMaterial(nullptr, weights, base)->clone();
			assert(mat);
			res.materials.add(matName, mat);

			if(name[0] == '#') {
				Texture* diff = res.textures.get(name);
				if(diff) mat->getPass(0)->setTexture("diffuseMap", diff);
			}
			else {
				// just in case name already has the file extension
				int len = strlen(name);
				if(len>4 && strcmp(name+len-4, ".png")==0) len -= 4;

				// Get textures
				char buffer[64];
				auto findTexture = [&res, &buffer, mat, len, name] (const char* pattern, const char* target) {
					sprintf(buffer, pattern, len, name);
					Texture* tex = res.textures.get(buffer);
					if(tex) mat->getPass(0)->setTexture(target, tex);
					return tex;
				};

				//findTexture("%.*s.png", "diffuseMap") || findTexture("%.*s.dds", "diffuseMap");
				//findTexture("%.*s_n.png", "normalMap") || findTexture("%.*s_n.dds", "normalMap");
				for(const auto& m: MaterialSettings.textures) {
					findTexture(m.pattern, m.name);
				}
			}

			mat->getPass(0)->compile();
		}
		return mat;
	}
	else {
		if(!base) base = MaterialSettings.materials[0].file;
		Material* baseMaterial = res.materials.get(base);
		if(weights==0 || !baseMaterial) return baseMaterial;
		else {
			assert(weights>0 && weights <= 4);
			char buffer[64];
			sprintf(buffer, "%s_W%d", base, weights);
			if(Material* mat = res.materials.getIfExists(buffer)) return mat;
			printf("Creating %s material for %d weights\n", base, weights);
			Material* mat = baseMaterial->clone();
			res.materials.add(buffer, mat);
			// add new define to shader
			for(Pass* pass: *mat) {
				Shader* old = pass->getShader();
				const char* shaderName = res.shaders.getName(old);
				const char* key = "SKINNED";
				Shader* newShader = nullptr;
				if(shaderName) {
					if(strchr(shaderName, '|')) sprintf(buffer, "%s,%s %d", shaderName, key, weights);
					else sprintf(buffer, "%s|%s %d", shaderName, key, weights);
					newShader = res.shaders.get(buffer);
				}
				else { // Unnamed shader - make a copy
					sprintf(buffer, "%s %d", key, weights);
					newShader = new Shader();
					for(ShaderPart* s: old->getParts()) newShader->attach(new ShaderPart(*s));
					newShader->bindDefaultAttributeLocations();
					newShader->define(buffer);
				}
				pass->getParameters().setAuto("boneMatrices", AUTO_SKIN_MATRICES);
				pass->setShader(newShader);
				pass->compile();
			}
			return mat;
		}
	}
}


Model* world::attachModel(SceneNode* node, const char* file, AnimationController** animated, bool moves, MeshFilter&& filter, const char* baseMaterial,const char* overrideMaterial, float* custom) {
	static bool init = false;
	if(!init) {
		base::BMLoader::registerExtension<AnimationPropertyExtension>("animation");
		init = true;
	}

	auto checkFilter = [&filter](const char* mesh) {
		for(const char* val : filter.values) {
			bool matched = false;
			if(String::match(mesh, val)) {
				if(filter.mode == INCLUDE) return true;
				matched = true;
			}
			if(matched && filter.mode == EXCLUDE) return false;
		}
		return filter.mode == EXCLUDE;
	};

	assert(node);
	Model* model = Resources::getInstance()->models.get(file);
	if(model) {
		if(!model->getSkeleton() && animated && !*animated) animated = nullptr;
		for(const Model::MeshInfo& mesh: model->meshes()) {
			if(!checkFilter(mesh.name)) continue;
			DrawableMesh* drawable = new DrawableMesh(mesh.mesh);
			const char* materialName = overrideMaterial? overrideMaterial: mesh.materialName;
			int weights = animated? mesh.mesh->getWeightsPerVertex(): 0;
			drawable->setMaterial( loadMaterial(materialName, weights, baseMaterial) );
			drawable->setCustom(custom);
			if(animated && weights) {
				if(!*animated) {
					*animated = new AnimationController();
					(*animated)->initialise(model, getAnimationBank(model), moves);
					if((*animated)->getAnimations()) {
						(*animated)->setIdle(getIdleAnimation(model));
					}
				}
				drawable->setupSkinData((*animated)->getSkeleton());
			}
			for(const auto& m: MaterialSettings.materials) {
				if(!m.pattern || String::match(materialName, m.pattern)) drawable->setRenderQueue(m.queue);
			}
			node->attach(drawable);
		}
	}
	return model;
}


// ======================================================================================= //


WorldObjectBase::~WorldObjectBase() {
	deleteAttachments(true);
}

void WorldObjectBase::setUpdate(bool u) {
	if(m_world && u != m_hasUpdate) {
		if(u) m_world->m_messages.push_back({ObjectWorldBase::Message::StartUpdate, this});
		else  m_world->m_messages.push_back({ObjectWorldBase::Message::StopUpdate,  this});
	}
	else m_hasUpdate = u;
}

