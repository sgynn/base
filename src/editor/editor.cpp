#include <base/editor/editor.h>
#include <base/editor/layout.h>
#include <base/editor/assetbrowser.h>
#include <base/editor/compositoreditor.h>
#include <base/editor/particleeditor.h>
#include <base/editor/renderdoc.h>

#include <base/gui/gui.h>
#include <base/gui/renderer.h>
#include <base/camera.h>
#include <base/resources.h>
#include <base/drawable.h>
#include <base/material.h>
#include <base/renderer.h>
#include <base/compositor.h>
#include <base/autovariables.h>
#include <base/scenecomponent.h>

#include <base/framebuffer.h>
#include <base/game.h>
#include <base/input.h>
#include <base/opengl.h>
#include <base/png.h>
#include <cstdio>

using namespace editor;
using namespace base;
using namespace gui;

#include <base/editor/embed.h>
BINDATA(editor_gui, EDITOR_DATA "/editor.xml")
BINDATA(editor_icons,  EDITOR_DATA "/editoricons.png")

SceneEditor::SceneEditor() : GameStateComponent(-50, 80, PERSISTENT) {
	m_toggleKey = KEY_F12;
	add<LayoutViewer, CompositorEditor, AssetBrowser, ParticleEditorComponent, RenderDoc>();
}

SceneEditor::~SceneEditor() {
	for(EditorComponent* c: m_components) delete c;
	delete m_selectionBuffer;
	delete m_gui;
}

void SceneEditor::setToggleKey(int key) {
	m_toggleKey = key;
}

bool SceneEditor::addEmbeddedPNGImage(const char* name, const char& bin, unsigned length) {
	base::Image image = PNG::parse(&bin, length);
	if(!image) return false;
	m_gui->getRenderer()->addImage(name, image.getWidth(), image.getHeight(), image.getBytesPerPixel(), image.getData());
	return true;
}

void SceneEditor::initialiseComponents() {
	printf("------ EDITOR ------\n");
	m_gui = new Root(base::Game::getSize());
	addEmbeddedPNGImage("data/editor/editoricons.png", editor_icons, editor_icons_len);
	if(m_gui->load("data/editor/editor.xml") || m_gui->parse(&editor_gui)) {
		m_toolTip.tip = m_gui->getWidget("tooltip");
		for(auto& create: m_creation) {
			EditorComponent* component = create();
			m_components.push_back(component);
			component->m_editor = this;
			component->initialise();
		}
	}
	m_creation.clear();
	printf("--------------------\n");
}

void SceneEditor::addTransientComponent(EditorComponent* c) {
	assert(!m_components.empty());
	m_components.push_back(c);
	c->m_editor = this;
	c->initialise();
}

bool SceneEditor::detectScene() {
	if(SceneComponent* c = getState()->getComponent<SceneComponent>()) {
		m_workspace = &c->getWorkspace();
		m_camera = c->getWorkspace()->getCamera();
		m_scene = c->getScene();
		return m_scene;
	}
	return false;
}

void SceneEditor::end() {
	setActive(false);
	m_workspace = nullptr;
	m_camera = nullptr;
	m_scene = nullptr;
}

Button* SceneEditor::addButton(const char* iconset, const char* name) {
	Button* b = m_gui->createWidget<Button>("iconbutton");
	m_gui->getWidget("editor")->add(b);
	IconList* list = m_gui->getIconList(iconset);
	b->setIcon(list, name);
	return b;
}

Button* EditorComponent::addToggleButton(gui::Widget* panel, const char* iconset, const char* iconName) {
	Button* b = m_editor->addButton(iconset, iconName);
	b->eventPressed.bind([this, panel](Button* b) {
		bool on = !isActive();
		if(panel) panel->setVisible(on);
		if(panel && on) panel->raise();
		if(on) { activate(); refresh(); }
		else deactivate();
		b->setSelected(on);
	});
	if(gui::Window* w = cast<gui::Window>(panel)) {
		w->eventClosed.bind([this, b](gui::Window*) {
			b->setSelected(false);
			deactivate();
		});
	}
	return b;
}

Widget* EditorComponent::loadUI(const char* file, const char& embed) {
	char path[64];
	snprintf(path, 63, EDITOR_DATA "/%s", file);
	Widget* panel = m_editor->getGUI()->load(path);
	if(!panel && embed) panel = m_editor->getGUI()->parse(&embed);
	if(panel) panel->setVisible(false);
	return panel;
}

const char* EditorComponent::getResourceNameFromFile(base::ResourceManagerBase& rc, const char* filename) {
	if(!filename || !filename[0]) return nullptr;
	if(filename[0]=='.' && filename[1]=='/') filename += 2;

	/*
	StringView file(filename);
	for(const char* path: rc.getSearchPaths()) {
		if(path[0]=='.' && path[1]=='/') path += 2;
		if(file.startsWith(path)) {
			filename += strlen(path);
			if(filename[0]=='/') ++filename;
			return filename;
		}
	}
	*/
	return nullptr;
}

// ====================================================================== //

void SceneEditor::resized(const Point& s) {
	if(m_gui) m_gui->resize(s);
}

base::SceneNode* SceneEditor::getSceneRoot() const {
	if(!m_scene) return 0;
	return m_scene->getRootNode();
}

void SceneEditor::setScene(base::Scene* scene, base::Camera* camera) {
	m_scene = scene;
	m_camera = camera;
	refresh();
}

void SceneEditor::setWorkspace(base::Workspace*& ws) {
	m_workspace = &ws;
	refresh();
}

bool SceneEditor::isActive() const {
	return m_active;
}

void SceneEditor::setActive(bool a) {
	if(a==m_active) return;
	m_active = a;
	if(m_active) {
		if(!m_gui) initialiseComponents();
		if(!m_scene) detectScene();
		m_gui->getRootWidget()->setFocus();
		for(EditorComponent* c: m_components) {
			if(c->isActive()) {
				c->activate();
				c->refresh();
			}
		}
	}
	else {
		for(EditorComponent* c: m_components) c->deactivate();
	}
}

void SceneEditor::cancelActiveTools(EditorComponent* skip) {
	for(EditorComponent* c: m_components) if(c != skip) c->cancel();
}

void SceneEditor::refresh() {
	for(EditorComponent* c: m_components) if(c->isActive()) c->refresh();
}

void SceneEditor::update() {
	if(m_toggleKey && Game::Pressed(m_toggleKey)) setActive(!m_active);
	if(!m_active) return;

	const Mouse& mouse = Game::input()->mouse;
	m_gui->setKeyMask((gui::KeyMask)(Game::input()->getKeyModifier()>>9));
	m_gui->mouseEvent(Point(mouse.x, Game::height()-mouse.y), mouse.button, mouse.wheel);
	if(Game::LastKey()) m_gui->keyEvent(Game::LastKey(), Game::LastChar());
	m_gui->updateAnimators(Game::frameTime());

	// Block flags from gui
	if(m_gui->getWidgetUnderMouse()) setComponentFlags(BLOCK_MOUSE);
	if(m_gui->getWheelEventConsumed()) setComponentFlags(BLOCK_WHEEL);
	if(cast<Textbox>(m_gui->getFocusedWidget())) setComponentFlags(BLOCK_KEYS);
	setComponentFlags(BLOCK_GRAB);

	// Component events
	Ray ray = getCamera()? getCamera()->getMouseRay(mouse, Game::getSize()): Ray();
	MouseEventData event{mouse, ray, m_gui->getKeyMask(), (bool)m_gui->getWidgetUnderMouse()};
	if(mouse.delta.x || mouse.delta.y || mouse.pressed || mouse.released) {
		for(EditorComponent* c: m_components) {
			if(c->mouseEvent(event)) { setComponentFlags(BLOCK_MOUSE); break; }
		}
	}
	if(mouse.wheel && !hasComponentFlags(BLOCK_WHEEL)) {
		for(EditorComponent* c: m_components) {
			if(c->wheelEvent(event)) { setComponentFlags(BLOCK_WHEEL); break; }
		}
	}
	if(Game::LastKey() && !hasComponentFlags(BLOCK_KEYS)) {
		for(EditorComponent* c: m_components) {
			if(c->keyEvent(Game::LastKey())) { setComponentFlags(BLOCK_KEYS); break; }
		}
	}

	// General component updates
	for(EditorComponent* c: m_components) c->update();

	// Tooltip
	if(m_toolTip.tip) {
		Widget* target = m_gui->getWidgetUnderMouse();
		while(target && target->isTemplate()) target = target->getParent();
		if(target != m_toolTip.target || mouse.button) m_toolTip.time = 0;
		else if(!m_toolTip.tip->isVisible() && (mouse.delta.x || mouse.delta.y)) m_toolTip.time = 0;
		m_toolTip.target = target;
		if(m_toolTip.time<1) m_toolTip.tip->setVisible(false);
		else if(m_toolTip.target && m_toolTip.target->getToolTip() && !m_toolTip.tip->isVisible()) {
			m_toolTip.tip->getWidget(0)->as<Label>()->setCaption(m_toolTip.target->getToolTip());
			m_toolTip.tip->setVisible(true);
			m_toolTip.tip->raise();
		}
		if(m_toolTip.tip->isVisible()) {
			Point view = m_toolTip.tip->getParent()->getSize();
			Point size = m_toolTip.tip->getSize();
			Point pos = m_gui->getMousePos() + Point(1, 16);
			if(pos.x + size.x > view.x) pos.x = view.x - size.x;
			m_toolTip.tip->setPosition(pos);
		}
		m_toolTip.time += 3/60.f;
	}
}

void EditorComponent::promoteEventHandler() {
	if(m_editor->m_components[0] == this) return;
	for(size_t i=1; i<m_editor->m_components.size(); ++i) {
		if(m_editor->m_components[i] == this) {
			while(i-->0) m_editor->m_components[i+1] = m_editor->m_components[i];
			m_editor->m_components[0] = this;
			break;
		}
	}
}

void SceneEditor::draw() {
	if(m_active) m_gui->draw();
}


void SceneEditor::changeWorkspace(Workspace* ws) {
	if(ws && m_workspace) {
		delete *m_workspace;
		*m_workspace = ws;
	}
}


// --------------------------------- //

SceneEditor::TraceResult SceneEditor::trace(const Point& pos) {
	if(!m_scene || !m_camera) return TraceResult();

	struct Stack { SceneNode* node; int index; };
	struct Item { SceneNode* node; Drawable* drawable; };
	std::vector<Stack> stack;
	std::vector<Item> items;
	RenderState state;
	state.setCamera(m_camera);

	Point size = base::Game::getSize();
	if(!m_selectionBuffer || m_selectionBuffer->width()!=size.x || m_selectionBuffer->height()!=size.y) {
		delete m_selectionBuffer;
		m_selectionBuffer = new base::FrameBuffer(size.x, size.y, Texture::NONE, Texture::D24S8);
	}

	m_selectionBuffer->bind();
	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	Item selected{0,0};
	unsigned depth = 0xffffff;

	auto readData = [&]() {
		unsigned pixel;
		glReadPixels(pos.x, pos.y, 1, 1, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, &pixel);
		if(pixel&0xff && pixel>>8<=depth) {
			selected = items[(pixel&0xff) - 1];
			depth = pixel >> 8;
		}
		items.clear();
		glClear(GL_STENCIL_BUFFER_BIT);
	};
	
	stack.push_back({getSceneRoot(), 0});
	while(!stack.empty()) {
		SceneNode* n = stack.back().node->getChild(stack.back().index);
		if(!n) stack.pop_back();
		else if(!n->isVisible()) ++stack.back().index;
		else {
			// Render drawables to selection buffer
			for(auto& d: n->attachments()) {
				if(d->getMaterial() && !d->getMaterial()->getPass(0)->state.depthWrite) continue;

				items.push_back({n, d});
				glStencilFunc(GL_ALWAYS, items.size()&0xff, 0xff);
				state.getVariableSource()->setModelMatrix(d->getTransform());
				state.getVariableSource()->setCustom(d->getCustom());
				d->draw(state);
				if(items.size()==255) readData();
			}

			// Next
			++stack.back().index;
			if(n->getChildCount()) {
				stack.push_back({n, 0});
			}
		}
	}
	readData();
	base::FrameBuffer::Screen.bind();
	if(selected.node) {
		//float d = (float)depth / 0xffffff;
		return TraceResult{selected.node};
	}
	return TraceResult();
}


// --------------------------------- //

bool SceneEditor::canDrop(const Point& p, const Asset& asset) const {
	Widget* target = m_gui->getRootWidget()->getWidget(p);
	if(target) {
		Point localPos = target->getDerivedTransform().untransform(p);
		for(EditorComponent* c: m_components) {
			if(c->drop(target, localPos, asset, false)) return true;
		}
	}
	return false;
}

void SceneEditor::drop(const Point& p, const Asset& asset) {
	Widget* target = m_gui->getRootWidget()->getWidget(p);
	if(target) {
		Point localPos = target->getDerivedTransform().untransform(p);
		for(EditorComponent* c: m_components) {
			if(c->drop(target, localPos, asset, true)) return;
		}
	}
	if(!m_gui->getRootWidget()->getWidget(p)) {
		for(auto& func: m_construct) {
			if(func(asset)) return;
		}
	}
}

SceneNode* SceneEditor::constructObject(const Asset& asset) {
	// Reversed so user handlers will execute first
	for(int i=m_construct.size() - 1; i>=0; --i) {
		if(SceneNode* r = m_construct[i](asset)) return r;
	}
	return nullptr;
}

