#pragma once

#include <base/gui/delegate.h>
#include <base/gamestate.h>
#include <base/point.h>
#include <base/vec.h>
#include <map>

namespace gui { class Root; class Widget; class Button; class IconList; enum class KeyMask; }
namespace base { class Camera; class FrameBuffer; class Workspace; class Drawable; class Scene; class SceneNode; struct Mouse; }
class vec3;

namespace editor {

class EditorComponent;

// Global interface for editor
class SceneEditor : public base::GameStateComponent {
	public:
	SceneEditor();
	~SceneEditor();
	void setScene(base::Scene*, base::Camera*);
	void setWorkspace(base::Workspace*&);
	void resized(const Point& s) override;
	void setToggleKey(int keycode);
	void refresh();
	void end() override;
	void update() override;
	void draw() override;
	void setActive(bool);
	bool isActive() const;

	base::SceneNode* getSceneRoot() const;
	base::Workspace* getWorkspace() const { return m_workspace? *m_workspace: 0 ; }
	base::Camera* getCamera() const { return m_camera; }
	void changeWorkspace(base::Workspace* ws);



	struct TraceResult {
		base::SceneNode* node = nullptr;
		base::Drawable* drawable = nullptr;
		vec3 location;
		operator bool() const { return node; }
	};
	TraceResult trace(const Point& screenPos);

	// Add a button to the top level editor interface
	gui::Button* addButton(const char* iconset, const char* icon);
	gui::Root* getGUI() { return m_gui; }

	bool canDrop(const Point& p, int key) const;
	void drop(const Point& p, int key, const char* data);
	void cancelActiveTools(EditorComponent* skip=nullptr); // An action can cancel effects of other editors

	// Handlers for opening resources for edit/preview
	template<class F> void addHandler(const F& lambda) { m_handlers.push_back({}); m_handlers.back().bind(lambda); }
	template<class M> void addHandler(M* inst, bool(M::*func)(const char*)) { m_handlers.push_back(bind(inst, func)); }
	bool callHandler(const char* file);

	// Scene object construction - dropping files on the world
	template<class F> void addConstructor(const F& lambda) { m_construct.push_back({}); m_construct.back().bind(lambda); }
	template<class M> void addConstructor(M* inst, bool(M::*func)(const char*)) { m_construct.push_back(bind(inst, func)); }
	base::SceneNode* constructObject(const char* source);

	bool addEmbeddedPNGImage(const char* name, const char& bin, unsigned length);

	private:
	using CreateList = std::vector<EditorComponent*(*)()>;
	template<class ...R> struct Adder;
	template<class E, class ...R> struct Adder<E, R...> { static void add(CreateList& list) { Adder<E>::add(list); Adder<R...>::add(list); } };
	template<class E> struct Adder<E> { static void add(CreateList& list) { list.push_back([]()->EditorComponent*{return new E;}); } };
	public:
	// Initialise with editor->setup<editor::LayoutEditor, editor::CompositorEditor, editor::AudioEditor>();
	template<class ...R> void add() {
		//m_creation.push_back([]()->EditorComponent*{ return new E(); });
		//if constexpr(sizeof...(R)) add<R...>(); // Requires c++17, and avoids the template nonsense above
		Adder<R...>::add(m_creation);
	}

	template<class T> T* getComponent() {
		for(EditorComponent* c: m_components) if(T* r=dynamic_cast<T*>(c)) return r;
		return nullptr;
	}

	protected:
	void initialiseComponents();
	bool detectScene();

	protected:
	friend class EditorComponent;
	bool m_active = false;
	int m_toggleKey;
	gui::Root* m_gui = nullptr;
	struct {
		gui::Widget* tip = nullptr;
		gui::Widget* target = nullptr;
		float time = 0;
	} m_toolTip;
	base::Workspace** m_workspace = nullptr;
	base::Camera* m_camera = nullptr;
	base::Scene* m_scene = nullptr;
	base::FrameBuffer* m_selectionBuffer = nullptr;
	std::vector<EditorComponent*> m_components;
	std::vector<EditorComponent*(*)()> m_creation;
	std::vector<Delegate<bool(const char*)>> m_handlers;
	std::vector<Delegate<base::SceneNode*(const char*)>> m_construct;
};

struct MouseEventData {
	const base::Mouse& mouse;
	Ray ray;
	gui::KeyMask keyMask;
	bool overGUI;
};

class EditorComponent {
	private:
	friend class SceneEditor;
	SceneEditor* m_editor;
	public:
	virtual ~EditorComponent() {}
	virtual void initialise() = 0;
	virtual void update() {}
	virtual bool isActive() const { return false; }
	virtual void cancel() {}
	virtual void refresh() {}
	virtual void activate() {}
	virtual void deactivate() {}
	SceneEditor* getEditor() { return m_editor; }
	protected:
	gui::Button* addToggleButton(gui::Widget* panel, const char* iconset, const char* iconName);
	gui::Widget* loadUI(const char* file) { char e=0; return loadUI(file, e); }
	gui::Widget* loadUI(const char* file, const char& embed);
	virtual bool canDrop(const Point& p, int key) const { return false;  }
	virtual bool drop(const Point& p, int key, const char* file) { return false; }

	// Use these for editors that interact with the viewport. Return true to stop processing
	virtual bool mouseEvent(const MouseEventData&) { return false; }
	virtual bool wheelEvent(const MouseEventData&) { return false; }
	virtual bool keyEvent(int keyCode) { return false; }
	void promoteEventHandler(); // Make sure this editor handles events first

};


}

