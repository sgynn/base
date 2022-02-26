#include <base/wavefront.h>
#include <base/hardwarebuffer.h>
#include <base/model.h>
#include <base/parse.h>
#include <cstdio>
#include <vector>
#include <map>

using namespace base;

Model* Wavefront::load(const char* file) {
	FILE* fp  = fopen(file, "r");
	if(fp) {
		fseek(fp, 0, SEEK_END);
		int size = ftell(fp);
		rewind(fp);
		// read data
		char* data = new char[size+1];
		size = fread(data, 1, size, fp);
		data[size] = 0;
		fclose(fp);
		// Parse
		Model* m = parse(data);
		delete [] data;
		return m;
	}
	printf("Failed to load Wavefront model %s\n", file);
	return 0;
}

Model* Wavefront::parse(const char* str) {
	std::vector<vec3> vx;
	std::vector<vec3> vn;
	std::vector<vec2> vt;
	std::vector<Point3> f;

	const char* s = str;
	int r=0, i=0;
	vec3 tmp;
	Point3 ix;
	Point3 size;
	char material[32], group[32];
	material[0] = group[0] = 0;

	Model* model = new Model;

	while(*s) {
		s += parseSpace(s);
		switch(*s) {
		case '#':	// Comment
			s += parseDelimiter(s);
			break;

		case 'v':	// Vertex data
			switch(s[1]) {
			case ' ': case '\t':	// Position
				r = 1;
				for(i=0; i<3; ++i) {
					r += parseSpace(s+r);
					r += parseFloat(s+r, tmp[i]);
				}
				if(r) vx.push_back(tmp);
				else return 0;
				break;
			case 'n':				// Normal
				r = 2;
				for(i=0; i<3; ++i) {
					r += parseSpace(s+r);
					r += parseFloat(s+r, tmp[i]);
				}
				if(r) vn.push_back(tmp);
				else return 0;
				break;
			case 't':				// Texture Coordinates
				r = 2;
				for(i=0; i<2; ++i) {
					r += parseSpace(s+r);
					r += parseFloat(s+r, tmp[i]);
				}
				if(r) vt.push_back(tmp.xy());
				else return 0;
				break;
			default:
				printf("OBJ error: %.10s...\n", s);
				return 0;
			}
			s += r;
			break;
		
		case 'f': // Read face
			++s;
			i = f.size();
			size = Point3(vx.size()+1, vt.size()+1, vn.size()+1);
			for(int j=0; true; ++j) {
				s += parseSpace(s);
				for(int k=0; k<8; ++k) {
					r = parseInt(s, ix[k]);
					if(r==0) ix[k] = 0;
					else s += r;
					if(ix[k]<0) ix[k] += size[k]; // negative values are relative
					if(*s!='/') break;
					++s;
				}
				if(ix[0]==0) break;
				// Add to face list and triangulate if nessesary
				if(j>2) {
					f.push_back(f[i]);
					f.push_back(f[i+1]);
				}
				f.push_back(ix);
			}
			break;
		
		case 'g':	// Groups
			++s;
			s += parseSpace(s);
			r += parseDelimiter(s, '\n', group, 32);
			for(char* c=group; *c; ++c) if(*c==' ' || *c=='\t') { *c=0; break; }
			s += r;

			// start a new mesh
			if(!f.empty()) {
				model->addMesh(0, buildMesh(f.size(), &f[0], &vx[0], &vt[0], &vn[0]), material);
				f.clear();
			}
			break;
		case 'u': // usemtl
			r = parseKeyword(s, "usemtl", true);
			if(r>0) {
				r += parseSpace(s+r);
				r += parseDelimiter(s+r, '\n', material, 32);
				for(char* c=material; *c; ++c) if(*c==' ' || *c=='\t') { *c=0; break; }
				s += r;
				// Start a new mesh
				if(!f.empty()) {
					model->addMesh(0, buildMesh(f.size(), &f[0], &vx[0], &vt[0], &vn[0]), material);
					f.clear();
				}
				break;
			}
		default:
			printf("Wavefront error: invalid line '%.10s'\n", s);
			s += parseDelimiter(s);
		}
	}

	// Add final mesh
	if(!f.empty()) model->addMesh(0, buildMesh(f.size(), &f[0], &vx[0], &vt[0], &vn[0]), material);

	// Something failed
	if(model->getMeshCount()==0) { delete model; return 0; }
	return model;
}


Mesh* Wavefront::buildMesh(int size, Point3* f, vec3* vx, vec2* tx, vec3* nx) {
	std::map<Point3, int> map;
	std::map<Point3, int>::iterator it;

	HardwareVertexBuffer* vertexBuffer = new HardwareVertexBuffer();
	HardwareIndexBuffer* indexBuffer = new HardwareIndexBuffer();
	
	// Index array
	uint16* ix = new uint16[size];
	for(int i=0; i<size; ++i) {
		it = map.find(f[i]);
		if(it==map.end()) {
			ix[i] = map.size();
			map[f[i]] = ix[i];
		} else ix[i] = it->second;
	}
	indexBuffer->setData(ix, size, true);

	// Vertex array
	vertexBuffer->attributes.add(VA_VERTEX, VA_FLOAT3);
	if(nx) vertexBuffer->attributes.add(VA_NORMAL, VA_FLOAT3);
	if(tx) vertexBuffer->attributes.add(VA_TEXCOORD, VA_FLOAT2);
	int stride = vertexBuffer->attributes.calculateStride() / 4;
	int on = 3, ot = (nx?6:3);
	float* v = new float[ stride * map.size() ];
	for(it=map.begin(); it!=map.end(); ++it) {
		const Point3& d = it->first;
		int k = it->second * stride;
		memcpy(v+k, vx+d.x-1,  3*sizeof(float));
		if(nx && d.z) memcpy(v+k+on, nx+d.z-1, 3*sizeof(float));
		if(tx && d.y) memcpy(v+k+ot, tx+d.y-1, 2*sizeof(float));
	}

	// Create mesh object
	Mesh* mesh = new Mesh;
	mesh->setVertexBuffer(vertexBuffer);
	mesh->setIndexBuffer(indexBuffer);
	return mesh;
}

static void writeMesh(FILE* fp, Mesh* mesh, int offset=0) {
	bool hasNormals = mesh->getVertexBuffer()->attributes.hasAttrribute(VA_NORMAL);
	bool hasTexCoords = mesh->getVertexBuffer()->attributes.hasAttrribute(VA_TEXCOORD);
	uint vertexCount = mesh->getVertexCount();

	// Vertices
	for(uint i=0; i<vertexCount; ++i) {
		const vec3& v = mesh->getVertex(i);
		fprintf(fp, "v %g %g %g\n", v.x, v.y, v.z);
	}
	// Normals
	if(hasNormals) {
		for(uint i=0; i<vertexCount; ++i) {
			const vec3& n = mesh->getNormal(i);
			fprintf(fp, "vn %g %g %g\n", n.x, n.y, n.z);
		}
	}
	//TexCoords
	if(hasTexCoords) {
		const Attribute& elem = mesh->getVertexBuffer()->attributes.get(VA_TEXCOORD, 0);
		for(uint i=0; i<vertexCount; ++i) {
			const vec2& t = mesh->getVertexBuffer()->getVertexData<vec2>(elem, i);
			fprintf(fp, "vt %g %g\n", t.x, t.y);
		}
	}
	// Faces
	uint size = mesh->getIndexCount();
	for(uint i=0; i<size; ++i) {
		if(i%3==0) fprintf(fp, "\nf ");
		uint16 ix = mesh->getIndexBuffer()->getIndex(i) + offset;
		fprintf(fp, "%d", ix);
		if(hasTexCoords) fprintf(fp,"/%d", ix);
		else if(hasNormals) fprintf(fp, "/");
		if(hasNormals) fprintf(fp, "/%d", ix);
	}
}


bool Wavefront::save(Model* model, const char* filename) {
	FILE* fp = fopen(filename, "w");
	if(!fp) return false;

	fprintf(fp, "# libbase OBJ file #\n");
	int offset = 1;
	for(int m=0; m<model->getMeshCount(); ++m) {
		Mesh* mesh = model->getMesh(m);
		if(model->getMeshCount()>1) fprintf(fp, "g mesh%d\n", m+1);
		writeMesh(fp, mesh, offset);
		offset += mesh->getVertexCount();
	}
	fclose(fp);
	return true;
}

bool Wavefront::save(Mesh* mesh, const char* filename) {
	FILE* fp = fopen(filename, "w");
	if(!fp) return false;
	fprintf(fp, "# libbase OBJ file #\n");
	writeMesh(fp, mesh, 1);
	fclose(fp);
	return true;
}

