#ifndef _BASE_SKELETON_
#define _BASE_SKELETON_

#include "math.h"
#include "quaternion.h"

namespace base {
namespace model {

	class Skeleton;
	class Animation;
	
	/** Bone class */
	class Bone {
		public:
		
		/** Bone update mode */
		enum Mode {
			DEFAULT,	/**< Bone will use animation unless descended from TRUNCATE type */
			ANIMATED,	/**< Uses animation, overrides truncate */
			TRUNCATE,	/**< Calls to animate() stops here unless this is the animation root */
			USER,		/**< Bone will always use values set by the user. Ignored by animation */
			FIXED,		/**< Bone absolute matrix set by user. Note: bone get() values will be wrong */
		};


		Mode        getMode() const;		/**< Get the update mode */
		void        setMode(Mode m);		/**< Set the update mode */
		int         getIndex() const;		/**< Get the bone index in the skeleton */
		const char* getName() const;		/**< Get the bone name */
		float       getLength() const;		/**< Get the rest length */
		void        setLength(float len);	/**< Set the rest length */
		Bone*       getParent();			/**< Get the parent bone */

		const vec3&       getPosition() const;	/**< Get the relative position */
		const vec3&       getScale() const;		/**< Get the relative scale */
		const vec3        getEuler() const;		/**< Get the relative angle as Pitch,Yaw,Roll */
		const Quaternion& getAngle() const;		/**< Get the relative rotation as a quaternion */
		const Matrix&     getTransformation() const;			/**< Get the local transformation of the bone */
		const Matrix&     getAbsoluteTransformation() const;	/**< Get absolute transformation */

		void setPosition(const vec3& pos);		/**< Set the relative posiiton */
		void setScale(float scale);				/**< Set the relative scale */
		void setScale(const vec3& scale);		/**< Set the relative scale */
		void setEuler(const vec3& pyr);			/**< Set the relative angle from pitch,yaw,roll */
		void setAngle(const Quaternion& q);		/**< Set the relative angle quaternion */
		void setTransformation(const Matrix&);	/**< Set the local transformation */
		void setAbsoluteTransformation(const Matrix&);	/**< Set the absolute transformation */

		private:
		friend class Skeleton;

		Skeleton*   m_skeleton;	// Skeleton object this bone belongs to
		int         m_index;	// Skeleton index
		int         m_parent;	// Bone parent index
		const char* m_name;		// Bone name
		float       m_length;	// Rest length
		Mode        m_mode;		// Update behavious mode
		int         m_state;	// Which parts have been changed so need updating

		Matrix      m_local;	// Local matrix (cached)
		Matrix      m_combined; // Absolute matrix
		vec3        m_position;
		vec3        m_scale;
		Quaternion  m_angle;

		Bone();						/**< Private constructor - create from Skeleton */
		void        updateLocal();  // Calculate outdated local variables
	};

	/** Skeleton class 
	 * The skeleton will always contain a root bone at 0
	 * A bone's parent will always have a smaller index
	 * */
	class Skeleton {
		public:
		Skeleton();							/**< Default constructor */
		Skeleton(const Skeleton&);			/**< Copy constructor */
		~Skeleton();						/**< Destructor */

		Bone*       addBone(const Bone* parent, const char* name=0, const float* local=0);	/**< Add a new bone to the skeleton */
		Bone*       getBone(int index);				/**< Get bone by index */
		const Bone* getBone(int index) const;	/**< Get bone by index */
		Bone*       getBone(const char *name);		/**< Get bone by name */
		int         getBoneCount() const;				/**< Get number of bones in skeleton */
		void        setMode(Bone::Mode m);			/**< Set all bone update modes */

		void update();							/**< Calculate bone matrices */

		void animate(const Animation* anim, float frame, const Bone* root=0, float weight=1);
		void animate(float frame, const Bone* root=0, float weight=1);
		bool animateBone(Bone* bone, const Animation* anim, float frame, float weight=1);
		bool animateBone(Bone* bone, float frame, float weight=1);

		private:
		Bone*            m_bones;	// Array of bones
		int              m_count;	// Number of bones
		const Animation* m_last;	// Last animation used
		uint16*          m_hints;	// Animation hints
		ubyte*           m_flags;	// Flag array to save reallocation
	};


	// Inline functions
	inline Bone::Mode  Bone::getMode() const    { return m_mode; }
	inline void        Bone::setMode(Mode m)    { m_mode = m; }
	inline int         Bone::getIndex() const   { return m_index; }
	inline const char* Bone::getName() const    { return m_name; }
	inline float       Bone::getLength() const  { return m_length; }
	inline void        Bone::setLength(float l) { m_length = l; }
	inline Bone*       Bone::getParent()        { return m_parent<0? 0: m_skeleton->getBone(m_parent); }

	inline const Quaternion& Bone::getAngle() const                  { return m_angle; }
	inline const vec3&       Bone::getPosition() const               { return m_position; }
	inline const vec3&       Bone::getScale() const                  { return m_scale; }
	inline const Matrix&     Bone::getTransformation() const         { return m_local; }
	inline const Matrix&     Bone::getAbsoluteTransformation() const { return m_combined; }

	// Skeleton Inlines
	inline Bone*       Skeleton::getBone(int index)                        { return &m_bones[index]; }
	inline const Bone* Skeleton::getBone(int index) const                  { return &m_bones[index]; }
	inline int         Skeleton::getBoneCount() const                      { return m_count; }
	inline void        Skeleton::animate(float f, const Bone* r, float w)  { animate(m_last,f,r,w); }
	inline bool        Skeleton::animateBone(Bone* b, float f, float w)    { return animateBone(b,m_last,f,w); }


};
};


#endif
