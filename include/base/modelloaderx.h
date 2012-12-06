#ifndef _BASE_MODEL_LOADER_X_
#define _BASE_MODEL_LOADER_X_

#include <stdlib.h>
#include "model.h"

// This would probably be the 6th time ive written this //

namespace base {
namespace model {
	class XLoader {
		public:
		/** Load model */
		static Model* load(const char* filename);

		/** Set debug mode */
		static void showDebug(bool on=true) { s_debug=on; }

		/** Set material set function */
		static void setMaterialFunction(void (*func)(SMaterial&, float*, float*, float*, float, const char*)) { s_matFunc=func; }
		
		private:
		// internal material structure
		struct XMaterial {
			float colour[4];
			float power;
			float specular[3];
			float emission[3];
			char texture[32];
		};
		
		/** Default set material function */
		static void (*s_matFunc)(SMaterial&, float*, float*, float*, float, const char*);

		
		/** Loader functions */
		XLoader(const char* data);
		~XLoader();
		Model* readModel();
		int readTemplate();
		int readFrame(Skeleton* s=0, int parent=0);
		int readFrameTransformationMatrix(float*);
		int readMesh(int joint=0);
		int readNormals(float* vertices, size_t stride);
		int readTextureCoords(float* vertices, size_t stride);
		int readMaterialList(int& count, unsigned char* mat, XMaterial* &materials);
		int readMaterial(XMaterial& mat);
		int readTexture(char* filename);
		int readSkinHeader(int& skinCount);
		int readSkin(Mesh::Skin& skin);
		
		int readAnimationSet();
		int readAnimation(Animation* animation);
		int readAnimationKey(Animation::JointAnimation& anim);
		
		/** Parser functions */
		int comment();
		int whiteSpace();
		int readInt(int& i);
		int readFloat(float& f);
		int keyword(const char* word, bool delim=true);
		int readName(char* name);
		
		//local data
		const char* m_data; //file data
		const char* m_read; //read pointer
		
		Model* m_model;	//model we are reading into
		
		static bool s_debug; //debug output?
		static bool s_flipZ; //Flip the Z axis?
	};
};
};

#endif
