#pragma once

#include <base/hardwarebuffer.h>
#include <base/math.h>

namespace base {

class Mesh;

class Batcher {
	public:
	void clear();
	size_t size() const { return m_items.size(); }
	bool empty() const { return m_items.empty(); }
	void addMesh(Mesh* mesh, const Matrix& transform, const vec4& custom=vec4());
	void addMesh(Mesh* mesh, const vec3& pos, const Quaternion& rot=Quaternion(), const vec4& custom=vec4());
	Mesh* build(Mesh* replaceExisting=0) const;
	VertexAttributes attributes;
	protected:
	struct BatchItem {
		Mesh* mesh;
		Matrix transform;
		vec4 custom;
	};
	std::vector<BatchItem> m_items;
};

}

