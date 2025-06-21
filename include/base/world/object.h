#pragma once

#include <base/scene.h>

/*
 *  Object world is an abstract utility.
 *  Usage:
 *  	class Level;
 *  	class Object : public base::world::WorldObject<Level> { };
 *
 *  	using TraceResult = TraceResultT<Object>;
 *  	class Level : public base::world::ObjectWorld<Object> { };
 *
 *  This allows your world class to use your object type as if it were the base class
 */


namespace base {

class Mesh;
class Model;
class Material;
class AnimationController;
class AnimationBank;

enum Inclusion { EXCLUDE, INCLUDE };
struct MeshFilter {
	Inclusion mode;
	std::vector<const char*> values;
	MeshFilter() : mode(EXCLUDE) {}
	MeshFilter(const char* m) : mode(m? INCLUDE: EXCLUDE) { if(m) values = {m}; }
	MeshFilter(Inclusion i, const char* m) : mode(i), values{m} {}
	MeshFilter(Inclusion i, std::initializer_list<const char*> m) : mode(i), values(m) {}
};
struct ObjectTraceResult { float t=-1; vec3 normal; operator bool()const{return t>=0;} };

namespace world {


	// Helper functions //
	extern void addTextureSearchPattern(const char* variable, const char* pattern);
	extern Material* loadMaterial(const char* name, int weights=0, const char* base=0);
	inline Material* loadMaterial(const char* name, const char* base) { return loadMaterial(name, 0, base); }
	extern AnimationBank* getAnimationBank(const char* filename);
	extern AnimationBank* getAnimationBank(Model*, bool create=true);
	extern void storeAnimationBank(AnimationBank*, Model*);
	extern int addAnimationsFromModel(AnimationBank* bank, Model* model, bool replaceExisting=false);
	extern int addAnimationsFromModel(AnimationBank* bank, const char* modelFile, bool replaceExisting=false);
	extern Model* attachModel(SceneNode* target, const char* file, AnimationController** animated=nullptr, bool moves=false, MeshFilter&& meshFilter={}, const char* overrideMaterial=nullptr, float* customData=nullptr);
	extern Drawable* attachMesh(SceneNode* target, Mesh* mesh, const char* material = nullptr);
	extern Drawable* attachMesh(SceneNode* target, Mesh* mesh, Material* material);

	class ObjectWorldBase;
	class WorldObjectBase : public SceneNode {
		public:
		using ObjectTraceResult = base::ObjectTraceResult;

		WorldObjectBase() = default;
		virtual ~WorldObjectBase();
		virtual void onAdded() {} // Called whan added to zone
		virtual void onRemoved() {}
		virtual void update(float time) {}
		virtual ObjectTraceResult trace(const Ray& ray, float radius, float limit) const { return {}; }

		void setUpdate(bool);
		void setTraceGroup(int group) { m_traceGroup = group; }

		// Redirectors for legacy access
		static void addTextureSearchPattern(const char* variable, const char* pattern)		{ world::addTextureSearchPattern(variable, pattern); }
		static Material* loadMaterial(const char* name, int weights=0, const char* base=0)	{ return world::loadMaterial(name, weights, base); }
		static Material* loadMaterial(const char* name, const char* base)					{ return world::loadMaterial(name, 0, base); }
		static AnimationBank* getAnimationBank(const char* filename)						{ return world::getAnimationBank(filename); }
		static AnimationBank* getAnimationBank(Model* model, bool create=true)				{ return world::getAnimationBank(model, create); }
		static void storeAnimationBank(AnimationBank* bank, Model* model)					{ world::storeAnimationBank(bank, model); }
		static int addAnimationsFromModel(AnimationBank* bank, Model* model, bool replaceExisting=false) { return world::addAnimationsFromModel(bank, model, replaceExisting); }
		static int addAnimationsFromModel(AnimationBank* bank, const char* modelFile, bool replaceExisting=false) { return world::addAnimationsFromModel(bank, modelFile, replaceExisting); }

		protected:
		Model* loadModel(const char* name, AnimationController** animated, bool moves, MeshFilter&& filter={}, const char* material=0) { return attachModel(this, name, animated, moves, std::forward<MeshFilter>(filter), material, m_custom); }
		Model* loadModel(const char* name, MeshFilter&& filter={}, const char* material=0) { return loadModel(name, nullptr, false, std::forward<MeshFilter>(filter), material); }

		private:
		template<class W> friend class WorldObject;
		friend class ObjectWorldBase;
		ObjectWorldBase* m_world = nullptr;
		bool m_hasUpdate = false;
		int m_traceGroup = 0;
		protected:
		vec4 m_custom = 0.f;
	};


	template<class WorldClass>
	class WorldObject : public world::WorldObjectBase {
		public:
		WorldClass* getWorld() const { return (WorldClass*)m_world; } // c cast as WorldClass will be undefined
	};
}

}


