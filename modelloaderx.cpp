#include "modelloaderx.h"
#include <string.h>
#include <cstdio>
#include "file.h"

using namespace base;
using namespace model;

Model* XLoader::load(const char* filename) {
	//search
	char path[64];
	if(File::find(filename, path)) filename = path;

	FILE* file = fopen(filename, "r");
	if(file) {
		//get file length
		fseek(file, 0, SEEK_END);
		int size = ftell(file);
		rewind(file);
		//get data
		char* buffer = new char[size+1];
		size = fread(buffer, 1, size, file);
		buffer[size] = 0; //end of file
		fclose (file);
		
		Model* mdl = XLoader(buffer).readModel();
		delete [] buffer;
		return mdl;
	}
	return 0;
}

/** ***************************** Main functions ***************************************** **/
#define Debug(A)  if(s_debug) std::cout << A << std::endl;

bool XLoader::s_debug = 0;
bool XLoader::s_flipZ = 1;

XLoader::XLoader( const char* data ) : m_data(data), m_read(data) {}
XLoader::~XLoader() {}
Model* XLoader::readModel() { 
	//Read header
	if(!keyword("xof ", false)) return 0;
	m_read += 4; //skip version number
	if(!keyword("txt 0032", false)) return 0;
	
	m_model = new Model();
	
	//Main loop
	char temp[32];
	const char* last = 0;
	while(last!=m_read) {
		last = m_read;
		
		comment();
		if(keyword("template") && !readTemplate()) break;
		else if(keyword("Frame") && !readFrame()) break;
		else if(keyword("Mesh") && !readMesh()) break;
		else if(keyword("AnimationSet") && !readAnimationSet()) break;
		else if(*m_read == 0) return m_model;  //Complete!
		else if(readName(temp)) {
			if(s_debug) {
				//get line
				const char* c=0;
				int line=0; for(c=m_data; c<m_read; c++) if(*c == '\n') line++;
				printf("XLoader::%s on line %d\n", temp, line);
			}
			break;
		}
	}
	
	Debug("XModel::Load Error");
	
	//Load error
	delete m_model;
	return 0;
}

int XLoader::readTemplate() {
	whiteSpace();
	char name[32] = "";
	readName(name);
	whiteSpace();
	Debug("XLoader::template " << name);
	if(!keyword("{", false)) return 0;
	while(*m_read!=0 && *(m_read-1)!='}') m_read++;
	return *m_read!=0;
}

int XLoader::readFrame(Skeleton* skeleton, int parent) {
	whiteSpace();
	char name[32] = "";
	readName(name);
	whiteSpace();
	if(!keyword("{", false)) return 0;
	
	//Create skeleton
	if(!skeleton) {
		skeleton = new Skeleton();
		m_model->setSkeleton(skeleton);
	}
	
	//Add joint - Note: This joint may already be referenced by a skin
	int joint = skeleton->addJoint(parent, name);
	Debug("XLoader::readFrame " << name << " [" << joint << "]");
	
	
	//Frame loop
	const char* last=0;
	while(last!=m_read) {
		last = m_read;
		
		comment();
		if(keyword("FrameTransformMatrix")) {
			float mat[16];
			if(!readFrameTransformationMatrix(mat)) return 0;
			skeleton->setDefaultMatrix(joint, mat);
		}
		else if(keyword("Frame") && !readFrame(skeleton, joint)) return 0;
		else if(keyword("Mesh") && !readMesh(joint)) return 0;
		else if(keyword("}", false)) break;
		else if(*m_read == 0) return 0;
	}
	
	return 1;
}

int XLoader::readFrameTransformationMatrix(float* mat) {
	whiteSpace();
	if(!keyword("{", false)) return 0;
	comment();
	for(int i=0; i<16; i++) {
		if(!readFloat(mat[i])) return 0;
		whiteSpace();
		if(!keyword(i<15?",":";", false)) return 0;
		comment();
	}
	if(s_flipZ) {
	}
	
	if(!keyword(";", false)) return 0;
	comment();
	return keyword("}", false);
}

int XLoader::readMesh(int joint) { 
	whiteSpace();
	char name[32] = "";
	readName(name);
	whiteSpace();
	if(!keyword("{", false)) return 0;
	comment();
	
	Debug("XLoader::readMesh " << name);
		
	int vertexCount=0;
	if(!readInt(vertexCount)) return 0;
	keyword(";", false);
	
	Debug(vertexCount << " vertices");
	
	//read vertices
	float* vertices = new float[ vertexCount*8 ];
	memset(vertices, 0, vertexCount*8*sizeof(float));
	for(int i=0; i<vertexCount; i++) {
		comment();
		readFloat(vertices[i*8+0]); keyword(";", false);
		readFloat(vertices[i*8+1]); keyword(";", false);
		readFloat(vertices[i*8+2]); keyword(";", false);
		//next
		keyword(i<vertexCount-1?",":";", false);
	}
	
	comment();
	int polyCount=0;
	readInt(polyCount);
	keyword(";", false);
	
	Debug(polyCount << " polygons");
	
	struct Polygon { int size; int* indices; };
	Polygon* polygons = new Polygon[ polyCount ];
	
	int materialCount=0;
	unsigned char* materialList = new unsigned char[ polyCount ];
	memset(materialList, 0, polyCount);
	XMaterial* materials;
	
	//Read polygons - need to triangulate later;
	int size=0;
	for(int i=0; i<polyCount; i++) {
		comment();
		readInt(size); keyword(";", false);
		polygons[i].size = size;
		polygons[i].indices = new int[size];
		for(int j=0; j<size; j++) {
			readInt(polygons[i].indices[j]); keyword(j<size-1?",":";", false);
		}
		keyword(i<polyCount-1?",":";", false);
	}
	
	//Skin stuff
	int skinCount=0, cSkin=0;
	Mesh::Skin* skins=0;
	
	//Mesh Loop
	bool fail = true; //We need to do some cleanup if something fails
	const char* last = 0;
	while(last!=m_read) {
		last = m_read;
		
		comment();
		if(keyword("MeshNormals")		&& !readNormals(vertices+3,8)) break;
		if(keyword("MeshTextureCoords")		&& !readTextureCoords(vertices+6,8)) break;
		if(keyword("VertexDuplicationIndices")	&& !readTemplate()) break;
		if(keyword("MeshMaterialList")		&& !readMaterialList(materialCount, materialList, materials)) break;
		
		if(keyword("XSkinMeshHeader")) {
			if(readSkinHeader(skinCount)) skins = new Mesh::Skin[ skinCount ];
			else break;
		}
		if(keyword("SkinWeights")		&& !readSkin(skins[cSkin++])) break;
		
		//Also skin stuff
		if(keyword("}", false)) { fail=false; break; }
		if(*m_read == 0) break;
	}
	
	/** ****** *********** Build Meshes ***************** ******************* *************************** ************************** */
	if(!fail) {
		
		struct Map { unsigned short to; bool used; };
		Map* map = new Map[vertexCount]; //map of loaded indices -> mesh indices
		for(int m=0; m==0 || m<materialCount; m++) {
			//clear map
			memset(map, 0, vertexCount*sizeof(Map));
			//count polygons and vertices. Also create map
			int vCount=0, pCount=0;
			for(int i=0; i<polyCount; i++) {
				if(materialList[i]==m) {
					pCount += polygons[i].size-2;
					for(int j=0; j<polygons[i].size; j++) {
						if( !map[ polygons[i].indices[j] ].used ) {
							map[ polygons[i].indices[j] ].used = true;
							map[ polygons[i].indices[j] ].to = vCount++;
						}
					}
				}
			}
			if(pCount>0) {
				//Then copy data across
				float* mVertices = new float[ vCount*8 ];
				unsigned short* mIndices = new unsigned short[ pCount*3 ];
				for(int i=0; i<vertexCount; i++) {
					if(map[i].used) memcpy(&mVertices[8*map[i].to], &vertices[i*8], 8*sizeof(float));
				}
				//Split any polygons with more than 3 points here
				int index=0;
				for(int i=0; i<polyCount; i++) {
					if(materialList[i]==m) {
						for(int k=1; k<polygons[i].size-1; k++) {
							mIndices[index++] = map[ polygons[i].indices[0] ].to;
							mIndices[index++] = map[ polygons[i].indices[k] ].to;
							mIndices[index++] = map[ polygons[i].indices[k+1] ].to;
						}
					}
				}
				//Add stuff to mesh
				Mesh* mesh = new Mesh();
				m_model->addMesh(mesh, strdup(name), joint); 
				mesh->setVertices(vCount, mVertices);
				mesh->setIndices(pCount*3, mIndices);

				//Set material
				if(materialCount>0) setMaterial(mesh, materials[m]);
				
				//Set skins
				for(int i=0; i<cSkin; i++) {
					//get skin size
					int skinSize=0;
					for(int j=0; j<skins[i].size; j++) if(map[skins[i].indices[j]].used) skinSize++;
					//copy skin
					if(skinSize>0) {
						Mesh::Skin* skin = new Mesh::Skin(skins[i]); //copy constructor should copy jointname and matrix.
						skin->indices = new unsigned short[skinSize];
						skin->weights = new float[skinSize];
						skin->size = skinSize;
						int index=0;
						for(int j=0; j<skins[i].size; j++) {
							if(map[skins[i].indices[j]].used) {
								skin->indices[index] = map[ skins[i].indices[j] ].to;
								skin->weights[index] = skins[i].weights[j];
								index++;
							}
						}
						Debug("Adding skin " << skin->jointName << " with " << skinSize << " weights");
						mesh->addSkin(skin);
					}
				}
				
				Debug("XLoader: Mesh added with " << vCount << " vertices and " << pCount << " polygons");
			}
		}
	}
	/** *********************** *********************** ************************ ****** ******************************************* *** */
	
	delete [] vertices;
	delete [] polygons;
	delete [] materialList;
	delete [] materials;
	return fail? 0: 1;
}

int XLoader::readNormals(float* vertices, size_t stride) {
	whiteSpace();
	if(!keyword("{", false)) return 0;
	comment();
	Debug("XLoader::readMeshNormals");
	
	int vertexCount=0;
	if(!readInt(vertexCount)) return 0;
	keyword(";", false);
	//read normals
	for(int i=0; i<vertexCount; i++) {
		comment();
		readFloat(vertices[i*stride+0]); keyword(";", false);
		readFloat(vertices[i*stride+1]); keyword(";", false);
		readFloat(vertices[i*stride+2]); keyword(";", false);
		//next
		keyword(i<vertexCount-1?",":";", false);
	}
	
	//skip the rest of this block
	while(*m_read!=0 && *(m_read-1)!='}') m_read++;
	return *m_read!=0;
}
int XLoader::readTextureCoords(float* vertices, size_t stride) {
	whiteSpace();
	if(!keyword("{", false)) return 0;
	comment();
	Debug("XLoader::readTextureCoords");
	
	int vertexCount=0;
	if(!readInt(vertexCount)) return 0;
	keyword(";", false);
	//read coordinates
	for(int i=0; i<vertexCount; i++) {
		comment();
		readFloat(vertices[i*stride+0]); keyword(";", false);
		readFloat(vertices[i*stride+1]); keyword(";", false);
		//next
		keyword(i<vertexCount-1?",":";", false);
	}
	//end block
	comment();
	return keyword("}", false);
}
int XLoader::readMaterialList(int& count, unsigned char* mat, XMaterial* &materials) {
	whiteSpace();
	if(!keyword("{", false)) return 0;
	comment();
	Debug("XLoader::readMaterialList");
	
	int polyCount=0;
	if(!readInt(count)) return 0;
	keyword(";", false);
	comment();
	if(!readInt(polyCount)) return 0;
	keyword(";", false);
	
	//Read material indices
	int index;
	for(int i=0; i<polyCount; i++) {
		comment();
		if(!readInt(index)) return 0;
		keyword(i<polyCount-1?",":";", false);
		mat[i] = index;
	}
	
	//Read materials
	materials = new XMaterial[ count ];
	for(int i=0; i<count; i++) {
		comment();
		//cant remember how material references work.
		if(keyword("Material") && !readMaterial(materials[i])) return 0;
		
	}
	
	//end block
	comment();
	return keyword("}", false);
	
}
int XLoader::readMaterial(XMaterial& material) {
	whiteSpace();
	char name[32] = "";
	readName(name);
	whiteSpace();
	if(!keyword("{", false)) return 0;
	comment();
	Debug("XLoader::readMaterial " << name);
	
	readFloat(material.colour[0]); keyword(";", false); whiteSpace();
	readFloat(material.colour[1]); keyword(";", false); whiteSpace();
	readFloat(material.colour[2]); keyword(";", false); whiteSpace();
	readFloat(material.colour[3]); keyword(";;", false);
	comment();
	readFloat(material.power); keyword(";", false);
	comment();
	readFloat(material.specular[0]); keyword(";", false); whiteSpace();
	readFloat(material.specular[1]); keyword(";", false); whiteSpace();
	readFloat(material.specular[2]); keyword(";;", false);
	comment();
	readFloat(material.emmision[0]); keyword(";", false); whiteSpace();
	readFloat(material.emmision[1]); keyword(";", false); whiteSpace();
	readFloat(material.emmision[2]); keyword(";;", false);
	comment();
	
	//texture
	material.texture[0]=0;
	if(keyword("TextureFilename") && !readTexture(material.texture)) return 0;
	
	//end block
	comment();
	return keyword("}", false);
}
int XLoader::readTexture(char* name) {
	whiteSpace();
	if(!keyword("{", false)) return 0;
	comment();
	readName(name);
	keyword(";", false);
	Debug("XLoader::readTexture " << name);
	comment();
	return keyword("}", false);
}


int XLoader::readSkinHeader(int& skinCount) {
	whiteSpace();
	if(!keyword("{", false)) return 0;
	comment();
	Debug("XLoader::readSkinHeader");
	int temp;
	readInt(temp); keyword(";", false); comment(); //Maximum Weights Per Vertex
	readInt(temp); keyword(";", false); comment(); //Maximum Weights Per Face
	readInt(skinCount); keyword(";", false); comment(); //Number of skins
	comment();
	return keyword("}", false);	
}
int XLoader::readSkin(Mesh::Skin& skin) {
	whiteSpace();
	if(!keyword("{", false)) return 0;
	comment();
	
	//Joint reference
	char name[32];
	readName(name); keyword(";", false); comment();
	Debug("XLoader::readSkin " << name);
	readInt(skin.size); keyword(";", false);
	
	//validate
	if(skin.size==0) return 0;
	skin.joint = -1;
	skin.jointName = strdup(name);
	
	//indices
	int index=0;
	skin.indices = new unsigned short[skin.size];
	for(int i=0; i<skin.size; i++) {
		comment();
		readInt( index ); skin.indices[i]=index;
		keyword(i<skin.size-1?",":";", false);
	}
	//weights
	skin.weights = new float[skin.size];
	for(int i=0; i<skin.size; i++) {
		comment();
		readFloat( skin.weights[i] );
		keyword(i<skin.size-1?",":";", false);
	}
	//skinOffsetMatrix
	for(int i=0; i<16; i++) {
		comment();
		readFloat( skin.offset[i] );
		keyword(i<15?",":";", false);
	}
	keyword(";", false);
	
	//end block
	comment();
	if(keyword("}", false)) {
		skin.jointName = strdup(name);
		skin.joint = -1;
		return 1;
	} else {
		delete [] skin.indices;
		delete [] skin.weights;
		return 0;
	}
}

/** ******************************************** Animations ********************************** ********************** **/

int XLoader::readAnimationSet() {
	whiteSpace();
	char name[32] = "";
	readName(name);
	whiteSpace();
	if(!keyword("{", false)) return 0;
	comment();
	
	Debug("XLoader::readAnimationSet " << name);
	
	Animation* animation = new Animation();
	
	//Animation loop
	while(keyword("Animation")) {
		if(!readAnimation(animation)) break;
		comment();
	}
	comment();
	
	//end of block - add animation to model
	if(keyword("}", false)) {
		m_model->addAnimation(name, animation);
		return 1;
	} else {
		delete animation;
		return 0;
	}
}
int XLoader::readAnimation(Animation* animation) {
	whiteSpace();
	char name[32] = "";
	readName(name);
	whiteSpace();
	if(!keyword("{", false)) return 0;
	comment();
	
	//read joint name
	char jointName[32] = "";
	if(!keyword("{", false)) return 0;
	whiteSpace();
	readName(jointName);
	whiteSpace();
	if(!keyword("}", false)) return 0;
	comment();
	
	//get Joint ID
	int joint = m_model->getSkeleton()->getJoint(jointName)->getID();
	Debug("XLoader::readAnimation " << jointName  << " [" << joint << "]");
	if(joint<0) return 0;
	
	//Add joint animation
	Animation::JointAnimation& anim = animation->getJointAnimation( joint );
	
	//contains a list of keys for this joint
	const char* tr = 0;
	while(tr!=m_read) {
		tr = m_read;
		if(keyword("AnimationKey") && !readAnimationKey(anim)) return 0;
		if(keyword("}", false)) return 1; //end of block
		comment();
	}
	return 0;
}
int XLoader::readAnimationKey(Animation::JointAnimation& anim) {
	whiteSpace();
	if(!keyword("{", false)) return 0;
	comment();
	int type=0, size=0;
	readInt(type); keyword(";", false); comment();
	readInt(size); keyword(";", false); comment();
	if(type>2) return 0; // Unsupported format
	
	Debug("XLoader::readAnimationKey " << type << " - " << size << " keyframes");
	
	int values=0;
	if(type==0) {
		Animation::KeyFrame4* data = new Animation::KeyFrame4[ size ];
		for(int i=0; i<size; i++) {
			readInt(data[i].frame); keyword(";", false); whiteSpace();
			readInt(values); keyword(";", false); whiteSpace();
			for(int j=0; j<values; j++) {
				readFloat(data[i].data[(j+3)%4]); keyword(j<values-1?",":";", false); whiteSpace();
			}
			data[i].data[3] *= -1;
			
			keyword(i<size-1?";,":";;", false);
			comment();
		}
		anim.rotationKeys = data;
		anim.rotationCount = size;
	} else {
		Animation::KeyFrame3* data = new Animation::KeyFrame3[ size ];
		for(int i=0; i<size; i++) {
			readInt(data[i].frame); keyword(";", false); whiteSpace();
			readInt(values); keyword(";", false); whiteSpace();
			for(int j=0; j<values; j++) {
				readFloat(data[i].data[j]); keyword(j<values-1?",":";", false); whiteSpace();
			}
			keyword(i<size-1?";,":";;", false);
			comment();
		}
		if(type==1) {
			anim.scaleKeys = data;
			anim.scaleCount = size;
		} else {
			anim.positionKeys = data;
			anim.positionCount = size;
		}
	}
	
	return keyword("}", false);
}

/** ***************************************** Parser functions ****************************************************** **/
int XLoader::whiteSpace() {
	const char* tr = m_read;
	while(*m_read == '\t' || *m_read == ' ' || *m_read==0x0d || *m_read==0x0a) m_read++;
	return m_read!=tr;
}
int XLoader::comment() {
	const char* tr = m_read;
	while(*m_read!=0) {
		if(keyword("//", false)) { //single line comments
			while(*m_read!='\n' && *m_read!=0) m_read++;
		} else if(keyword("/*", false)) { //block comments
			while(!keyword("*/", false) && *m_read!=0) m_read++;
		} else if(!whiteSpace()) break;
	}
	return m_read!=tr;
}
int XLoader::readInt(int& i) {
	const char* tr = m_read;
	bool neg = *tr == '-';
	if(neg) tr++;
	long r = 0;
	while(*tr>=0x30 && *tr<=0x39) {
		r = r * 10 + (*tr++ - 0x30);
	}
	if(tr>m_read + (neg?1:0)) {
		i = neg? -r: r;
		m_read = tr;
		return true;
	} else 	return false;
}
int XLoader::readFloat(float& f) {
	const char* tr = m_read;
	char temp[16];
	int i = 0;
	if(*tr == '-' || (*tr>=0x30 && *tr<=0x39)) {
		temp[i++] = *(tr++);
		while(*tr>=0x30 && *tr<=0x39) temp[i++] = *(tr++);	//integer part
		if(*tr == '.') temp[i++] = *(tr++); else return false;	//decimal point
		while(*tr>=0x30 && *tr<=0x39) temp[i++] = *(tr++);	//fraction part
		if(temp[i-1] == '.') return false; //can't end in a .	
		f = atof(temp);	//convert to float
		m_read = tr;
		return true;
	}
	return false;
}
int XLoader::keyword(const char* word, bool term) {
	const char* tr = m_read;
	for(int i=0; word[i]!=0; i++) {
		if(m_read[i] != word[i]) return false;
		tr++;
	}
	if(term && (*tr == '_' || (*tr>='A' && *tr<='Z') || (*tr>='a' && *tr<='z') || (*tr>='0' && *tr<='9'))) return false;
	m_read = tr;
	return true;
}
int XLoader::readName(char* name) {
	const char* tr = m_read;
	int r = 0;
	if(*tr=='"') {
		tr++;
		while(*tr!='"' && *tr) name[r++]=*tr++;
		name[r] = 0;
		if(*tr=='"') m_read = tr+1;
		return 1;
	} else {
		while((*tr == '_' || (*tr>='A' && *tr<='Z') || (*tr>='a' && *tr<='z') || (*tr>='0' && *tr<='9'))) name[r++]=*tr++;
		name[r]=0;
		m_read = tr;
		return r;
	}
}

