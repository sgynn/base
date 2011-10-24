/**
 *	Base include for models library
 *	
 *	Design: 
 *	- Loader
 *	- Mesh (or Model with MeshBuffers )
 *	- Animation data
 *	- Modifiers (skin, morph, skeleton)
 *	- Custom Joints
 *	- Renderer
 *	
 *	Morph modifier takes two input meshes and outputs a mesh.
 *	Skin modifier takes a mesh and a skeleton, outputs a mesh
 *	The skeleton needs animation data and the current frame.
 *	
 *	Skeletons and animations are allowed to come from seperate sources.
 *	Would like to be able to add meshed from different sources too.
 *	All joints, meshes and animations can be named.
 *	
 *	Skins will be stored by jointID in the Mesh object
 *	
 *	Meshes can be stored as buffer objects
 *
 *	Step 1: Load model
 *		// Creates a ModelX instance and calls load(). other loaders could be added factory style
 *		Model* sheep = Model::load( "sheep.x" );
 *		//Creates Meshes, Skeletons and Animations and animations and adds them to the model.
 *		
 *	Step 2: Build model
 *		// This step can be skipped as the loader will create a fully built model.
 *		// This is for manually putting model parts together. Stuff like adding morphs or extra animations, and hiding meshes.
 *		sheep->addMorph( sheep->getMesh("Head1"), sheep->getMesh("Head2") ); //sources dont have to come from the same model.
 *		sheep->addSkin( sheep->getMesh(0) ); //add a skinned mesh.
 *		sheep->setSkeleton(...); //Can also change the skeleton. Multiple skeletons??
 *		//Can set a mesh to not be drawn (for say a collision mesh)
 *		sheep->setMeshFlag("Collision", MESH_META); //can also give mesh index
 *		
 *	
 *	Step 3: Update Model
 *		// Update any morphs, animate skeletons, skin meshes ...
 *		model->update(); //this should work for the automatic thingy
 *
 *
 *
 *	Step 4: Render Model
 *		// draw only MESH_OUTPUT meshes in the model
 *		sheep->draw(); //should work
 *	
 *	
 *	Adding a skinned mesh, or morphed mesh to a model marks its sources as MESH_INPUT
 *	and doesnt draw them in draw(). Also getMesh() will have a paremeter MESH_INPUT, MESH_OUTPUT, MESH_ALL.
 *	Need to be able to get the list of morphs to change the interpolation value.
 *	
 *	Oh well - i spent a whole week on the flipping of the model in XLoader. I am going to give up.
 *	Perhaps tackle this problem elsewhere: by writing my own DirectX Exporter for Lightwave.
 */

#ifndef _MODELS_
#define _MODELS_

//need some material class.
#include "material.h"

#include "hashmap.h"
namespace base {
namespace model {

	typedef struct { float x,y,z; } vec3;
	typedef struct { float w,x,y,z; } quaternion;
	typedef struct { float m[16]; } Matrix;
	
	
	class Model;
	/** Static class for storing model loaders */
	class Loader {
		public:
		static void add( Model*(*loader)(const char*) );
		static void load(const char* file);
	};
	
	/** Mesh vertex formats */
	enum VertexFormat {
		VERTEX_SIMPLE,	///< format: position:3
		VERTEX_TEXTURE, ///< format: position:3, texture:2
		VERTEX_DEFAULT,	///< format: position:3, normal:3, texture:2
		VERTEX_TANGENT,	///< format: position:3, normal:3, texture:2, tangent:3
	};
	
	/** A data buffer for containing vertex or index data. Can be referenced from multiple places, and convetred to vertex buffer objects */
	template <typename T>
	struct Buffer {
		Buffer() : data(0), size(0), stride(0), bufferObject(0), referenceCount(0) {};
		typedef T type;			//expose data type
		T* data;			//The data
		unsigned int size;		//number of elements in the array
		unsigned int stride;		//Data stride in bytes (cached)
		unsigned int bufferObject;	//Buffer object 0=not used
		int referenceCount;		//reference count
	};
	
	/** Meshes hold vertex data. A mesh can only have one material. Vertex format?  what about index arrays?*/
	class Mesh {
		public:
		/** Default constructor */
		Mesh();
		/** Copy constructor. @param copyVertices Vertex array is copied rather than referenced. @param referenceSkin can discard the skin */
		Mesh(const Mesh&, bool copyVertices=false, bool referenceSkin=true);
		/** Destructor. */
		~Mesh();
		
		/** Reference counting */
		int grab();
		int drop();
		
		/** Create the vertex buffer object */
		bool createBufferObject();
		/** Set this mesh to use vertex buffer objects */
		void useBufferObject(bool use=true);
		/** copy mesh data to the vertex buffer object */
		void updateBufferObject();
		/** Delete the buffer object */
		void deleteBufferObject();
		
		/** get the number of vertices in the mesh */
		unsigned int getVertexCount() const { return m_vertexBuffer->size/m_formatSize; }
		/** get the size of the mesh. If index buffer is used, this may be > vertexCount */
		unsigned int getSize() const { return m_indexBuffer? m_indexBuffer->size: m_vertexBuffer->size/m_formatSize; }
		/** get the mesh format */
		VertexFormat getFormat() const { return m_format; }
		/** Change the format of the mesh? */
		void changeFormat(VertexFormat format);
		/** Change the drawing mode */
		void setMode(unsigned int mode) { m_drawMode = mode; }
		/** Get the OpenGL Drawing mode. Default GL_TRIANGLES */
		unsigned int getMode() const { return m_drawMode; }
		/** Does this mesh use an index array? */
		bool hasIndices() const { return m_indexBuffer!=0; }
		
		/**Functions for rendering and data access */
		Material& getMaterial() { return m_material; }
		int bindBuffer() const;
		int getStride() const		{ return m_formatSize * sizeof(float); }
		const float* getVertexPointer() const 	{ return m_vertexBuffer->bufferObject? 0: m_vertexBuffer->data; }
		const float* getNormalPointer() const 	{ return s_offset[m_format][0] + (s_offset[m_format][0] && !m_vertexBuffer->bufferObject? m_vertexBuffer->data: 0); }
		const float* getTexCoordPointer() const	{ return s_offset[m_format][1] + (s_offset[m_format][1] && !m_vertexBuffer->bufferObject? m_vertexBuffer->data: 0); }
		const float* getTangentPointer() const 	{ return s_offset[m_format][2] + (s_offset[m_format][2] && !m_vertexBuffer->bufferObject? m_vertexBuffer->data: 0); }
		const unsigned short* getIndexPointer() const { return m_indexBuffer->bufferObject? 0: m_indexBuffer->data; }
		
		/** get data buffers */
		Buffer<float>* getVertexBuffer() { return m_vertexBuffer; }
		Buffer<const unsigned short>* getIndexBuffer() { return m_indexBuffer; }
		
		/** Get the position of a vertex */
		const float* getVertex(int index) const { return m_vertexBuffer->data + m_formatSize*index; }
		
		/** The weights used for skinning */
		struct Skin { int joint; const char* jointName; int size; unsigned short* indices; float* weights; float offset[16]; };
		const Skin* getSkin(int index) const { return m_skinBuffer->data[index]; }
		int getSkinCount() const { return m_skinBuffer->size; }
		
		
		/** Data Set Functions */
		void setVertices(int count, float* data, VertexFormat format=VERTEX_DEFAULT);
		void setVertices(Buffer<float>* buffer, VertexFormat format=VERTEX_DEFAULT);
		void setIndices(int count, const unsigned short* data);
		void setIndices(Buffer<const unsigned short>* buffer);
		void addSkin(Skin* skin);
		void setMaterial(const Material& material) { m_material=material; }
		
		/** Data Processing */
		int calculateTangents();
		int normaliseWeights();

		/** Bounding box */
		void updateBox();
		const float* boxMin() const { return m_box; }
		const float* boxMax() const { return m_box+3; }
		
		protected:
		static size_t s_offset[4][4]; //Normal, Texture, Tagnent index offsets for each vertex type
		VertexFormat m_format;
		int m_formatSize;
		unsigned int m_drawMode;
		
		Buffer<float>* m_vertexBuffer;
		Buffer<const unsigned short>* m_indexBuffer;
		Buffer<Skin*>* m_skinBuffer;

		float m_box[6]; //Bounding box;

		/** Calculate tangent for a polygon. Assumes vertex has at least {position,normal,texcoord} */
		int tangent(const float* a, const float* b, const float* c, float* t);

		Material m_material;
		
		/** Reference counting for models */
		int m_referenceCount;
	};
	
	/** Joint animation modes */
	enum JointMode {
		JOINT_DEFAULT,	///< Joint will get data from current animation, unless a parent joint is TRUNCATE
		JOINT_DISABLED,	///< Joint will not change
		JOINT_ANIMATED,	///< Joint will get data from the current animation. Overrides TRUNCATE
		JOINT_OVERRIDE,	///< User sets joint transformations
		JOINT_TRUNCATE,	///< Same as DISABLED, but disables all child joints too. Good for partial animations.
	};	
	/** Joints contain current position/rotation. */
	class Joint {
		friend class Skeleton;
		public:
		/** Constructor */
		Joint();
		Joint(const Joint&); /** Get the current mode of this joint */
		JointMode getMode() const { return m_mode; }
		/** Set the joint mode */
		void setMode(JointMode mode=JOINT_DEFAULT) { m_mode=mode; }
		/** Get joint id */
		int getID() const { return m_id; }
		/** get relative position */
		const float* getPosition() const { return m_position; }
		/** get relative rotation as quaternion */
		const float* getRotation() const { return m_rotation; }
		/** Get relative scale */
		const float* getScale() const { return m_scale; }
		/** Set position (only works if jointmode=OVERRIDE?) */
		void setPosition(const float* pos);
		/** Set rotation (only works if jointmode=OVERRIDE?) */
		void setRotation(const float* rot);
		/** Set scale (only works if jointmode=OVERRIDE?) */
		void setScale(const float* scale);
		/** Get the absolute transformation of this joint in world coordinates */
		const float* getTransformation() const { return m_combined; }
		/** Get the absolute transformation of this joint in world coordinates */
		void getTransformation(float* matrix) const;
		/** get the length of the joint (used for drawing and IK) */
		float getLength() const { return m_length; }
		/** Set the joint length */
		void setLength(float l) { m_length=l; }
		
		private:
		int m_id;		//Joint index
		int m_parent;		//Joint parent index
		JointMode m_mode;	//Animation mode
		float m_local[16];	//local transformation cached
		float m_combined[16];	//combined transformation. absolute in model space
		float m_rotation[4];	//rotation quaternion
		float m_position[3];	//translation vector
		float m_scale[3];	//scale vector
		float m_length;		//bone length (optional)
	};
	
	/** Holds animation keyframe data */
	class Animation {
		public:
		Animation();
		~Animation();
		
		void addKeyPosition(int frame, int joint, const vec3& position);
		void addKeyRotation(int frame, int joint, const quaternion& position);
		void addKeyScale(int frame, int joint, const vec3& scale);
		
		int getPosition(float*pos, int joint, float frame, unsigned int& hint) const;
		int getRotation(float*rot, int joint, float frame, unsigned int& hint) const;
		int getScale   (float*scl, int joint, float frame, unsigned int& hint) const;
		
		int getRange(int& start, int& end);
		
		struct KeyFrame3 { int frame; float data[3]; };
		struct KeyFrame4 { int frame; float data[4]; };
		struct JointAnimation {
			KeyFrame3* positionKeys;
			KeyFrame3* scaleKeys;
			KeyFrame4* rotationKeys;
			int positionCount;
			int scaleCount;
			int rotationCount;
		};
		/** Get the JointAnimation for a bone. Adds an empty one if it does not exist */
		JointAnimation& getJointAnimation(int jointID);
		
		private:
		//I still want to store animations in an integer indexed array.
		JointAnimation* m_animations;
		int m_count;
		int m_start, m_end; //Keyframe range
		
		int m_ref; //reference counting
	};
	
	/** Holds skeleton data. This includes joint heirachy, names, and initial matrices */
	class Skeleton {
		public:
		Skeleton();
		Skeleton(const Skeleton&);
		~Skeleton();
		
		/** Add a joint to the skeleton \returns added joint id */
		int addJoint(int parent, const char* name=0, const float* defaultMatrix=0);
		/** Set the default matrix of the joint. This is its position with no animation */
		void setDefaultMatrix(int joint, const float* matrix);
		/** Get a joint by ID */
		Joint* getJoint(int id);
		/** Get a joint by ID */
		const Joint* getJoint(int id) const;
		/** get a joint by name */
		Joint* getJoint(const char* name);
		/** get a joint by name */
		const Joint* getJoint(const char* name) const;
		/** get the number of joints in the skeleton */
		int getJointCount() const { return m_jointCount; }
		/** Get the parent joint of a joint */
		Joint* getParent(const Joint* joint);
		
		/** Calculates the joint positions a frame of an animation. Can animate from a certan joint */
		int animateJoints(const Animation* animation, float frame, int joint=0, float weight=1);
		/** Calcluate joint positions. Uses the last Animation */
		int animateJoints(float frame, int joint=0, float weight=1);
		/** Reset all joints to their default transformations */
		void reset();
		/** Reset a joint to its default transformation */
		void reset(int jointID);
		
		private:
		int animateJoint(int joint, float frame, unsigned char* flags, float weight);
		Joint* m_joints;
		Matrix* m_matrices;
		int m_jointCount;
		HashMap<unsigned int> m_names;
		
		//Caching of animation stuff
		const Animation* m_lastAnimation;
		unsigned int* m_animationHints;
	};
	
	/** The model class holds everything together. Mainly interfaces for the other classes */
	class Model {
		public:
		enum MeshFlag {
			MESH_OUTPUT,	//Normal mesh - will be drawn
			MESH_INPUT,	//These meshes are used as sources for the skinMesh and morphMesh functions
			MESH_META,	//Meshes for other purposes, ie collision, or navigation.
		};
		
		Model();
		Model(const Model& copy);
		~Model();
		
		/** Get the total number of meshes in this model. This includes ones that are not drawn */
		int getMeshCount() const { return m_meshCount; }
		/** Get a mesh by its index */
		Mesh* getMesh(int index);
		/** Get a mesh by name */
		Mesh* getMesh(const char* name);
		
		/** Get the MeshFlag for a mesh in this model */
		MeshFlag getMeshFlag(const Mesh* mesh) const;
		/** Set the flag for a mesh in this model */
		void setMeshFlag(const Mesh* mesh, MeshFlag flag);
		
		
		/** Add a mesh to the model */
		Mesh* addMesh(Mesh* mesh, const char* name, int joint);
		Mesh* addMesh(Mesh* mesh, int joint=0) { return addMesh(mesh, "", joint); }
		Mesh* addMesh(Mesh* mesh, const char* joint);
		
		/** Add a skeleton to the model. A model can only have one skeleton. */
		void setSkeleton(Skeleton* skeleton) { m_skeleton = skeleton; }
		/** Get the skeleton of this model. Needed for custom animations and attachments */
		Skeleton* getSkeleton() { return m_skeleton; }
		
		
		/** Add a morphed mesh to the model */
		Mesh* addMorphedMesh(Mesh* start, Mesh* end, float value, int joint=0);
		/** Add a skinned mesh to the model */
		Mesh* addSkinnedMesh(Mesh* source);
		
		/** Add an animation to the model */
		void addAnimation(const char* name, Animation* animation);
		/** Get an animation by name. Returns the first animation found if name not specified */
		Animation* getAnimation(const char* name=0);
		
		//Automation
		/** Draw all output meshes */
		void draw();
		/** Update any animations and reskin if necessary */
		void update(float time);
		/** Set the speed of the animation */
		void setAnimationSpeed(float fps);
		/** Set the current animation used by the update() function */
		void setAnimation(const char* name, bool loop=true);
		/** Set the current animation used by the update() function */
		void setAnimation(float start, float end, bool loop=true);
		/** Set the current animation used by the update() function */
		void setAnimation(const char* name, float start, float end, bool loop=true);
		/** Set the current frame. This must be within startFrame and endFrame set by setAnimation() */
		void setFrame(float frame);
		/** Get the current frame of the animation */
		float getFrame() const { return m_aFrame; }
		
		/** Load a model from file */
		static Model* loadModel(const char* name);
		
		/** Gets a linearly interpolated mesh between start and end. MorphFlags: vertices, texture coords */
		static Mesh* morphMesh(Mesh* out, const Mesh* start, const Mesh* end, float value, int morphFlags=1);
		/** Gets a skined mesh */
		static Mesh* skinMesh(Mesh* out, const Mesh* source, const Skeleton* skeleton);
		
		/** Draw a skeleton for debug purposes */
		static void drawSkeleton(const Skeleton* skeleton);
		/** Draw a mesh. */
		static void drawMesh(const Mesh* mesh);
		/** Draw a mesh. glFlags specify which client states are active for optimisation */
		static void drawMesh(const Mesh* mesh, int& glFlags);
		
		protected:
		struct MeshEntry { Mesh* mesh; Mesh* src; MeshFlag flag; const char* name; int joint; } *m_meshes;
		int m_meshCount;
		
		Skeleton* m_skeleton;
		
		HashMap<Animation*> m_animations;
		
		//Automation Data
		Animation* m_aAnimation;
		float m_aStart, m_aEnd;
		float m_aFrame, m_aSpeed;
		bool m_aLoop;
		
		
		
	};
	
};
};


#endif

