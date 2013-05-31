#ifndef _BASE_MODEL_X_
#define _BASE_MODEL_X_

#include "model.h"

namespace base {
namespace model {

	/** Version 7 of Direct X model format loader */
	class XModel {
		public:
		static Model* load(const char* filename);
		static Model* parse(const char* string);
		static bool   save(const Model* model, const char* filename);

		/** Set custom material function */
		static void setMaterialFunc( SMaterial* (*func)(SMaterial* in, const char* tex) );

		private:
		XModel();
		
		// Structures
		struct XSkin {
			char frame[32];
			Matrix offset;
			int size;
			int* indices;
			float* weights;
		};
		struct XFace {
			int n;
			int* ix;
		};
		// Full mesh structure as it may need a fair amount of conversion
		struct XMesh {
			char        name[32];		// Mesh name
			int         size;			// Face count
			int         vertexCount;	// Number of vertices
			float*      vertices;		// Vertices
			float*      normals;		// Normals
			float*      texcoords;		// Texture coordinates
			XFace*      faces;			// Mesh faces as indices
			XFace*      faceNormals;	// Normal indices
			int         materialCount;	// Number of materials
			int*        materialList;	// List of material indices
			SMaterial** materials;		// Material array
			int         skinCount;		// Number of skins
			XSkin*      skins;			// Skin data
		};
	

		// More parsing functions //
		static int parseIntArray(const char* in, int n, int* d);
		static int parseFloatArray(const char* in, int n, float* d);
		static int parseFloatKArray(const char* in, int n, int k, float* d);
		static int parseSpace(const char* in);


		// Structure functions
		int         read();										// Entry point read function
		int         readFrame(const char* name, Bone* parent);	// Read a frame object
		XMesh*      readMesh(const char* name);					// Read a mesh -> convert to library meshes
		XSkin       readSkin();									// Read a skin
		int         readFaces(int n, XFace* faces);				// Read face data
		SMaterial*  readMaterial(const char* name);				// Read a material
		Animation*  readAnimation(const char* name);			// Read an animation
		int         readReference(char* ref);					// Read a reference to frame,material,mesh
		int         readCount();								// Read integer for array size

		// Additional functions
		bool        startBlock(const char* block, char* name=0);	// Start block
		bool        endBlock();						// End a block
		bool        validate(int t);				// Called after parse functions, applies, checks errors and increments counters
		static void destroyMesh(XMesh*);			// XMesh destructor
		int         build(XMesh* mesh, Mesh** out);	// Build Meshes from XMesh
		int         finalise();						// FInalise model plus any cleanup


		// Data
		const char* m_data;
		const char* m_read;
		int         m_line;
		Model*      m_model;
		Skeleton*   m_skeleton;

		static SMaterial* (*s_matFunc)(SMaterial* in, const char* tex); // Material function
		
	};

};
};


#endif

