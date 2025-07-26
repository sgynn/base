#pragma once

#include <base/vec.h>
#include <vector>

namespace base {
class Camera;
class SceneNode;

namespace world {
	class WorldObjectBase;
	template<class T> class ObjectWorld;

	// World collision traces
	template<class ObjectType>
	struct TraceResultT {
		bool hit = false;
		ObjectType* object = nullptr;
		float distance;
		vec3 position;
		vec3 normal;
		inline operator bool() const { return hit; }
	};


	// Manages object updates
	class ObjectWorldBase {
		public:
		using TraceGroupMask = unsigned long long;

		ObjectWorldBase(SceneNode* node=nullptr, Camera* camera=nullptr);
		virtual ~ObjectWorldBase() = default;
		void initialise(SceneNode*, Camera* camera=nullptr);
		void setCamera(Camera* camera) { m_camera = camera; }
		const Camera* getCamera() const { return m_camera; }
		SceneNode* getSceneNode() { return m_sceneNode; }
		void addObject(WorldObjectBase*);
		void removeObject(WorldObjectBase*, bool destroy=true);
		bool processMessages();
		size_t getObjectCount() const { return m_objects.size(); }

		virtual void update(float time);

		protected:
		virtual void baseObjectAdded(WorldObjectBase*) {}
		virtual void baseObjectRemoved(WorldObjectBase*) {}
		static bool traceObject(const WorldObjectBase* object, TraceGroupMask mask, const Ray& ray, float radius, float limit, float& t, vec3& normal);


		protected:
		SceneNode* m_sceneNode;
		Camera* m_camera;

		private:
		friend class WorldObjectBase;
		template<class T> friend class ObjectWorld;
		enum class Message { Add, Remove, Delete, StartUpdate, StopUpdate };
		struct MessageData { Message type; WorldObjectBase* object; };
		std::vector<MessageData> m_messages;
		std::vector<WorldObjectBase*> m_objects;
		std::vector<WorldObjectBase*> m_update;
	};


	// Inherit this one
	template<class ObjectType>
	class ObjectWorld : public world::ObjectWorldBase {
		public:
		using ObjectWorldBase::ObjectWorldBase;

		struct ObjectIterator {
			std::vector<WorldObjectBase*>::const_iterator it;
			std::vector<WorldObjectBase*>::const_iterator operator++() { return ++it; }
			ObjectType* operator*() const { return static_cast<ObjectType*>(*it); }
			bool operator!=(const ObjectIterator& i) const { return it!=i.it; }
		};
		ObjectIterator begin() const { return ObjectIterator{m_objects.begin()}; }
		ObjectIterator end() const { return ObjectIterator{m_objects.end()}; }

		using TraceResult = TraceResultT<ObjectType>;
		TraceResult trace(const Ray& ray, float radius=0, float limit=1e8f) const { return trace(-1, ray, radius, limit); }
		TraceResult trace(TraceGroupMask mask, const Ray& ray, float radius=0, float limit=1e8f) const {
			TraceResult result;
			if(traceCustom(mask, ray, radius, limit, result)) limit = result.distance;
			for(ObjectType* o: *this) {
				if(traceObject(o, mask, ray, radius, limit, result.distance, result.normal)) {
					result.object = o;
					limit = result.distance;
					result.hit = true;
				}
			}
			if(result) result.position = ray.point(result.distance) - radius * result.normal;
			return result;
		}

		protected:
		/// Additional traces for non-objects - for stuff like terrain
		virtual bool traceCustom(TraceGroupMask mask, const Ray&, float radius, float limit, TraceResult& result) const { return false; }
		virtual void objectAdded(ObjectType*) {}
		virtual void objectRemoved(ObjectType*) {}

		private:
		void baseObjectAdded(WorldObjectBase* o) final { objectAdded(static_cast<ObjectType*>(o)); }
		void baseObjectRemoved(WorldObjectBase* o) final { objectRemoved(static_cast<ObjectType*>(o)); }
	};
	template<class ObjectType> typename ObjectWorld<ObjectType>::ObjectIterator begin(const ObjectWorld<ObjectType>* world) { return world->begin(); }
	template<class ObjectType> typename ObjectWorld<ObjectType>::ObjectIterator end(const ObjectWorld<ObjectType>* world) { return world->end(); }
}
}

