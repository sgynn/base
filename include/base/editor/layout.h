#pragma once

#include "editor.h"
#include <base/scene.h>
#include <base/drawablemesh.h>
#include <base/gui/widgets.h>

namespace base { class Camera; }
namespace gui { class TreeView; class TreeNode; }

namespace editor {
class Gizmo;
class DrawableType;
class SceneNodeType;

enum class GizmoMode { Move, Rotate, Scale };
enum class GizmoSpace { Global, Parent, Local };

class LayoutViewer : public EditorComponent {
	public:
	LayoutViewer();
	~LayoutViewer();
	void initialise() override;
	void setGizmoRenderQueue(int);		// Set gizmo render queue if needed by the compositor
	void selectItem(base::SceneNode*);	// Select an item in treeview
	void setRoot(base::SceneNode*);		// Set root scene node
	void refresh() override;			// Refresh scene tree display. Call this after the scene graph has changed
	void update() override;				// Updates selected item values gizmos
	bool isActive() const override;		// Is this window visible
	void activate() override;			// Editor activated
	void deactivate() override;			// Editor deactivated
	void cancel() override;
	void sceneDestroyed();				// Internal - callback if active scene gets destroyed

	void setGizmoMode(GizmoMode);
	void setGizmoSpace(GizmoSpace);

	public:
	static void registerType(SceneNodeType*);
	static void registerType(DrawableType*);
	SceneNodeType* getType(base::SceneNode*) const;
	DrawableType*  getType(base::Drawable*) const;
	private:
	static std::vector<SceneNodeType*> s_nodeTypes;
	static std::vector<DrawableType*> s_drawableTypes;


	private:
	bool mouseEvent(const MouseEventData&) final;
	void populate(gui::TreeNode*, base::SceneNode*, const char* filter);
	void refreshItem(gui::TreeNode*, gui::Widget*);
	void itemEvent(gui::TreeView*, gui::TreeNode*, gui::Widget*);
	void itemSelected(gui::TreeView*, gui::TreeNode*);
	void filterChanged(gui::Textbox*, const char*);
	void closed(gui::Window*);

	void createGizmo();
	void destroyGizmo();

	bool filterItem(base::SceneNode*, const char*) const;
	bool filterItem(base::Drawable*, const char*) const;
	static bool isValid(gui::TreeNode*);
	static bool isParentVisible(gui::TreeNode*);

	private:
	gui::Widget*      m_panel = 0;
	gui::Widget*      m_properties = 0;
	gui::Widget*      m_gizmoWidget = 0;
	gui::TreeView*    m_tree = 0;
	gui::Textbox*     m_filter = 0;
	gui::Button*      m_clearFilter = 0;
	base::SceneNode*  m_active = 0;
	base::Drawable*   m_drawable = 0;
	SceneNodeType*    m_activeType = 0;
	base::SceneNode*  m_listener = 0;
	uint              m_lastNodeCount = 0;
	Gizmo*            m_gizmo = 0;
	int               m_gizmoQueue = 10;
	GizmoSpace        m_gizmoSpace = GizmoSpace::Global;
};

class LayoutCategory {
	friend class LayoutProperties;
	LayoutCategory(gui::Widget* p) : m_panel(p) {}
	gui::Widget* m_panel;
	public:
	template<class T> T* createWidget(const char* type) const { return m_panel->getRoot()->createWidget<T>(type); }
	template<class T> T* addWidget(const char* type) { T* w=createWidget<T>(type); add(w); return w; }
	LayoutCategory addRow();
	gui::Label* addLabel(const char* text, int size=0);
	void add(gui::Widget* w) { m_panel->add(w); }
};

// Helper for adding widgets to details panel
class LayoutProperties {
	gui::Widget* m_panel;
	public:
	LayoutProperties(gui::Widget* panel) : m_panel(panel) {}
	LayoutCategory getCategory(const char* name);
	void addTransform(base::SceneNode* node);
	void addMaterial(base::Material* material);
};

// Needs property panels:
// SceneNode: position,orientation,scale,visibility;
// Drawable: material, renderqueue
// DrawableMesh [+]: skeleton, mesh
// Perhaps register property panels for different types.
// Two base classes: SceneNode and Drawable.
// Will probably need to run searches to find out which model a mesh is from.
// Unfortunately a skeleton instance loses the reference. May be possible with complex search.
//
// SceneNodes can have option of adding and removing drawables.
// Drawables can change mesh and material.

class SceneNodeType {
	public:
	virtual ~SceneNodeType() {}
	virtual bool match(base::SceneNode*) const;
	virtual void getValues(base::SceneNode*, const char*& name, int& icon, bool& visible) const;
	virtual void setVisible(base::SceneNode*, bool) const;
	virtual void showProperties(LayoutProperties& panel, base::SceneNode* node);
	virtual void updateTransform(gui::Widget* detailsPanel);
	virtual bool showAttachments() const { return true; }
	virtual bool showChildren() const { return true; }
	private:
	base::SceneNode* m_node = nullptr;
	vec3 m_pos, m_scale;
	Quaternion m_rot;
};

class DrawableType {
	public:
	virtual ~DrawableType() {}
	virtual bool match(base::Drawable*) const;
	virtual void getValues(base::Drawable*, const char*& name, int& icon, bool& visible) const;
	virtual void setVisible(base::Drawable*, bool) const;
	virtual void showProperties(LayoutProperties& panel, base::SceneNode* parent, base::Drawable* item);
};

class DrawableMeshType : public DrawableType {
	public:
	bool match(base::Drawable*) const override;
	void getValues(base::Drawable*, const char*& name, int& icon, bool& visible) const override;
	void showProperties(LayoutProperties& panel, base::SceneNode* parent, base::Drawable* item) override;
};

template<class T>
class CustomSceneNodeType : public SceneNodeType {
	public:
	CustomSceneNodeType(int icon, bool attachments=true) : m_attachments(attachments), m_icon(icon) {}
	bool match(base::SceneNode* n) const override { return typeid(*n) == typeid(T); }
	bool showAttachments() const override { return m_attachments; }
	void getValues(base::SceneNode* node, const char*& name, int& icon, bool& visible) const override {
		name = node->getName();
		icon = m_icon;
	}
	private:
	bool m_attachments;
	int m_icon;
};

template<class T>
class CustomDrawableType : public DrawableType {
	public:
	CustomDrawableType(int icon, const char* name) : m_name(name), m_icon(icon) {}
	bool match(base::Drawable* d) const override { return typeid(*d) == typeid(T); }
	void getValues(base::Drawable*, const char*& name, int& icon, bool& visible) const override {
		name = m_name;
		icon = m_icon;
	}
	private:
	gui::String m_name;
	int m_icon;
	
};


}

