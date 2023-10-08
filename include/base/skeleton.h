#pragma once

#include <base/math.h>

namespace base {
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
		const EulerAngles getEuler() const;		/**< Get the relative angle as Pitch,Yaw,Roll */
		const Quaternion& getAngle() const;		/**< Get the relative rotation as a quaternion */
		const Matrix&     getTransformation() const;			/**< Get the local transformation of the bone */
		const Matrix&     getAbsoluteTransformation() const;	/**< Get absolute transformation */

		void setPosition(const vec3& pos);		/**< Set the relative position */
		void setScale(float scale);				/**< Set the relative scale */
		void setScale(const vec3& scale);		/**< Set the relative scale */
		void setEuler(const vec3& pyr);			/**< Set the relative angle from pitch,yaw,roll */
		void setAngle(const Quaternion& q);		/**< Set the relative angle quaternion */
		void setTransformation(const Matrix&);	/**< Set the local transformation */
		void setAbsoluteTransformation(const Matrix&);	/**< Set the absolute transformation */
		void move(const vec3&);					/**< Move bone relative to its parent */
		void rotate(const Quaternion&);			/**< roatte bone relative to its parent */
		void updateLocal();						/**< Calculate outdated local variables */

		private:
		friend class Skeleton;

		Skeleton*   m_skeleton;	// Skeleton object this bone belongs to
		int         m_index;	// Skeleton index
		int         m_parent;	// Bone parent index
		const char* m_name;		// Bone name
		float       m_length;	// Rest length
		Mode        m_mode;		// Update behavious mode
		int         m_state;	// Which parts have been changed so need updating

		Matrix      m_local;	// Local matrix (cached from pos/scale/rot)
		Matrix      m_combined; // Derived matrix
		vec3        m_position;	// Bone local position
		vec3        m_scale;	// Bone local scale
		vec3        m_combinedScale; // Derived scale vector
		Quaternion  m_angle;	// Bone local orientation

		Bone();						/**< Private constructor - create from Skeleton */
		~Bone();
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

		Bone*       addBone(const Bone* parent, const char* name=0, const float* localMatrix=0, float length=1);	/**< Add a new bone to the skeleton */
		Bone*       getBone(int index);						/// Get bone by index
		const Bone* getBone(int index) const;				/// Get bone by index
		Bone*       getBone(const char *name) const;		///  Get bone by name
		int         getBoneIndex(const char* name) const;	/// Get index of named bone
		int         getBoneCount() const;					/// Get number of bones in skeleton
		void        setMode(Bone::Mode m);					/// Set all bone update modes
		void		setRestPose();							/// Set the rest pose as the current pose
		unsigned    getMapID() const;						/// Get skeleton ID for animation maps

		const Matrix& getRestPose(int bone) const;
		const Matrix& getSkinMatrix(int bone) const;
		const Quaternion& getRestAngle(int bone) const;

		bool update();								/**< Calculate bone matrices. Returns true if changed */

		void resetPose();
		void resetPose(int boneIndex);
		int  applyPose(const Animation*, float frame=0, int fromBone=-1, int mode=1, float weight=1, const char* keyMap=0);
		bool applyBonePose(Bone* bone, const Animation* anim, int set, float frame, int blend=1, float weight=1);

		const Matrix* getMatrixPtr() const;		/// Null if update not yet called
		Bone** begin() { return m_bones; }
		Bone** end() { return m_bones + m_count; }


		private:
		Bone**           m_bones;	// Array of bones
		int              m_count;	// Number of bones
		uint16*          m_hints;	// Animation hints
		ubyte*           m_flags;	// Flag array to save reallocation
		Matrix*          m_matrices;	// Final transform matrices for all bones (bone.combined * rest.skin)

		/* Skeleton rest data - can be shared */
		struct RestPose {
			int         ref;	// Reference count
			int         size;	// Number of bones
			Matrix*     local;	// Local bone matrixes for rest position
			Quaternion* rot;	// Local rest orientation
			vec3*       scale;	// Local rest scale
			Matrix*     skin;	// Skin matrices - inverse modelspace rest matrixes
		};
		RestPose* m_rest;
		void dropRestPose();
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
	inline Bone*       Skeleton::getBone(int index)                        { return m_bones[index]; }
	inline const Bone* Skeleton::getBone(int index) const                  { return m_bones[index]; }
	inline int         Skeleton::getBoneCount() const                      { return m_count; }

	inline const Matrix& Skeleton::getRestPose(int i) const { return m_rest->local[i]; }
	inline const Matrix& Skeleton::getSkinMatrix(int i) const { return m_rest->skin[i]; }
	inline const Quaternion& Skeleton::getRestAngle(int i) const { return m_rest->rot[i]; }

	inline const Matrix* Skeleton::getMatrixPtr() const { return m_matrices; }

}

