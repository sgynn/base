#ifndef _MODEL_LOADER_X_
#define _MODEL_LOADER_X_

#include <stdlib.h>
#include "model.h"

// This would probably be the 6th time ive written this //

namespace base {
namespace model {
	class XLoader {
		public:
		static Model* load(const char* filename);
		static void showDebug(bool on=true) { s_debug=on; }
		
		private:
		//internal material structure - nor really sure how to expose this
		struct XMaterial {
			float colour[4];
			float power;
			float specular[3];
			float emmision[3];
			char texture[32];
		};
		
		/** This iss the only part that relies on something external, so i will code it here to make it wasier to find if it needs changing */
		static void setMaterial(Mesh* mesh, const XMaterial& material) {
			float* diffuse = reinterpret_cast<float*>(&mesh->getMaterial().diffuse);
			float* ambient = reinterpret_cast<float*>(&mesh->getMaterial().ambient);
			for(int i=0; i<4; i++) {
				diffuse[i] = material.colour[i];
				if(i<3) ambient[i] = material.colour[i]*0.1f;
			}
			mesh->getMaterial().shininess = material.power;
			if(material.texture[0]) mesh->getMaterial().texture[0] = Texture::getTexture( material.texture );
		}
		
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
