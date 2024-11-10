#pragma once

#include <base/math.h>

namespace base {

class HardwareIndexBuffer;
class HardwareVertexBuffer;

typedef unsigned short IndexType;
typedef float VertexType;
enum class PolygonMode : char { TRIANGLES, QUADS, LINES, POINTS, TRIANGLE_STRIP, LINE_STRIP };

class Mesh {
	friend class Model;

	public:
	struct Morph {
		int size;
		char* name;
		IndexType* indices;
		vec3* vertices;
		vec3* normals;
	};

	public:
	Mesh();
	Mesh(const Mesh&);
	Mesh(Mesh&&);
	~Mesh();


	Mesh* addReference();
	const Mesh* addReference() const;
	int   dropReference();

	void setPolygonMode(PolygonMode mode);
	PolygonMode getPolygonMode() const;

	size_t getVertexCount() const;
	size_t getIndexCount() const;
	
	const vec3& getVertex(uint index) const;
	const vec3& getNormal(uint index) const;
	const vec3& getIndexedVertex(uint index) const;
	const vec3& getIndexedNormal(uint index) const;

	const BoundingBox& getBounds() const;
	const BoundingBox& calculateBounds();

	void initialiseSkinData(int count, int weightsPerVertex);
	void setSkinName(int index, const char* name);
	const char* getSkinName(int index) const;
	size_t getSkinCount() const;
	size_t getWeightsPerVertex() const;
	
	void setVertexBuffer(HardwareVertexBuffer*);
	void setIndexBuffer(HardwareIndexBuffer*);
	void setSkinBuffer(HardwareVertexBuffer*);
	void setMorphs(int count, Morph* morphs);

	HardwareIndexBuffer* getIndexBuffer();
	HardwareVertexBuffer* getVertexBuffer();
	HardwareVertexBuffer* getSkinBuffer();

	int getMorphIndex(const char* name) const;
	
	protected:
	int calculateNormals();
	int calculateTangents();
	int normaliseWeights();

	protected:
	mutable int m_ref;
	PolygonMode m_polygonMode;
	BoundingBox m_bounds;

	HardwareVertexBuffer* m_vertexBuffer;
	HardwareVertexBuffer* m_skinBuffer;
	HardwareIndexBuffer*  m_indexBuffer;
	// Cached offsets
	size_t m_vertexOffset;
	size_t m_normalOffset;
	size_t m_tangentOffset;

	// Skin info
	int m_skinCount;		// Number of skins
	int m_weightsPerVertex;	// Weights per vertex
	char** m_skinNames;		// Skin names to connect to bones

	int m_morphCount;
	Morph* m_morphs;
};

}

