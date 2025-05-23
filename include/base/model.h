#pragma once

#include "mesh.h"
#include "skeleton.h"
#include "animation.h"
#include "modellayout.h"
#include "animationstate.h"

namespace base {

/** Model extension base class - extend for loading additional data from model files eg layout, ragdoll, etc */
class ModelExtension {
	public:
	virtual ~ModelExtension() {}
	virtual int getType() const = 0;
	template<class T> T* as() { return getType()==T::staticType()? static_cast<T*>(this): 0; }
};
#define BASE_MODEL_EXTENSION public: \
	static int staticType() { static int typeID = __COUNTER__; return typeID; } \
	int getType() const { return staticType(); }
	

/** Model class
 * 	Contains meshes, morphs, skeletons, anomations
 *  Meshes in the list can be tagged as hidden, to be used as inputs to deformed meshes
 *  Model acts a an interface for everything else
 *  Also support lod and stuff by additional data in the mesh descriptor
 * */
class Model {
	public:
	Model();
	~Model();

	int   getMeshCount() const;				/// Number of meshes in the model
	Mesh* getMesh(int index) const;			/// Get mesh by index
	Mesh* getMesh(const char* name) const;	/// Get mesh by name
	int   getMeshIndex(const char* name) const; // Get index of named mesh. returns -1 if not found

	int   addMesh(const char* name, Mesh*, const char* materialName=0);

	void  removeMesh(const char* name);
	void  removeMesh(const Mesh*);
	void  removeMesh(int index);

	const char* getMeshName(int index) const;
	const char* getMeshName(const Mesh*) const;

	// Material
	void  setMaterialName(const char* m);
	void  setMaterialName(int meshIndex, const char* m);
	const char* getMaterialName(int index) const;
	const char* getMaterialName(const Mesh* mesh) const;
	const char* getMaterialName(const char* meshName) const;
	
	// Skeleton
	void setSkeleton(Skeleton*);
	Skeleton* getSkeleton() const;
	
	// ToDo: Morphs
	// ToDo: Blended animations ?
	
	void       addAnimation(Animation*);			// Add animation to model
	Animation* getAnimation(const char*) const;		// Get animation by name
	Animation* getAnimation(int index) const;		// Get animation by index
	size_t     getAnimationCount() const;			// Get number of animations

	AnimationState* getAnimationState() const;
	void            setAnimationState(AnimationState*);

	/// Update all automated things, and update any requited deformations
	void update(float time=1/60.f);

	static Mesh* createTargetMesh(const Mesh* src);									// Create a new mesh to use for software skinning or morphs
	static void skinMesh(Mesh* in, const Skeleton*, int* map, Mesh* out);		// Software skinning - deform a mesh
	static int* createSkinMap(const Skeleton*, const Mesh*);					// Create Bone->Skin map

	static void resetTargetMesh(Mesh* target, const Mesh* source);
	static void applyMorph(Mesh* target, const Mesh* source, int index, float amount);

	const ModelLayout* getLayout() const { return m_layout; }
	void setLayout(ModelLayout* l) { delete m_layout; m_layout = l; }

	// Extensions
	void addExtension(ModelExtension* e);
	const std::vector<ModelExtension*>& getExtensions() const { return m_extensions; }
	template<class T> T* getExtension(int index=0) { for(ModelExtension* e: m_extensions) { if(e->getType()==T::staticType() && --index==-1) return e->as<T>(); } return 0; }

	struct MeshInfo {
		Mesh* mesh;
		char* name;
		char* materialName;
		int* skinMap;
	};

	// Collectons to be iterated over
	const std::vector<Animation*> animations() const { return m_animations; }
	const std::vector<MeshInfo> meshes() const { return m_meshes; }

	protected:
	std::vector<MeshInfo> m_meshes;
	Skeleton* m_skeleton = nullptr;
	ModelLayout* m_layout = nullptr;

	// Animations
	std::vector<Animation*> m_animations;	// All animations
	AnimationState* m_animationState = nullptr;

	// Extensions
	std::vector<ModelExtension*> m_extensions;
};

}

