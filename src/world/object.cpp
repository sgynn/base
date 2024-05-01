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
#include <base/xml.h>

using namespace base;
using script::Variable;

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



AnimationBank* Object::getAnimationBank(const char* filename) {
	if(Model* m = Resources::getInstance()->models.get(filename))
		return getAnimationBank(m);
	return nullptr;
}
AnimationBank* Object::getAnimationBank(Model* model) {
	AnimationBankExtension* data = model->getExtension<AnimationBankExtension>();
	if(!data) {
		if(model->getAnimationCount()==0) return nullptr; // No animations to create
		AnimationBank* bank = new AnimationBank("root");
		data = new AnimationBankExtension(bank);
		model->addExtension(data);
		addAnimationsFromModel(bank, model);
	}
	return data->animations;
}
int Object::addAnimationsFromModel(AnimationBank* bank, Model* model) {
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
		const char* name = a->getName();
		if(type == AnimationPropertyExtension::Idle) name = "Idle";
		bank->add(name, a, ~0u, 1, type == AnimationPropertyExtension::Move);
	}
	return model->getAnimationCount();
}
int Object::addAnimationsFromModel(AnimationBank* bank, const char* file) {
	if(Model* model = Resources::getInstance()->models.get(file)) {
		return addAnimationsFromModel(bank, model);
	}
	return 0;
}
AnimationKey getIdleAnimation(Model* model) {
	for(ModelExtension* e: model->getExtensions()) {
		if(AnimationPropertyExtension* prop = e->as<AnimationPropertyExtension>()) {
			if(prop->type == AnimationPropertyExtension::Idle) return AnimationKey(prop->name);
		}
	}
	return AnimationKey();
}


Object::Object(const Variable& data, const char* name) {
	setName(name? name: (const char*)data.get("name"));
	m_data.set("data", data);
	m_data.link("position", m_position);
}

Object::~Object() {
	deleteAttachments();
}

Material* Object::loadMaterial(const char* name, int weights, const char* base) {
	Resources& res = *Resources::getInstance();
	if(name && name[0]) {
		char temp[64];
		const char* matName = name;
		Material* mat;
		if(strstr(name, ".mat")) mat = res.materials.get(name);
		else {
			if(weights > 0) { // Ensure weight count cannot be wrong
				sprintf(temp, "%s_W%d", name, weights);
				matName = temp;
			}
			mat = res.materials.getIfExists(matName);
		}

		if(!mat) {
			mat = loadMaterial(base, weights)->clone();
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

				findTexture("%.*s.png", "diffuseMap") || findTexture("%.*s.dds", "diffuseMap");
				findTexture("%.*s_n.png", "normalMap") || findTexture("%.*s_n.dds", "normalMap");
			}

			mat->getPass(0)->compile();
		}
		return mat;
	}
	else if(base) {
		return loadMaterial(base, weights);
	}
	else {
		if(weights==0) return res.materials.get("object.mat");
		else {
			assert(weights>0 && weights <= 4);
			char buffer[32];
			sprintf(buffer, "object.mat_W%d", weights);
			if(Material* mat = res.materials.getIfExists(buffer)) return mat;
			printf("Creating base material for %d weights\n", weights);
			Material* mat = loadMaterial(0,0,0)->clone();
			res.materials.add(buffer, mat);
			sprintf(buffer, "object.glsl|SKINNED %d", weights); // FIXME
			Shader* newShader = res.shaders.get(buffer);
			mat->getPass(0)->getParameters().setAuto("boneMatrices", AUTO_SKIN_MATRICES);
			mat->getPass(0)->setShader(newShader);
			mat->getPass(0)->compile();
			return mat;
		}
	}
}


Model* Object::loadModel(const char* file, AnimationController** animated, bool moves, const char* filter, const char* overrideMaterial) {
	static bool init = false;
	if(!init) {
		base::BMLoader::registerExtension<AnimationPropertyExtension>("animation");
		init = true;
	}

	Model* model = Resources::getInstance()->models.get(file);
	if(model) {
		if(!model->getSkeleton()) animated = nullptr;
		for(int i=0; i<model->getMeshCount(); ++i) {
			Mesh* mesh = model->getMesh(i);
			if(filter && strcmp(model->getMeshName(i), filter)!=0) continue;
			if(strncmp(model->getMeshName(i), "COLLISION", 9)==0) continue;
			DrawableMesh* d = new DrawableMesh(mesh);
			const char* materialName = overrideMaterial? overrideMaterial: model->getMaterialName(i);
			int weights = animated? mesh->getWeightsPerVertex(): 0;
			d->setMaterial( loadMaterial(materialName, weights) );
			d->setCustom(m_custom);
			if(animated && weights) {
				if(!*animated) {
					*animated = new AnimationController();
					(*animated)->initialise(model, getAnimationBank(model), moves);
					if((*animated)->getAnimations()) {
						(*animated)->setIdle(getIdleAnimation(model));
					}
				}
				d->setupSkinData((*animated)->getSkeleton());
			}
			attach(d);
		}
	}
	return model;
}

void Object::setUpdate(bool u) {
	if(!m_world) m_hasUpdate = u;
	else if(u != m_hasUpdate) m_world->setUpdate(this, u);
}

