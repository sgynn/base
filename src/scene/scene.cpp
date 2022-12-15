#include <base/scene.h>
#include <base/drawable.h>
#include <base/renderer.h>
#include <base/camera.h>
#include <assert.h>
#include <cstring>

using namespace base;


SceneNode::SceneNode(const char* name): m_scale(1,1,1), m_depth(0), m_visible(true), m_name(0), m_scene(0), m_parent(0), m_changed(false), m_updateQueued(false) {
	if(name) setName(name);
}

SceneNode::~SceneNode() {
	removeFromParent();
	deleteChildren(false);
	if(m_name) free(m_name);
}

void SceneNode::setName(const char* n) {
	if(m_name) free(m_name);
	m_name = n? strdup(n): 0;
}

const char* SceneNode::getName() const {
	return m_name? m_name: "";
}

SceneNode* SceneNode::createChild(const char* name, const vec3& pos) {
	SceneNode* n = new SceneNode(name);
	n->setPosition(pos);
	addChild(n);
	return n;
}

void SceneNode::addChild(SceneNode* n) {
	assert(n!=this); // Can't add to self
	if(n->m_parent==this) return; // already added
	if(n->m_parent) n->m_parent->removeChild(n);
	m_children.push_back(n);
	n->m_parent = this;
	n->notifyAdded();
}

bool SceneNode::removeChild(SceneNode* n) {
	for(size_t i=0; i<m_children.size(); ++i) {
		if(m_children[i] == n) {
			m_children[i] = m_children.back();
			m_children.pop_back();
			n->notifyRemoved();
			n->m_parent = 0;
			n->m_scene = 0;
			n->m_depth = 0;
			return true;
		}
	}
	return false;
}

bool SceneNode::removeFromParent() {
	if(m_parent) return m_parent->removeChild(this);
	return false;
}

void SceneNode::notifyAdded() {
	m_depth = m_parent->m_depth+1;
	m_scene = m_parent->m_scene;
	for(SceneNode* n: m_children) n->notifyAdded();
	if(m_scene) m_scene->notifyAdd(this);
	m_changed = false; // force add to change list
	notifyChange();
}

void SceneNode::notifyRemoved() {
	if(m_scene) m_scene->notifyRemove(this);
	for(SceneNode* n: m_children) n->notifyRemoved();
	m_scene = 0;
}


void SceneNode::attach(Drawable* d) {
	m_drawables.push_back(d);
	d->shareTransform(&m_derived);
}

void SceneNode::detach(Drawable* d) {
	for(size_t i=0; i<m_drawables.size(); ++i) {
		if(m_drawables[i] == d) {
			m_drawables[i] = m_drawables.back();
			m_drawables.pop_back();
			d->shareTransform(0);
			return;
		}
	}
}

bool SceneNode::isAttached(const Drawable* item) const {
	for(Drawable* d: m_drawables) if(d==item) return true;
	return false;
}

void SceneNode::deleteAttachments(bool recurse) {
	if(recurse) for(SceneNode* n: m_children) n->deleteAttachments(recurse);
	for(Drawable* d: m_drawables) delete d;
	m_drawables.clear();
}

void SceneNode::deleteChildren(bool attachments) {
	if(attachments) deleteAttachments(true);
	for(SceneNode* n: m_children) {
		n->m_parent = 0; // to not invalidate iterator
		delete n;
	}
	m_children.clear();
}

SceneNode* SceneNode::getParent() const { return m_parent; }
SceneNode* SceneNode::getChild(size_t index) const { return index<m_children.size()? m_children[index]: 0; }
SceneNode* SceneNode::getChild(const char* name) const {
	for(SceneNode* node: m_children) {
		if(node->m_name && strcmp(node->m_name, name)==0) return node;
	}
	return 0;
}
size_t SceneNode::getChildCount() const { return m_children.size(); }
size_t SceneNode::getAttachmentCount() const { return m_drawables.size(); }
Drawable* SceneNode::getAttachment(size_t index) const { return index<m_drawables.size()? m_drawables[index]: 0; }

const vec3& SceneNode::getPosition() const { return m_position; }
const Quaternion& SceneNode::getOrientation() const { return m_orientation; }
const vec3& SceneNode::getScale() const { return m_scale; }

void SceneNode::setVisible(bool v) {
	m_visible = v;
}

bool SceneNode::isVisible() const {
	return m_visible;
}

bool SceneNode::isParentVisible() const {
	return !m_parent || (m_parent->m_visible && m_parent->isParentVisible());
}


void SceneNode::setPosition(const vec3& p) {
	m_position = p;
	notifyChange();
}

void SceneNode::setOrientation(const Quaternion& q) {
	m_orientation = q;
	notifyChange();
}

void SceneNode::setScale(const vec3& s) {
	m_scale = s;
	notifyChange();
}

void SceneNode::setScale(float s) {
	m_scale.set(s,s,s);
	notifyChange();
}

void SceneNode::setTransform(const Matrix& m) {
	m_position.set(m[12], m[13], m[14]);
	m_orientation.fromMatrix(m);
	notifyChange();
}

void SceneNode::setTransform(const vec3& p, const Quaternion& q) {
	m_position = p;
	m_orientation = q;
	notifyChange();
}

void SceneNode::setTransform(const vec3& p, const Quaternion& q, const vec3& s) {
	m_position = p;
	m_orientation = q;
	m_scale = s;
	notifyChange();
}

void SceneNode::move(const vec3& delta) {
	m_position += delta;
	notifyChange();
}

void SceneNode::moveLocal(const vec3& delta) {
	m_position += m_orientation * delta;
	notifyChange();
}

void SceneNode::rotate(const Quaternion& delta) {
	m_orientation *= delta;
	notifyChange();
}

const Matrix& SceneNode::getDerivedTransform() const {
	return m_derived;
}

const Matrix& SceneNode::getDerivedTransformUpdated() {
	if(m_changed && m_parent) {
		const Matrix& p = m_parent->getDerivedTransformUpdated();
		Matrix tmp;
		createLocalMatrix(tmp);
		m_derived = p * tmp;
		m_changed = false;
		for(SceneNode* c: children()) c->notifyChange();
	}
	return m_derived;
}

void SceneNode::createLocalMatrix(Matrix& out) const {
	m_orientation.toMatrix(out);
	out.setTranslation(m_position);
	out.scale(m_scale);
}

void SceneNode::notifyChange() {
	if(!m_changed && m_scene) m_scene->notifyChange(this);
	m_changed = true;
}


// ==================================================================================== //

Scene::Scene() {
	m_rootNode = new SceneNode("SceneRoot");
	m_rootNode->m_scene = this;
}

Scene::~Scene() {
	delete m_rootNode;
}

SceneNode* Scene::getRootNode() const {
	return m_rootNode;
}

void Scene::notifyAdd(SceneNode* n) {
	if(n->m_depth >= m_changed.size()) m_changed.resize(n->m_depth+4);
}

void Scene::notifyChange(SceneNode* n) {
	m_changed[n->m_depth].push_back(n);
	n->m_updateQueued = true;
}

void Scene::notifyRemove(SceneNode* n) {
	if(n->m_updateQueued) {
		n->m_updateQueued = false;
		std::vector<SceneNode*>& list = m_changed[n->m_depth];
		for(size_t i=0; i<list.size(); ++i) {
			if(list[i] == n) { list[i] = list.back(); list.pop_back(); break; }
		}
	}
}

void Scene::updateSceneGraph() {
	static SceneNode dummy;
	m_rootNode->m_parent = &dummy;
	Matrix tmp;
	for(auto& level : m_changed) {
		for(SceneNode* n : level) {
			if(n->m_changed) {
				n->createLocalMatrix(tmp);
				n->m_derived = n->m_parent->m_derived * tmp;
				n->m_changed = false;
				n->m_updateQueued = false;
				// Children
				for(SceneNode* c: n->children()) c->notifyChange();
			}
		}
		level.clear();
	}
	m_rootNode->m_parent = 0;
}

void Scene::collect(Renderer* r, Camera* cam, unsigned char a, unsigned char b) const {
	std::vector<SceneNode*> queue;
	queue.push_back(getRootNode());
	for(size_t i=0; i<queue.size(); ++i) {
		if(queue[i]->isVisible()) {
			for(Drawable* d: queue[i]->m_drawables) {
				if(d->isVisible()) {
					// ToDo: render queue filtering and frustum culling.
					// ToDo: multithreaded add
					r->add(d, d->getRenderQueue());
				}
			}
			for(SceneNode* n : queue[i]->m_children) queue.push_back(n);
		}
	}
}


