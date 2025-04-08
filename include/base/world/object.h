#pragma once

#include <base/rtti.h>
#include <base/scene.h>
#include <base/variable.h>
#include <vector>

namespace base {

class Model;
class Material;
class AnimationController;
class AnimationBank;
class ObjectWorld;

class Object : public SceneNode {
	RTTI_BASE(Object)
	public:
	using Variable = script::Variable;
	Object(const script::Variable& data=Variable(), const char* name=0);
	virtual ~Object();
	virtual void onAdded() {} // Called whan added to zone
	virtual void onRemoved() {}
	virtual void update(float time) {}
	virtual bool trace(const Ray& ray, float radius, float& t, vec3& normal) const { return false; }
	virtual bool hit(float damage, Object* from) { return false; }
	script::Variable& getData() { return m_data; }
	const script::Variable& getData() const { return m_data; }
	ObjectWorld* getWorld() const { return m_world; }
	void setUpdate(bool);
	void setTraceGroup(int group) { m_traceGroup = group; }

	static Material* loadMaterial(const char* name, int weights=0, const char* base=0);
	static Material* loadMaterial(const char* name, const char* base) { return loadMaterial(name, 0, base); }
	static AnimationBank* getAnimationBank(const char* filename);
	static AnimationBank* getAnimationBank(Model*, bool create=true);
	static void storeAnimationBank(AnimationBank*, Model*);
	static int addAnimationsFromModel(AnimationBank* bank, Model* model, bool replaceExisting=false);
	static int addAnimationsFromModel(AnimationBank* bank, const char* modelFile, bool replaceExisting=false);
	static void addTextureSearchPattern(const char* variable, const char* pattern);
	protected:
	Model* loadModel(const char* name, AnimationController** anim, bool moves, const char* meshFilter=0, const char* material=0);
	Model* loadModel(const char* name, const char* meshFilter=0, const char* material=0) { return loadModel(name, nullptr, false, meshFilter, material); }

	protected:
	script::Variable m_data;

	private:
	friend class ObjectWorld;
	ObjectWorld* m_world = nullptr;
	int m_traceGroup = 0;
	bool  m_hasUpdate = false;
	float m_custom[4] = {0,0,0,0};
};

}


