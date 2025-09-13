#include <base/world/object.h>
#include <base/world/objectworld.h>
#include <base/scene.h>
#include <base/assert.h>

using namespace base;
using namespace world;

ObjectWorldBase::ObjectWorldBase(SceneNode* scene, Camera* camera) : m_sceneNode(scene), m_camera(camera) {
}

void ObjectWorldBase::initialise(base::SceneNode* scene, Camera* camera) {
	assert(!m_sceneNode);
	m_sceneNode = scene;
	m_camera = camera;
}

void ObjectWorldBase::addObject(WorldObjectBase* o) {
	m_messages.push_back({Message::Add, o});
}

void ObjectWorldBase::removeObject(WorldObjectBase* o, bool del) {
	m_messages.push_back({del? Message::Delete: Message::Remove, o});
}

template<class T> bool removeFromList(std::vector<T>& list, const T& item) {
	for(size_t i=0; i<list.size(); ++i) {
		if(list[i] == item) {
			list[i] = list.back();
			list.pop_back();
			return true;
		}
	}
	return false;
}

void ObjectWorldBase::update(float time) {
	processMessages();
	for(WorldObjectBase* o: m_update) o->update(time);
}


bool ObjectWorldBase::processMessages() {
	if(!m_sceneNode) return false;
	if(m_messages.empty()) return false;
	static auto isAncestor = [](const SceneNode* node, const SceneNode* ancestor) {
		for(SceneNode* n = node->getParent(); n; n=n->getParent()) if(n==ancestor) return true;
		return false;
	};
	for(size_t i=0; i<m_messages.size(); ++i) {
		MessageData msg = m_messages[i];
		WorldObjectBase* o = msg.object;
		switch(msg.type) {
		case Message::Add:
			if(o->m_world == this) break;
			// Add called before remove, process remove first
			if(o->m_world) {
				o->onRemoved();
				o->m_world->baseObjectRemoved(o);
				removeFromList(o->m_world->m_objects, o);
				if(o->m_hasUpdate) removeFromList(o->m_world->m_update, o);
			}
			o->m_world = this;
			if(!isAncestor(o, m_sceneNode)) m_sceneNode->addChild(o); // may already be attached
			if(o->m_hasUpdate) m_update.push_back(o);
			m_objects.push_back(o);
			baseObjectAdded(o);
			o->onAdded();
			break;
		case Message::Remove:
		case Message::Delete:
			assert(o->m_world == this || msg.type == Message::Remove); // Deleting object from wrong world
			if(o->m_world != this) break;
			o->onRemoved();
			baseObjectRemoved(o);
			o->m_world = nullptr;
			m_sceneNode->removeChild(o);
			removeFromList(m_objects, o);
			if(o->m_hasUpdate) removeFromList(m_update, o);
			if(msg.type == Message::Delete) delete o;
			break;
		case Message::StartUpdate:
			if(o->m_world != this) break;
			if(o->m_hasUpdate) break;
			m_update.push_back(o);
			o->m_hasUpdate = true;
			break;
		case Message::StopUpdate:
			if(o->m_world != this) break;
			o->m_hasUpdate = false;
			removeFromList(m_update, o);
			break;
		}
	}
	m_messages.clear();
	return true;
}


bool ObjectWorldBase::traceObject(const WorldObjectBase* o, TraceGroupMask mask, const Ray& ray, float radius, float limit, float& t, vec3& normal) {
	if(mask & (1<<o->m_traceGroup)) {
		if(ObjectTraceResult hit = o->trace(ray, radius, limit)) {
			t = hit.t;
			normal = hit.normal;
			return true;
		}
	}
	return false;
}

/*
TraceResult ObjectWorldBase::trace(uint64 mask, const Ray& ray, float radius, float limit) const {
	TraceResult result;
	traceCustom(mask, ray, radius, limit, result);
	if(result) limit = result.distance;

	vec3 normal;
	float distance = limit;
	for(Object* o: m_objects) {
		if(mask & 1<<o->m_traceGroup && o->trace(ray, radius, distance, normal) && distance < limit) {
			result.hit = true;
			result.normal = normal;
			result.object = o;
			result.distance = distance;
			limit = distance;
		}
	}
	if(result) result.position = ray.point(result.distance);
	return result;
}
*/

