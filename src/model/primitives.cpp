#include <base/genmesh.h>
#include <base/hardwarebuffer.h>
#include <base/mesh.h>
#include <base/vec.h>
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



}

