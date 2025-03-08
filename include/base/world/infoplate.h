#pragma once

#include <base/gui/gui.h>
#include <base/vec.h>

namespace base {

class Camera;
class SceneNode;
class Scene;

class InfoPlateDrawable;

// GUI element in the world - For stuff line name tags, healthbars etc.
class InfoPlate {
	friend class InfoPlateManager;
	friend class InfoPlateDrawable;
	public:
	~InfoPlate();
	void setPosition(const vec3& p) { m_position = p; }
	template<class T=gui::Widget> T* getWidget(const char* name) { return m_widget->getTemplateWidget<T>(name); }
	template<class T=gui::Widget> T* getWidget(int index) { return cast<T>(m_widget->getTemplateWidget(index)); }
	template<class T=gui::Widget> T* getWidget() { return cast<T>(m_widget); }
	protected:
	InfoPlate() = default;
	gui::Widget* m_widget = nullptr;
	InfoPlateDrawable* m_drawable = nullptr;
	vec3 m_position;
	float m_depth = 0;
	float m_range = 2;
	float m_scale = 1;
};

// Manages infoplates. Must be added to gui root
// Setting scene is optional.
// 	if not set, infoplaes are drawn on the gui layer
// 	if set, plates are drawn as 3d objects in the world
// Must call update every frame
class InfoPlateManager : public gui::Widget {
	WIDGET_TYPE(InfoPlateManager);
	public:
	InfoPlateManager(const base::Camera* camera, base::Scene* scene=nullptr, int renderQueue=5);
	InfoPlate* create(const char* widget, float range=10, float scale=1);
	using Widget::remove;
	void remove(InfoPlate*);
	void update();
	void setVisible(bool vis) override;

	private:
	const base::Camera* m_camera;
	std::vector<InfoPlate*> m_plates;
	base::SceneNode* m_sceneNode;
	int m_rendererQueue;
};

}

