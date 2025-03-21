#include <base/world/infoplate.h>
#include <base/camera.h>
#include <algorithm>

#include <base/drawable.h>
#include <base/renderer.h>
#include <base/scene.h>

using namespace base;

namespace base {
	class InfoPlateDrawable : public base::Drawable {
		public:
		InfoPlateDrawable(InfoPlate* plate) : m_plate(plate) {	}
		void draw(base::RenderState& r) override {
			if(!isVisible() || !m_plate->m_widget->isVisible()) return;
			base::Camera* cam = r.getCamera();
			const Rect& rect = m_plate->m_widget->getRect();
			float anchorX = rect.x + rect.width / 2;
			float anchorY = rect.bottom();
			float scale = 0.002 * m_plate->m_scale;

			vec3 x = vec3(0,1,0).cross(cam->getDirection()).normalise() * scale;
			vec3 y = x.cross(cam->getDirection()).normalise() * scale;

			Matrix transform(x, y, r.getCamera()->getDirection(), m_plate->m_position - anchorX*x - anchorY*y);
			transform = cam->getProjection() * cam->getModelview() * transform;

			r.setMaterial(nullptr);
			m_plate->m_widget->getRoot()->draw(transform, m_plate->m_widget, true);
		}
		private:
		InfoPlate* m_plate;
	};

	class InfoPlateSceneNode : public SceneNode {
		SceneNode*& m_pointer;
		public:
		InfoPlateSceneNode(SceneNode*& p) : SceneNode("InfoPlates"), m_pointer(p) {}
		~InfoPlateSceneNode() { m_pointer = nullptr; }
	};
}



InfoPlate::~InfoPlate() {
	m_widget->getParent()->as<InfoPlateManager>()->remove(this);
	delete m_widget;
}

void InfoPlate::setVisible(bool vis) {
	if(m_visible && !vis) {
		if(m_widget) m_widget->setVisible(false);
		if(m_drawable) m_drawable->setVisible(false);
	}
	m_visible = vis;
}

// ------------------------------------------ //

InfoPlateManager::InfoPlateManager(const base::Camera* camera, base::Scene* scene, int queue) : m_camera(camera), m_rendererQueue(queue) {
	setTangible(gui::Tangible::NONE);
	if(scene) {
		m_sceneNode = new InfoPlateSceneNode(m_sceneNode);
		scene->add(m_sceneNode);
	}
	setVisible(true);
}

InfoPlateManager::~InfoPlateManager() {
	for(InfoPlate* p: m_plates) p->m_drawable = nullptr;
	if(m_sceneNode) m_sceneNode->deleteAttachments();
	delete m_sceneNode;
}


void InfoPlateManager::setVisible(bool vis) {
	Widget::setVisible(vis && !m_sceneNode);
	if(m_sceneNode) m_sceneNode->setVisible(vis);
}

InfoPlate* InfoPlateManager::create(const char* type, float range, float scale) {
	InfoPlate* plate = new InfoPlate();
	plate->m_widget = createChild<Widget>(type);
	plate->m_widget->setVisible(false);
	plate->m_range = range * range;
	plate->m_scale = scale;
	m_plates.push_back(plate);
	if(m_sceneNode) {
		plate->m_drawable = new InfoPlateDrawable(plate);
		plate->m_drawable->setRenderQueue(m_rendererQueue);
		m_sceneNode->attach(plate->m_drawable);
		plate->m_drawable->shareTransform(nullptr);
	}
	return plate;
}

void InfoPlateManager::remove(InfoPlate* p) {
	for(size_t i=0; i<m_plates.size(); ++i) {
		if(m_plates[i] == p) {
			m_plates[i] = m_plates.back();
			m_plates.pop_back();
			break;
		}
	}
	if(p->m_drawable && m_sceneNode) {
		m_sceneNode->detach(p->m_drawable);
		delete p->m_drawable;
		p->m_drawable = nullptr;
	}
}

void InfoPlateManager::update() {
	if(!getRoot()) return;
	Point view = getRoot()->getRootWidget()->getSize();
	setSize(view);
	for(InfoPlate* p: m_plates) {
		if(!p->m_visible) continue;
		bool visible = m_camera->getDirection().dot(p->m_position - m_camera->getPosition()) < 0
					&& m_camera->getPosition().distance2(p->m_position) < p->m_range;
		p->m_widget->setVisible(visible);
		if(p->m_drawable) {
			p->m_drawable->setVisible(visible);
			p->m_drawable->setTransform(p->m_position, Quaternion(), 1.f); // Used for depth sorting
		}
		if(visible) {
			Point s = p->m_widget->getSize();
			vec3 pos = m_camera->project(p->m_position, view);
			pos.x -= s.x / 2;
			switch(p->m_widget->getAnchor() >> 4) {
			case 0: break; //Top
			case 1: pos.y += s.y; break; // Bottom;
			case 2: // Centre
			case 3: pos.y += s.y / 2; break; // Span
			}
			p->m_widget->setPosition(pos.x, view.y - pos.y);
			p->m_depth = pos.z;
		}
	}
	
	// Depth sorting
	if(!m_sceneNode) {
		std::sort(m_plates.begin(), m_plates.end(), [](InfoPlate* a, InfoPlate* b) { return a->m_depth > b->m_depth; });
		for(size_t i=0; i<m_plates.size(); ++i) m_children[i] = m_plates[i]->m_widget;
	}
}

