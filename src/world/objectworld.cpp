#include <base/world/object.h>
#include <base/world/objectworld.h>
#include <base/scene.h>

using namespace base;
using script::Variable;

ObjectWorld::ObjectWorld(base::SceneNode* scene, const Variable& data) : m_sceneNode(scene), m_data(data) {
}

void ObjectWorld::initialise(base::SceneNode* scene, const Variable& data) {
	assert(!m_sceneNode);
	m_sceneNode = scene;
	m_data = data;
}

void ObjectWorld::addObject(Object* o) {
	m_messages.push_back({Message::Add, o});
}

void ObjectWorld::removeObject(Object* o, bool del) {
	m_messages.push_back({del? Message::Delete: Message::Remove, o});
}

void ObjectWorld::setUpdate(Object* o, bool up) {
	m_messages.push_back({up? Message::StartUpdate: Message::StopUpdate, o});
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

void ObjectWorld::update(float time) {
	for(Object* o: m_update) o->update(time);
	processMessages();
}


void ObjectWorld::processMessages() {
	if(!m_sceneNode) return;
	for(MessageData& msg : m_messages) {
		Object* o = msg.object;
		switch(msg.type) {
		case Message::Add:
			if(o->m_world != this) {
				o->m_world = this;
				m_sceneNode->addChild(o);
				if(o->m_hasUpdate) m_update.push_back(o);
				m_objects.push_back(o);
				objectAdded(o);
				o->onAdded();
			}
			break;
		case Message::Remove:
		case Message::Delete:
			o->onRemoved();
			objectRemoved(o);
			o->m_world = nullptr;
			m_sceneNode->removeChild(o);
			removeFromList(m_objects, o);
			if(o->m_hasUpdate) removeFromList(m_update, o);
			if(msg.type == Message::Delete) delete o;
			break;
		case Message::StartUpdate:
			if(!o->m_hasUpdate && o->m_world == this) {
				m_update.push_back(o);
				o->m_hasUpdate = true;
			}
			break;
		case Message::StopUpdate:
			if(o->m_hasUpdate && o->m_world == this) {
				o->m_hasUpdate = false;
				removeFromList(m_update, o);
			}
			break;
		}
	}
	m_messages.clear();
}

TraceResult ObjectWorld::trace(uint64 mask, const Ray& ray, float radius, float limit) const {
	TraceResult result;
	vec3 normal;
	for(Object* o: m_objects) {
		if(mask & 1<<o->m_traceGroup && o->trace(ray, radius, limit, normal)) {
			result.hit = true;
			result.normal = normal;
			result.object = o;
			result.distance = limit;
		}
	}
	if(result) result.position = ray.point(result.distance);
	return result;
}

