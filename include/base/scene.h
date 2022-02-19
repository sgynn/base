#pragma once

#include <vector>
#include <base/math.h>

namespace base { class Camera; }

namespace base {
	class Drawable;
	class Renderer;
	class Scene;

	// Ragned iterator helper
	template<class T> class Iterable {
		const T m_begin, m_end;
		public:
		Iterable(const T& begin, const T& end) : m_begin(begin), m_end(end) {}
		const T& begin() const { return m_begin; }
		const T& end() const { return m_end; }
	};
	
	/// Scene node
	class SceneNode {
		public:
		SceneNode(const char* name=0);
		virtual ~SceneNode();
		
		SceneNode* createChild(const char* name, const vec3& pos=vec3());
		SceneNode* createChild(const vec3& pos=vec3()) { return createChild(0,pos); }
		void addChild(SceneNode*);
		bool removeChild(SceneNode*);
		bool removeFromParent();
		void attach(Drawable*);
		void detach(Drawable*);

		void setVisible(bool v);
		bool isVisible() const;
		bool isParentVisible() const;

		const vec3&       getPosition() const;
		const Quaternion& getOrientation() const;
		const vec3&       getScale() const;

		const Matrix& getDerivedTransform() const;
		const Matrix& getDerivedTransformUpdated();

		void setPosition(const vec3&);
		void setOrientation(const Quaternion&);
		void setScale(const vec3&);
		void setScale(float scale);
		void setTransform(const Matrix&);
		void setTransform(const vec3& pos, const Quaternion& r);
		void setTransform(const vec3& pos, const Quaternion& r, const vec3& scale);
		virtual void notifyChange();
		virtual void notifyAdded();
		virtual void notifyRemoved();

		void move(const vec3& delta);
		void moveLocal(const vec3& delta);
		void rotate(const Quaternion& delta);

		SceneNode* getParent() const;
		SceneNode* getChild(size_t index) const;
		SceneNode* getChild(const char* name) const;
		Drawable*  getAttachment(size_t index) const;
		size_t     getChildCount() const;
		size_t     getAttachmentCount() const;
		bool       isAttached(const Drawable*) const;

		void        deleteAttachments(bool recursive=false);
		void        deleteChildren(bool deleteAttachments=false);
		void        setName(const char*);
		const char* getName() const;

		// Iterators
		Iterable<std::vector<SceneNode*>::iterator> children() { return Iterable<std::vector<SceneNode*>::iterator>(m_children.begin(), m_children.end()); }
		Iterable<std::vector<Drawable*>::iterator> attachments() { return Iterable<std::vector<Drawable*>::iterator>(m_drawables.begin(), m_drawables.end()); }

		protected:
		friend class Scene;
		vec3       m_position;
		Quaternion m_orientation;
		vec3       m_scale;
		Matrix     m_derived;
		size_t     m_depth;
		bool       m_visible;
		char*      m_name;

		Scene*                  m_scene;
		SceneNode*              m_parent;
		std::vector<SceneNode*> m_children;
		std::vector<Drawable*>  m_drawables;

		private:
		bool       m_changed;
	};

	/// Scene graph
	class Scene {
		public:
		Scene();
		~Scene();

		SceneNode* getRootNode() const;
		SceneNode*  add(SceneNode* n) { getRootNode()->addChild(n); return n; }
		SceneNode*  add(Drawable* d, const char* name=0)                                          { SceneNode* n=getRootNode()->createChild(name); n->attach(d); return n; }
		SceneNode*  add(Drawable* d, const Matrix& transform, const char* name=0)                 { SceneNode* n=getRootNode()->createChild(name); n->attach(d); n->setTransform(transform); return n; }
		SceneNode*  add(Drawable* d, const vec3& pos, const char* name=0)                         { SceneNode* n=getRootNode()->createChild(name); n->attach(d); n->setPosition(pos); return n; }
		SceneNode*  add(Drawable* d, const vec3& pos, const Quaternion& rot, const char* name=0)  { SceneNode* n=getRootNode()->createChild(name); n->attach(d); n->setTransform(pos,rot); return n; }
		SceneNode*  add(const char* name, const vec3& pos=vec3())                                 { return getRootNode()->createChild(name, pos); }

		void notifyAdd(SceneNode*);
		void notifyChange(SceneNode*);
		void notifyRemove(SceneNode*);

		/// Update derived transforms (ToDo: multithreaded)
		void  updateSceneGraph();

		/// Populate renderer with drawables (ToDo: multithread this)
		void collect(Renderer* target, base::Camera* camera, unsigned char first=0, unsigned char last=255) const;

		protected:
		SceneNode* m_rootNode;
		std::vector< std::vector<SceneNode*> > m_changed;
	};

}

