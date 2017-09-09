#include "base/xmodel.h"
#include "base/parse.h"
#include "base/texture.h"
#include <cstring>
#include <cstdio>

using namespace base;
using namespace model;


#define Warning printf(__func__); printf
#define Error   printf("Error: %s ", __func__); printf
#define Info    if(0) printf

// Custom material loader function
SMaterial* (*XModel::s_matFunc)(SMaterial* in, const char* tex) = 0;
void XModel::setMaterialFunc( SMaterial* (*func)(SMaterial* in, const char* tex) ) {
	s_matFunc = func;
}

Model* XModel::load(const char* filename) {
	FILE* fp = fopen(filename, "r");
	if(fp) {
		// File length
		fseek(fp, 0, SEEK_END);
		int size = ftell(fp);
		rewind(fp);
		// Read file
		char* data = new char[size+1];
		size = fread(data, 1, size, fp);
		data[size] = 0; // eof
		fclose(fp);
		// Parse
		Model* mdl = parse(data);
		delete [] data;
		return mdl;
	}
	printf("Failed to load XModel %s\n", filename);
	return 0;
}

Model* XModel::parse(const char* string) {
	XModel xm;
	xm.m_data = xm.m_read = string;
	xm.m_line = 0;
	xm.m_model = new Model;
	if(xm.read()) return xm.m_model;
	else return 0;
}



//// //// //// //// //// //// //// //// //// //// //// //// //// //// //// ////
int XModel::parseSpace(const char* in) {
	int n = ::parseSpace(in);
	// Parse comments too
	if(in[n]=='/' && in[n+1]=='/') {
		while(in[n] && in[n]!='\n' && in[n]!='\r') ++n;
		n += ::parseSpace(in+n);
	}
	return n;
}

int XModel::parseIntArray(const char* in, int n, int* d) {
	int t=0;
	for(int i=0; i<n; ++i) {
		t += parseInt(in+t, d[i]);
		if(in[t]==',' || in[t]==';') t  += parseSpace(in+t+1)+1;
		//if(in[t]==',' && i<n-1) t += parseSpace(in+t+1)+1;
		//else if(in[t]==';' && i==n-1) ++t;
		else return 0;
	}
	return t;
}
int XModel::parseFloatArray(const char* in, int n, float* d) {
	int t=0;
	for(int i=0; i<n; ++i) {
		t += parseFloat(in+t, d[i]);
		if(in[t]==',' && i<n-1) t += parseSpace(in+t+1)+1;
		else if(in[t]==';' && i==n-1) ++t;
		else return 0;
	}
	return t;
}
int XModel::parseFloatKArray(const char* in, int n, int k, float* d) {
	int t=0;
	for(int i=0; i<n; ++i) {
		for(int j=0; j<k; ++j) {
			t += parseFloat(in+t, d[i*k+j]);
			if(!t || in[t]!=';') return 0;
			else t += parseSpace(in+t+1)+1;
		}
		if(in[t]==',' && i<n-1) t += parseSpace(in+t+1)+1;
		else if(in[t]==';' && i==n-1) ++t;
		else return 0;
	}
	return t;
}


bool XModel::startBlock(const char* block, char* name) {
	static char tmp[32];
	// Type
	int len = strlen(block);
	if(strncmp(block, m_read, len)!=0) return false;
	if(parseAlphaNumeric(m_read+len, tmp, 1)>0) return false;
	m_read += len;
	validate( parseSpace(m_read) );
	// Name
	if(*m_read!='{') {
		char* n = name? name: tmp;
		if(!validate( parseAlphaNumeric(m_read, n))) n[0] = 0;
		validate( parseSpace(m_read) );
	} else if(name) name[0] = 0;
	if(*m_read=='{') ++m_read;
	else return false;
	validate( parseSpace(m_read) );
	Info("Start block: %s %s\n", block, name?name:"");
	return true;
}
bool XModel::endBlock() {
	validate( parseSpace(m_read) );
	if(*m_read=='}') {
		++m_read;
		return true;
	} else {
		// Stuff to skip
		while(*m_read && *m_read!='}') {
			if(*m_read=='\n') ++m_line;
			else if(*m_read=='{') {
				++m_read;
				endBlock();
			}
			++m_read;
		}
		if(*m_read) ++m_read;
	}
	return false;
}

bool XModel::validate(int t) {
	for(int i=0; i<t; ++i) if(m_read[i]=='\n') ++m_line;
	m_read+=t;
	return t>0;
}


//// //// //// //// //// //// //// //// //// //// //// //// //// //// //// ////

XModel::XModel() {
}

int XModel::read() {
	// Header
	if(strncmp(m_read, "xof ", 4) !=0 ) { Error("Invalid header\n"); return 0; }
	if(strncmp(m_read+8, "txt 0032", 8) !=0 )  { Error("Invalid XModel type\n"); return 0; }
	m_read+=16;

	// Create skeleton
	m_skeleton = new Skeleton();
	m_model->setSkeleton( m_skeleton );

	// Main loop
	char name[32];
	while(*m_read) {
		validate( parseSpace(m_read) );
		if(startBlock("template")) endBlock();
		else if(startBlock("Frame", name)) {
			if(!readFrame(name, 0)) break;
			endBlock();
		}
		else if(startBlock("Mesh", name)) {
			XMesh* mesh = readMesh(name);
			if(!mesh) break;
			// Add meshes to model
			Mesh* m[32];
			int c = build(mesh, m);
			for(int i=0; i<c; ++i) {
				if(m[i]->getSkinCount()) m_model->addSkin(m[i]);
				else m_model->addMesh(m[i]);
			}
			endBlock();
		}
		else if(startBlock("AnimationSet", name)) {
			Animation* anim = readAnimation(name);
			if(!anim) break;
			m_model->addAnimation(anim);
			endBlock();
		}
		else if(*m_read==0) return finalise();
		else break;
	}

	delete m_model;
	return 0;
}

int XModel::finalise() {
	m_model->setSkeleton(m_skeleton); // Update skin indices
	return 1;
}

int XModel::readFrame(const char* name, Bone* parent) {
	Bone* bone = m_skeleton->addBone(parent, strdup(name));
	char tmp[32];
	// Frame loop
	while(*m_read) {
		validate( parseSpace(m_read) );
		if(startBlock("FrameTransformMatrix")) {
			Matrix m;
			int t = parseFloatArray(m_read, 16, m);
			if(!validate(t)) return 0;
			bone->setTransformation(m);
			if(parent && m[12]==0 && m[13]==0) parent->setLength(m[14]);
			endBlock();

		} else if(startBlock("Frame", tmp)) {
			if(!readFrame(tmp, bone)) return 0;
			endBlock();

		} else if(startBlock("Mesh", tmp)) {
			XMesh* mesh = readMesh(tmp);
			if(!mesh) { Error("Failed to load mesh\n"); break; }
			// Add meshes to frame
			Mesh* m[32];
			int c = build(mesh, m);
			for(int i=0; i<c; ++i) {
				if(m[i]->getSkinCount()) m_model->addSkin(m[i]);
				else m_model->addMesh(m[i], bone);
			}
			endBlock();
		} else if(*m_read=='}') return 1;
		else break;
	}
	Error("line %d\n", m_line);
	return 0;
}


int XModel::readCount() {
	int v;
	int t = parseInt(m_read, v);
	if(!t || m_read[t]!=';') return -1;
	validate( t + parseSpace(m_read+t+1)+1);
	return v;
}

int XModel::readReference(char* ref) {
	if(m_read[0] != '{') return 0;
	int t = parseSpace(m_read+1) + 1;
	int s = parseAlphaNumeric(m_read+t, ref);
	t += s + parseSpace(m_read+s+t);
	if(m_read[t]!='}') return 0;
	validate(t+1);
	return t+1;
}

XModel::XMesh* XModel::readMesh(const char* name) {
	// Initialise mesh
	XMesh* mesh = new XMesh;
	memset(mesh, 0, sizeof(XMesh));
	strcpy(mesh->name, name);
	int t, count;

	// Vertex Data
	count = mesh->vertexCount = readCount();
	if(count<=0) { destroyMesh(mesh); return 0; }
	mesh->vertices = new float[count*3];
	t = parseFloatKArray(m_read, count, 3, mesh->vertices);
	validate(t + parseSpace(m_read+t));

	// Faces
	mesh->size = count = readCount();
	if(count<=0) { destroyMesh(mesh); return 0; }
	mesh->faces = new XFace[count];
	t = readFaces(count, mesh->faces);

	Info("Mesh: %d vertices, %d faces\n", mesh->vertexCount, count);


	// Mesh loop
	while(*m_read) {
		validate( parseSpace(m_read) );
		if(startBlock("MeshNormals")) {						//// NORMALS ////
			// Normal values
			count = readCount();
			if(count<=0) break;
			mesh->normals = new float[count*3];
			t = parseFloatKArray(m_read, count, 3, mesh->normals);
			validate(t + parseSpace(m_read+t));

			// Normal face indices
			if(mesh->size!=readCount()) break;
			mesh->faceNormals = new XFace[mesh->size];
			t = readFaces(mesh->size, mesh->faceNormals);
			endBlock();

		} else if(startBlock("MeshTextureCoords")) {		//// TEXTURE COORDINATES ////
			count = readCount();
			if(count != mesh->vertexCount) break;
			mesh->texcoords = new float[count*2];
			t = parseFloatKArray(m_read, count, 2, mesh->texcoords);
			validate(t);
			endBlock();


		} else if(startBlock("VertexDuplicationIndices")) {
			endBlock();
		} else if(startBlock("MeshMaterialList")) {			//// MATERIAL LIST ////
			int mat = mesh->materialCount = readCount();
			mesh->materials = new SMaterial*[mat];

			count = readCount();
			if(count<=0) break;
			mesh->materialList = new int[count];
			t = parseIntArray(m_read, count, mesh->materialList);
			if(m_read[t]==';') ++t; // Blender puts an extra ';' here
			validate(t);

			// Read mat materials or references
			for(int i=0; i<mat; ++i) {
				validate( parseSpace(m_read) );
				if(startBlock("Material")) {
					mesh->materials[i] = readMaterial(0);
					endBlock();
				} else break;
			}

			if(!endBlock()) break;
			

		} else if(startBlock("XSkinMeshHeader")) {			//// SKIN HEADER ////
			readCount(); // Weights Per vertex
			readCount(); // Weights per face
			count = readCount(); // Bones
			mesh->skins = new XSkin[ count ];
			endBlock();

		} else if(startBlock("SkinWeights")) {				//// SKINS ////
			if(!mesh->skins) break; // Error - skin header not read
			XSkin& skin = mesh->skins[ mesh->skinCount++ ];

			// Read bone name
			t = parseString(m_read, skin.frame);
			if(m_read[t]==';') t += 1 + parseSpace(m_read+t+1);
			if(!validate(t)) break;

			// Read weight data
			skin.size = readCount();
			if(skin.size>0) {
				skin.indices = new int[skin.size];
				skin.weights = new float[skin.size];
				t  = parseIntArray(m_read, skin.size, skin.indices);
				t += parseSpace(m_read+t);
				t += parseFloatArray(m_read+t, skin.size, skin.weights);
				validate(t);

				// Skin offset matrix
				validate( parseSpace(m_read) );
				t = parseFloatArray(m_read, 16, skin.offset);
				validate(t);
			}

			endBlock();

		} else if(*m_read=='}') return mesh;
		else break;
	}
	Error("Mesh Error '%.20s'\n", m_read);
	destroyMesh(mesh);
	return 0;
}

int XModel::readFaces(int n, XFace* faces) {
	for(int i=0; i<n; ++i) {
		faces[i].n = readCount();
		if(faces[i].n<=0) return 0; // Fail
		faces[i].ix = new int[ faces[i].n ];
		int t = parseIntArray(m_read, faces[i].n, faces[i].ix);
		// Delimiter
		if(m_read[t]==',' && i<n-1) ++t += parseSpace(m_read+t);
		else if(m_read[t]==';' && i==n-1) ++t;
		else return 0;
		validate(t);
	}
	return n;
}

SMaterial* XModel::readMaterial(const char* name) {
	SMaterial* mat = new SMaterial;
	int t;
	// Diffuse
	t = parseFloatKArray(m_read, 1, 4, mat->diffuse);
	if(!validate(t+parseSpace(m_read+t))) { delete mat; return 0; }
	// Shininess
	t = parseFloatArray(m_read, 1, &mat->shininess);
	if(!validate(t+parseSpace(m_read+t))) { delete mat; return 0; }
	// Specular
	t = parseFloatKArray(m_read, 1, 3, mat->specular);
	if(!validate(t+parseSpace(m_read+t))) { delete mat; return 0; }
	// Emissive
	t = parseFloatKArray(m_read, 1, 3, mat->ambient);
	if(!validate(t+parseSpace(m_read+t))) { delete mat; return 0; }

	// Texture
	char file[64];
	if(startBlock("TextureFilename")) {
		t = parseString(m_read, file);
		endBlock();
	} else file[0] = 0;

	// Use custom function
	SMaterial* r = s_matFunc? s_matFunc(mat, file): mat;
	if(r!=mat) delete mat;
	return r;
}

void XModel::destroyMesh(XMesh* m) {
	// Failure - delete temporary mesh
	if(m->vertices)     delete [] m->vertices;
	if(m->normals)      delete [] m->normals;
	if(m->texcoords)    delete [] m->texcoords;
	if(m->materialList) delete [] m->materialList;
	if(m->materials)    delete [] m->materials;
	if(m->faces) {
		for(int i=0; i<m->size; ++i) delete [] m->faces[i].ix;
		delete [] m->faces;
	}
	if(m->faceNormals) {
		for(int i=0; i<m->size; ++i) delete [] m->faceNormals[i].ix;
		delete [] m->faceNormals;
	}
	if(m->skins) {
		for(int i=0; i<m->skinCount; ++i) {
			delete [] m->skins[i].indices;
			delete [] m->skins[i].weights;
		}
		delete [] m->skins;
	}
	delete m;
}


int XModel::build(XMesh* m, Mesh** out) {
	struct IMap { int to; bool used; };
	IMap* map = new IMap[m->vertexCount];
	int count = 0;
	bool noMat = m->materialCount==0;

	// one mesh per material (unless empty)
	for(int mi=0; mi<m->materialCount || (noMat&&mi==0); ++mi) {
		// Count vertices, indices and create index transformation map
		int vcount=0, icount=0;
		memset(map, 0, m->vertexCount*sizeof(IMap));
		for(int i=0; i<m->size; ++i) {
			if(noMat || m->materialList[i] == mi) {
				// Count vertices and add to map
				for(int j=0; j<m->faces[i].n; ++j) {
					int k = m->faces[i].ix[j];
					if(!map[k].used) {
						map[k].to = vcount;
						map[k].used = true;
						++vcount;
					}
				}
				// Count indices
				if(m->faces[i].n>3) icount += (m->faces[i].n-2)*3;
				else icount += 3;
			}
		}
		
		// Create mesh
		if(vcount>0) {
			// Copy vertex data
			int stride = 8; // use VERTEX_DEFAULT = VERTEX3|NORMAL|TEXCOORD
			VertexType* vx = new VertexType[ vcount * stride ]; 
			for(int i=0; i<m->vertexCount; ++i) {
				if(map[i].used) {
					memcpy(&vx[map[i].to*stride], &m->vertices[i*3], 3*sizeof(VertexType));
					if(m->normals)   memcpy(&vx[map[i].to*stride+3], &m->normals[i*3],   3*sizeof(VertexType));
					if(m->texcoords) memcpy(&vx[map[i].to*stride+6], &m->texcoords[i*2], 2*sizeof(VertexType));
				}
			}
			//FIXME: Normals dont necessarily have the same indices
			
			// Create indices - Need to split or merge any faces that have more or less than two vertices
			int k=0;
			IndexType*  ix = new IndexType[ icount ];
			for(int i=0; i<m->size; ++i) {
				if(noMat || m->materialList[i] == mi) {
					if(m->faces[i].n<3) {
						for(int j=0; j<m->faces[i].n; ++j) ix[k+j] = map[ m->faces[i].ix[j] ].to;
						for(int j=m->faces[i].n; j<3; ++j)  ix[k+j] = map[ m->faces[i].ix[0] ].to;
						k+=3;
					} else {
						for(int j=1; j<m->faces[i].n-1; ++j) {
							ix[k]   = map[ m->faces[i].ix[0]  ].to;
							ix[k+1] = map[ m->faces[i].ix[j]  ].to;
							ix[k+2] = map[ m->faces[i].ix[j+1]].to;
							k+=3;
						}
					}
				}
			}

			// Add to mesh
			Mesh* mesh = new Mesh;
			mesh->setVertices(vcount, vx, VERTEX_DEFAULT);
			mesh->setIndices(icount, ix);
			if(!noMat) mesh->setMaterial( m->materials[mi] );


			// Add skins
			for(int i=0; i<m->skinCount; ++i) {
				// get modified skin size
				int size = 0;
				for(int j=0; j<m->skins[i].size; ++j) if(map[ m->skins[i].indices[j]].used) ++size;
				// Create skin
				if(size) {
					Skin skin;
					skin.indices = new IndexType[size];
					skin.weights = new float[size];
					skin.bone = 0;
					skin.name = strdup(m->skins[i].frame);
					skin.offset = m->skins[i].offset;
					skin.size = size;
					k=0;
					for(int j=0; j<m->skins[i].size; ++j) {
						if(map[ m->skins[i].indices[j]].used) {
							skin.indices[k] = map[ m->skins[i].indices[j] ].to;
							skin.weights[k] = m->skins[i].weights[j];
							++k;
						}
					}
					mesh->addSkin(skin);
				}
			}
			// Add mesh to output list
			out[count] = mesh;
			Info("Mesh %s:%d added (%d/%d vertices)\n", m->name, count, vcount, m->vertexCount);
			++count;
		}
	}
	delete [] map;
	return count;
}

//// //// //// ///// //// //// //// ///// //// //// //// ///// //// //// //// /////

Animation* XModel::readAnimation(const char* name) {
	Animation* anim = new Animation;
	anim->setName(name);

	// Read bone animations
	char ref[32];
	float key[4];
	int type, count, f,c,t;
	int boneIndex;
	while(true) {
		validate( parseSpace(m_read) );
		if(*m_read=='}') return anim;
		else if(startBlock("Animation")) {
			// Frame reference
			if(!readReference(ref)) break;
			Bone* bone = m_skeleton->getBone(ref);
			if(!bone) { Error("No such bone '%s'\n", ref); break; }
			boneIndex = bone->getIndex();

			// AnimationKey
			while(*m_read && *m_read!='}') {
				validate(parseSpace(m_read));
				if(startBlock("AnimationKey")) {
					// Key type
					type = readCount();
					count = readCount();
					if(type<0 || count<=0) break;
					// Read keys
					for(int i=0; i<count; ++i) {
						f = readCount();
						c = readCount();
						t = parseFloatArray(m_read, c, key);
						validate(t+2); // HACK - should really validate ";," or ";;"
						validate(parseSpace(m_read));
						// Add key to animation
						switch(type) {
						case 0: anim->addRotationKey(boneIndex, f, Quaternion(key[0], -key[1], -key[2], -key[3])); break;
						case 1: anim->addScaleKey   (boneIndex, f, vec3(key)); break;
						case 2: anim->addPositionKey(boneIndex, f, vec3(key)); break;
						}
					}
					if(!endBlock()) break;
				}
			}

			endBlock();

		} else break;
	}
	Error("Invalid Animation\n");
	delete anim;
	return 0;
	
}

