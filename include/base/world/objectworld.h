#pragma once

#include <base/vec.h>
#include <base/variable.h>
#include <vector>

namespace base {

class Object;
class Camera;
class SceneNode;
using ObjectList = std::vector<Object*>;

// World collision traces
struct TraceResult {
	bool hit = false;
	Object* object = nullptr;
	float distance;
	vec3 position;
	vec3 normal;
	inline operator bool() const { return hit; }
};


// Manages object updates
class ObjectWorld {
	public:
	ObjectWorld(SceneNode* node=nullptr, Camera* camera=nullptr, const script::Variable& data = script::Variable());
	virtual ~ObjectWorld() {}

	void initialise(SceneNode*, Camera* camera=nullptr, const script::Variable& data=script::Variable());
	void setCamera(Camera* camera) { m_camera = camera; }
	const Camera* getCamera() const { return m_camera; }
	SceneNode* getSceneNode() { return m_sceneNode; }
	script::Variable& getData() { return m_data; }
	const ObjectList& getObjects() const { return m_objects; }
	void addObject(Object*);
	void removeObject(Object*, bool destroy=true);
	void setUpdate(Object*, bool updates);
	void processMessages();

	virtual void update(float time);

	virtual TraceResult trace(uint64 mask, const Ray& ray, float radius=0, float limit=1e8f) const;
	TraceResult trace(const Ray& ray, float radius=0, float limit=1e8f) const { return trace(~0u, ray, radius, limit); }

	protected:
	virtual void objectAdded(Object*) {}
	virtual void objectRemoved(Object*) {}
	/// Additional traces for non-objects - for stuff like terrain
	virtual void traceCustom(uint64 mask, const Ray&, float radius, float limit, TraceResult& result) const {}

	protected:
	enum class Message { Add, Remove, Delete, StartUpdate, StopUpdate };
	struct MessageData { Message type; Object* object; };
	std::vector<MessageData> m_messages;
	SceneNode* m_sceneNode;
	Camera* m_camera;
	ObjectList m_objects;
	ObjectList m_update;
	script::Variable m_data;
};

}

