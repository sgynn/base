#ifndef _BASE_MODEL_
#define _BASE_MODEL_

#include "math.h"
#include "mesh.h"
#include "morph.h"
#include "skeleton.h"
#include "animation.h"
#include "hashmap.h"
#include <vector>

namespace base {
namespace model {

	
	/** Model class.
	 * Contains automation animation, morphs, and mesh processing functions 
	 * A model contains INPUT, OUTPUT meshes
	 * Input meshes are source meshes for skinning and morphing. They can be shared between meshes
	 * Output meshes are the results of the above that are drawn.
	 * If a model has no deformations then there will be no input meshes.
	 *
	 * Model has static functions for processing so can be used externally.
	 *
	 * A model may have a single skeleton.
	 * 
	 *
	 * */
	class Model {
		public:
		Model();
		Model(const Model&);
		~Model();

		int         getMeshCount() const;				/**< Get the number of output meshes in this model */
		Mesh*       getMesh    (int index);				/**< Get output mesh by index */
		Mesh*       getMesh    (const char* name);		/**< Get output mesh by name */
		Mesh*       getSource  (int index) const;		/**< Get source mesh by index */
		const Bone* getMeshBone(int index) const;		/**< Get bone this mesh is attached to */

		int addMesh(Mesh* mesh, Bone* bone=0);					/**< Add an output mesh to the model attached to a bone */
		int addMesh(Mesh* mesh, const char* name, Bone* bone=0);	/**< Add a named output mesh to the model */
		int addSkin(Mesh* input, const char* name=0);			/**< Add a skinned mesh to the model */
		
		void setSkeleton(Skeleton* skel);						/**< Set the model skeleton */
		Skeleton* getSkeleton() const;							/**< Get the model skeleton */


		void  update(float time);										/** Update all automations */
		void  draw() const;												/** Draw all meshes */

		// Animation automation
		void       addAnimation(Animation*);							/**< Add an animation to the model. If name not specified, uses the name from the animation object */
		Animation* getAnimation(const char* name) const;				/**< Get animation by name */
		void       setAnimationSpeed(float speed, const Bone* b=0);		/**< Set the current animation speed multiplier. fps is defined in Animation */
		float      getAnimationSpeed(const Bone* b=0) const;			/**< Get current speed of the animation */
		void       setAnimation(const Animation*, const Bone* b=0);		/**< Start an animation */
		void       setAnimation(const char* name, const Bone* b=0);		/**< Start an animation */
		void       setFrame(float frame, const Bone* b=0);				/**< Set the current frame of the animation */
		float      getFrame(const Bone* b=0) const;						/**< Get the currentanimation frame */
		int        getFrameCount(const Bone* b=0) const;				/**< Get the number of frames in the active animation */
		bool       animationRunning(const Bone* b=0) const;				/**< Is the animation over? */

		// Morph automation
		int   setMorph(int mesh,         Morph* morph, float value);	/**< Add a morph to a mesh. */
		int   setMorph(const char* mesh, Morph* morph, float value); 	/**< Add a morph to a mesh. */
		void  setMorphValue(const char* name, float value);				/**< Set the value of a morph */
		float getMorphValue(const char* name) const;					/**< Get the value of a morph */
		void  startMorph(const char* name, float target, float time);	/**< Start an animated morph */
		bool  morphRunning(const char* name) const;						/**< Is an animated morph running */


		// Mesh processing
		static void skinMesh(Mesh* out, const Mesh* source, const Skeleton* skel);
		static void morphMesh(Mesh* out, const Mesh* source, const Morph* morph, float weight);

		// Drawing
		static void drawMesh(const Mesh* mesh);							/**< Draw a mesh */
		static int  drawMesh(const Mesh* mesh, int state);				/**< Draw a mesh using clientState state. Returns ending clientState */
		static void drawSkeleton(const Skeleton*);						/**< Draw the skeleton */
		static void drawBone(const Bone* bone);							/**< Draw a bone. Assumes clientState is correct */

		protected:
		struct MorphInfo {
			const Morph* morph;				// Morph vertex data
			uint         mesh;				// Mesh applies to
			float        value;				// Morph weight
			float        target;			// Target weight
			float        speed;				// Weight change per frame
		};
		struct AnimInfo {
			const Animation* animation;		// Current animation
			const Bone*      bone;			// Bone to play this animation from
			float            frame;			// Current frame
			float            speed;			// Animation speed (multiplier for Animation::getSpeed();)
		};
		struct MeshInfo {
			const Bone* bone;				// Bone attached to
			Mesh*       source;				// Mesh source
			Mesh*       morphed;			// Morphed mesh
			Mesh*       skinned;			// Skinned mesh
			Mesh*       output;				// Output mesh - points to source, morphed or skinned
			bool        morphChanged;		// Do the morphs need recalculating
		};

		std::vector<MeshInfo>  m_meshes;	// List of meshes
		std::vector<MorphInfo> m_morphs;	// List of all morphs
		std::vector<AnimInfo>  m_animations;// List of all running animations
		Skeleton*              m_skeleton;	// Model skeleton

		/** Maps of Name -> Index for all named objects in the model
		 *  A separate object so it can be shared between instances of this model
		 * */
		struct Maps {
			HashMap<int> meshes;
			HashMap<int> morphs;
			HashMap<Animation*> animations;
			int ref;
		}* m_maps;

		template<typename T> T mapValue(const HashMap<T>& map, const char* name, T def) const;
		const AnimInfo* boneAnimation(const Bone* bone) const;						// Get AnimInfo object for a bone
		AnimInfo*       boneAnimation(const Bone* bone);							// Get AnimInfo object for a bone
		void            morphMesh(MeshInfo& mesh, MorphInfo& morph, bool clean);	// Morph a mesh.

	};


	// Inline implementation
	
	template<typename T> inline T Model::mapValue(const HashMap<T>& map, const char* name, T def) const {
		typename HashMap<T>::const_iterator i = map.find(name);
		return i==map.end()? def: *i;
	}

	inline int        Model::getMeshCount() const   { return m_meshes.size(); }
	inline Mesh*      Model::getMesh(int index)     { return m_meshes[index].output; }
	inline Mesh*      Model::getMesh(const char* n) { int i=mapValue(m_maps->meshes, n, -1); return i<0? 0: getMesh(i); }
	inline Mesh*      Model::getSource(int index) const      { return m_meshes[index].source; }
	inline const Bone*Model::getMeshBone(int index) const    { return m_meshes[index].bone; }
	inline int        Model::addMesh(Mesh* mesh, Bone* bone) { return addMesh(mesh, 0, bone); };
	inline Skeleton*  Model::getSkeleton() const             { return m_skeleton; }

	inline void       Model::addAnimation(Animation* a)                    { m_maps->animations[ a->getName() ] = a; a->grab(); }
	inline Animation* Model::getAnimation(const char* name) const          { return mapValue(m_maps->animations, name, (Animation*)0); }
	inline void       Model::setAnimation(const char* n, const Bone* b)    { setAnimation(getAnimation(n), b); }
	inline void       Model::setAnimationSpeed(float speed, const Bone* b) { AnimInfo* i = boneAnimation(b); if(i) i->speed = speed; }
	inline void       Model::setFrame(float frame, const Bone* b)          { AnimInfo* i = boneAnimation(b); if(i) i->frame = frame; }
	inline float      Model::getAnimationSpeed(const Bone* b) const        { const AnimInfo* i = boneAnimation(b); return i? i->speed: 1; }
	inline float      Model::getFrame(const Bone* b) const                 { const AnimInfo* i = boneAnimation(b); return i? i->frame: 0; }
	inline int        Model::getFrameCount(const Bone* b) const            { const AnimInfo* i = boneAnimation(b); return i? i->animation->getLength(): 0; }
	inline bool       Model::animationRunning(const Bone* b) const         { const AnimInfo* i = boneAnimation(b); return i? i->speed!=0: false; }

	inline int        Model::setMorph(const char* n, Morph* m, float v)    { int i=mapValue(m_maps->meshes, n, -1); return i<0? -1: setMorph(i,m,v); }
	inline float      Model::getMorphValue(const char* m) const            { int i=mapValue(m_maps->morphs,m,-1); return i<0? 0: m_morphs[i].value; }
	inline bool       Model::morphRunning(const char* m) const             { int i=mapValue(m_maps->morphs,m,-1); return i<0? false: m_morphs[i].value != m_morphs[i].target; }
	

};
};

#endif

