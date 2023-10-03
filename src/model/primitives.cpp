#include <base/primitives.h>
#include <base/hardwarebuffer.h>
#include <base/mesh.h>
#include <base/vec.h>
#include <unordered_map>

namespace base {

static Mesh* createMesh(int vsize, float* vx, int isize, uint16* ix, PolygonMode mode=PolygonMode::TRIANGLES) {
	HardwareVertexBuffer* vbuf = new HardwareVertexBuffer();
	vbuf->attributes.add(VA_VERTEX, VA_FLOAT3);
	vbuf->attributes.add(VA_NORMAL, VA_FLOAT3);
	vbuf->attributes.add(VA_TANGENT, VA_FLOAT3);
	vbuf->attributes.add(VA_TEXCOORD, VA_FLOAT2);
	vbuf->setData(vx, vsize, 11*sizeof(float), true);
	vbuf->createBuffer();

	HardwareIndexBuffer* ibuf = new HardwareIndexBuffer();
	ibuf->setData(ix, isize, true);
	ibuf->createBuffer();

	Mesh* mesh = new Mesh();
	mesh->setVertexBuffer(vbuf);
	mesh->setIndexBuffer(ibuf);
	mesh->calculateBounds();
	mesh->setPolygonMode(mode);
	return mesh;
}

inline void set(float* d, const vec3& v) { d[0]=v.x; d[1]=v.y; d[2]=v.z; };

// ----------------------------------------------------------------------------- //
Mesh* createPlane(const vec2& size) {
	vec2 s = size/2;
	float* vx = new float[44] {
		-s.x, 0, -s.y,   0,1,0,   1,0,0,  0,0,
		 s.x, 0, -s.y,   0,1,0,   1,0,0,  1,0,
		-s.x, 0,  s.y,   0,1,0,   1,0,0,  0,1,
		 s.x, 0,  s.y,   0,1,0,   1,0,0,  1,1,
	};
	uint16* ix = new uint16[6] { 0,2,1, 1,2,3 };
	return createMesh(4, vx, 6, ix);
}
// ----------------------------------------------------------------------------- //
Mesh* createPlane(const vec2& size, int divisions) {
	++divisions;
	vec2 a = -size / 2;
	vec2 step = size / divisions;
	float uvStep = 1.f / divisions;
	int vstride = divisions + 1;
	float* vx = new float[11 * vstride * vstride];
	for(int y=0; y<vstride; ++y) {
		for(int x=0; x<vstride; ++x) {
			float* v = vx + 11 * (x + y * vstride);
			v[0] = a.x + step.x * x;
			v[1] = 0;
			v[2] = a.y + step.y * y;
			v[3] = 0;
			v[4] = 1;
			v[5] = 0;
			v[6] = 1;
			v[7] = 0;
			v[8] = 0;
			v[9] = x * uvStep;
			v[10] = y * uvStep;
		}
	}
	uint16* ix = new uint16[divisions * divisions * 6];
	for(int y=0; y<divisions; ++y) {
		for(int x=0; x<divisions; ++x) {
			uint16* i = ix + 6*(x + y*divisions);
			uint16 n = x + y * vstride;
			i[0] = n;
			i[1] = n+vstride;
			i[2] = n+1;
			i[3] = n+1;
			i[4] = n+vstride;
			i[5] = n+vstride+1;
		}
	}
	return createMesh(vstride*vstride, vx, divisions*divisions*6, ix);
}
// ----------------------------------------------------------------------------- //
Mesh* createBox(const vec3& size) {
	vec3 s = size/2;
	float* vx = new float[24 * 11];
	auto addPlane = [s, vx](float*& v, vec3 t, vec3 b, vec3 n) {
		set(v, (t+b+n) * s);
		set(v+3, n);
		set(v+6, t);
		v[9]=1; v[10]=1;
		v += 11;
		
		set(v, (t-b+n) * s);
		set(v+3, n);
		set(v+6, t);
		v[9]=1; v[10]=0;
		v += 11;

		set(v, (-t+b+n) * s);
		set(v+3, n);
		set(v+6, t);
		v[9]=0; v[10]=1;
		v += 11;

		set(v, (-t-b+n) * s);
		set(v+3, n);
		set(v+6, t);
		v[9]=0; v[10]=0;
		v += 11;
	};
	float* v = vx;
	addPlane(v, vec3(1,0,0), vec3(0,-1,0), vec3(0,0,1));
	addPlane(v, vec3(-1,0,0), vec3(0,-1,0), vec3(0,0,-1));
	addPlane(v, vec3(1,0,0), vec3(0,0,1), vec3(0,1,0));
	addPlane(v, vec3(1,0,0), vec3(0,0,-1), vec3(0,-1,0));
	addPlane(v, vec3(0,0,-1), vec3(0,-1,0), vec3(1,0,0));
	addPlane(v, vec3(0,0,1), vec3(0,-1,0), vec3(-1,0,0));

	uint16* ix = new uint16[36] {
		0,1,2,2,1,3,
		4,5,6,6,5,7,
		8,9,10,10,9,11,
		12,13,14,14,13,15,
		16,17,18,18,17,19,
		20,21,22,22,21,23,
	};
	return createMesh(24, vx, 36, ix);
}
// ----------------------------------------------------------------------------- //
inline vec3* createRing(int seg) {
	float a = TWOPI / seg;
	vec3* ring = new vec3[seg+1];
	for(int i=0; i<seg; ++i) ring[i].set(sin(-i*a), 0, cos(-i*a));
	ring[seg] = ring[0]; // Avoid floating point error
	return ring;
}
Mesh* createSphere(float radius, int seg, int div) {
	return createCapsule(radius, 0, seg, div);
}
Mesh* createCapsule(float radius, float length, int seg, int div) {
	length /= 2;
	int s1=seg+1, d1=div+1;
	vec3* ring = createRing(seg);
	int vcount = s1 * d1 + (length>0? s1: 0);
	float* vx = new float[11 * vcount];
	float b = PI / div;
	int half = length>0? d1/2: -1;
	float* start = vx;
	for(int i=0; i<d1; ++i) {
		float r = sin(i*b);
		float y = cos(i*b);
		for(int j=0; j<=seg; ++j) {
			float* v = start + j * 11;
			vec3 p = ring[j] * r;
			p.y = y;

			set(v, p * radius);
			set(v+3, p);
			set(v+6, vec3(ring[j].z, 0, -ring[j].x));
			v[9] = 1 - (float)j / s1;
			v[10] = (float)i / div; // Fixme: uvs for capsule length
			v[1] += i<=half? length: -length;
		}
		if(i==half) half = --i;
		start += s1 * 11;
	}
	delete [] ring;

	// indices
	int size = -1;
	if(length>0) ++div;
	uint16* ix = new uint16[s1 * div * 2 + div*2];
	for(int i=0; i<div; ++i) {
		for(int j=0; j<s1; ++j) {
			ix[++size] = i*s1 + j + s1;
			ix[++size] = i*s1 + j;
		}
		ix[++size] = i * s1 + s1;
		ix[++size] = i * s1 + s1;
	}
	return createMesh(vcount, vx, size-1, ix, PolygonMode::TRIANGLE_STRIP);
}
// ----------------------------------------------------------------------------- //
Mesh* createCylinder(float radius, float length, int seg) {
	// Vertices
	length /= 2;
	vec3* ring = createRing(seg);
	int vcount = (seg + seg + 1) * 2;
	float* vx = new float[vcount * 11];
	// Walls
	for(int i=0; i<=seg; ++i) {
		float* v = vx + i * 2 * 11;
		set(v, ring[i] * radius);
		set(v+3, ring[i]);
		set(v+6, vec3(ring[i].z, 0, -ring[i].x));
		v[9] = 1 - (float)i / (seg+1);
		v[10] = 1;
		v[1] -= length;

		v += 11;
		memcpy(v, v-11, 11*sizeof(float));
		v[10] = 0;
		v[1] += 2 * length;
	}
	// Caps
	int vs = (seg+1) * 2;
	for(int i=0; i<seg; ++i) {
		float* v = vx + (vs + i) * 11;
		set(v, ring[seg-i] * radius);
		set(v+3, vec3(0,1,0));
		set(v+6, vec3(1,0,0));
		v[9] = ring[i].x * 0.5 + 0.5;
		v[10] = ring[i].z * 0.5 + 0.5;
		v[1] += length;

		v += seg * 11;
		set(v, ring[i] * radius); // opposite winding
		set(v+3, vec3(0,1,0));
		set(v+6, vec3(1,0,0));
		v[9] = ring[i].x * 0.5 + 0.5;
		v[10] = ring[i].z * 0.5 + 0.5;
		v[1] -= length;
	}
	// Indices
	uint16* ix = new uint16[(seg+seg+1)*2 + 4];
	for(int i=0; i<vs; ++i) ix[i] = i;
	// Cap 1
	int k = vs;
	ix[k++] = vs;
	for(int s=0, e=seg+1; s<e; --e) {
		ix[k++] = vs + s;
		if(++s<=e) ix[k++] = vs + e%seg;
	}
	// cap 2
	vs += seg;
	ix[k] = ix[k-1]; k++;
	ix[k++] = vs;
	for(int s=0, e=seg+1; s<e; --e) {
		ix[k++] = vs + s;
		if(++s<=e) ix[k++] = vs + e%seg;
	}

	delete [] ring;
	return createMesh(vcount, vx, k, ix, PolygonMode::TRIANGLE_STRIP);
}

// ----------------------------------------------------------------------------- //
Mesh* createIcoSphere(float radius, int divisions) {
	if(divisions > 6) divisions=6; // Division limit for 16bit indices
	std::vector<vec3> vertices;
	std::vector<uint16> indices;

	auto addVertex = [&](const vec3& p) {
		vec3 n = p.normalised();
		vertices.push_back(n*radius);
		vertices.push_back(n);
	};
	
	// Base icosahedron
	float t = (1.f + sqrt(5.f)) / 2.f;
	addVertex(vec3(-1,  t,  0));
	addVertex(vec3( 1,  t,  0));
	addVertex(vec3(-1, -t,  0));
	addVertex(vec3( 1, -t,  0));
	addVertex(vec3( 0, -1,  t));
	addVertex(vec3( 0,  1,  t));
	addVertex(vec3( 0, -1, -t));
	addVertex(vec3( 0,  1, -t));
	addVertex(vec3( t,  0, -1));
	addVertex(vec3( t,  0,  1));
	addVertex(vec3(-t,  0, -1));
	addVertex(vec3(-t,  0,  1));

	static const int cix[] = {  0,11,5, 0,5,1, 0,1,7, 0,7,10, 0,10,11,
								1,5,9, 5,11,4, 11,10,2, 10,7,6, 7,1,8,
								3,9,4, 3,4,2, 3,2,6, 3,6,8, 3,8,9,
								4,9,5, 2,4,11, 6,2,10, 8,6,7, 9,8,1 };

	std::unordered_map<uint32, uint16> midpoints;
	auto mid = [&](int a, int b)->uint16 {
		uint32 key = a<b? a|b<<16: b|a<<16;
		auto it = midpoints.find(key);
		if(it!=midpoints.end()) return it->second;
		addVertex((vertices[a*2] + vertices[b*2]) * 0.5f);
		midpoints[key] = vertices.size() / 2 - 1;
		return vertices.size() / 2 - 1;
	};
	
	indices.assign(cix, cix+20*3);
	for(int d=0; d<divisions; ++d) {
		std::vector<uint16> next;
		next.reserve(indices.size()*4);
		for(uint i=0; i<indices.size(); i+=3) {
			uint16 a = mid(indices[i+0], indices[i+1]);
			uint16 b = mid(indices[i+1], indices[i+2]);
			uint16 c = mid(indices[i+2], indices[i+0]);

			next.push_back(indices[i]);
			next.push_back(a);
			next.push_back(c);

			next.push_back(indices[i+1]);
			next.push_back(b);
			next.push_back(a);

			next.push_back(indices[i+2]);
			next.push_back(c);
			next.push_back(b);

			next.push_back(a);
			next.push_back(b);
			next.push_back(c);
		}
		indices.swap(next);
	}

	// ToDo: Figure out UVs

	// Construct mesh
	HardwareVertexBuffer* vbuffer = new HardwareVertexBuffer();
	vbuffer->copyData(&vertices[0], vertices.size()/2, 2*sizeof(vec3));
	vbuffer->attributes.add(VA_VERTEX, VA_FLOAT3);
	vbuffer->attributes.add(VA_NORMAL, VA_FLOAT3);

	HardwareIndexBuffer* ibuffer = new HardwareIndexBuffer(IndexSize::I16);
	ibuffer->copyData(&indices[0], indices.size());

	Mesh* mesh = new Mesh();
	mesh->setVertexBuffer(vbuffer);
	mesh->setIndexBuffer(ibuffer);
	return mesh;
}
}

