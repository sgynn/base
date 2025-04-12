#pragma once

#include <base/gui/delegate.h>
#include <base/gamestate.h>
#include <base/hashmap.h>
#include <base/string.h>
#include <base/point.h>
#include <base/vec.h>
#include <base/virtualfilesystem.h>

namespace gui { class Root; class Widget; class Button; class IconList; enum class KeyMask; class MenuBuilder; }
namespace base { class Camera; class FrameBuffer; class Workspace; class Drawable; class Scene; class SceneNode; struct Mouse; class ResourceManagerBase; }
class vec3;

namespace editor {

class EditorComponent;
class SceneEditor;

// Matches resource manager types
enum class ResourceType { None, Model, Texture, Material, Shader, ShaderVars, Compositor, Graph, Particle, Custom };
struct Asset {
	ResourceType type = ResourceType::None;
	base::String resource; // Resource name. Empty if unloaded
	base::VirtualFileSystem::File file;
};

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

	base::SceneNode* getSelectedSceneNode() const { return m_selectedSceneNode; }
	base::Drawable* getSelectedDrawable() const { return m_selectedDrawable; }

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

	bool canDrop(const Point& p, const Asset&) const;
	void drop(const Point& p, const Asset&);
	void cancelActiveTools(EditorComponent* skip=nullptr); // An action can cancel effects of other editors

	// Handlers for opening resources for edit/preview
	MultiDelegate<void(const Asset&, bool)> assetChanged;

	// Scene object construction - dropping files on the world - SceneNode* (const Asset&);
	template<class F> void addConstructor(const F& lambda) { m_construct.push_back({}); m_construct.back().bind(lambda); }
	template<class M> void addConstructor(M* inst, base::SceneNode*(M::*func)(const Asset&)) { m_construct.push_back(bind(inst, func)); }
	base::SceneNode* constructObject(const Asset&);

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

	void addTransientComponent(EditorComponent* c); // For sub-compoonents
	const std::vector<EditorComponent*>& getComponents() const { return m_components; }
	friend std::vector<EditorComponent*>::const_reverse_iterator begin(SceneEditor* e) { return e->m_components.rbegin(); }
	friend std::vector<EditorComponent*>::const_reverse_iterator end(SceneEditor* e) { return e->m_components.rend(); }

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
	base::SceneNode* m_selectedSceneNode = nullptr;
	base::Drawable* m_selectedDrawable = nullptr;
	base::VirtualFileSystem* m_filesystem = nullptr;
	std::vector<EditorComponent*> m_components;
	std::vector<EditorComponent*(*)()> m_creation;
	std::vector<Delegate<base::SceneNode*(const Asset&)>> m_construct;
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
	base::VirtualFileSystem& getFileSystem() const { return *m_editor->m_filesystem; };
	public:
	struct AssetCreationBuilder {
		std::vector<std::pair<const char*, Delegate<Asset(const char*)>>> list;
		template<class ...T>void add(const char* name, T...t) { list.push_back({name, {}}); list.back().second.bind(t...); };
	};
	virtual gui::Widget* openAsset(const Asset&) { return nullptr; }
	virtual void assetCreationActions(AssetCreationBuilder&) {}
	virtual bool assetActions(gui::MenuBuilder& menu, const Asset&) { return false; }
	virtual int assetThumbnail(const Asset&) { return 0; }
	static const char* getResourceNameFromFile(base::ResourceManagerBase& rc, const char* file);
	protected:
	gui::Button* addToggleButton(gui::Widget* panel, const char* iconset, const char* iconName);
	gui::Widget* loadUI(const char* file) { char e=0; return loadUI(file, e); }
	gui::Widget* loadUI(const char* file, const char& embed);
	virtual bool drop(gui::Widget* target, const Point& p, const Asset&, bool apply) { return false; }

	void setSelectedObject(base::SceneNode* n, base::Drawable* d=nullptr) { m_editor->m_selectedSceneNode = n; m_editor->m_selectedDrawable = d; }

	// Use these for editors that interact with the viewport. Return true to stop processing
	virtual bool mouseEvent(const MouseEventData&) { return false; }
	virtual bool wheelEvent(const MouseEventData&) { return false; }
	virtual bool keyEvent(int keyCode) { return false; }
	void promoteEventHandler(); // Make sure this editor handles events first
};


}

