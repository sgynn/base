#include <base/editor/layout.h>
#include <base/editor/gizmo.h>
#include <base/scene.h>
#include <base/model.h>
#include <base/mesh.h>
#include <base/skeleton.h>
#include <base/shader.h>
#include <base/material.h>
#include <base/drawablemesh.h>
#include <base/resources.h>
#include <base/gui/tree.h>
#include <base/gui/widgets.h>
#include <base/gui/layouts.h>
#include <base/mesh.h>
#include <base/game.h>
#include <base/input.h>
#include <unordered_set>

#include <base/editor/embed.h>
BINDATA(editor_layout_gui, EDITOR_DATA "/layout.xml")

using base::SceneNode;
using base::Drawable;
using base::DrawableMesh;
using namespace gui;
using namespace editor;

std::vector<SceneNodeType*> LayoutViewer::s_nodeTypes;
std::vector<DrawableType*> LayoutViewer::s_drawableTypes;

struct ItemData {
	SceneNode* node = 0;
	Drawable* drawable = 0;
	SceneNodeType* nodeType = 0;
	DrawableType* drawableType = 0;
};
inline ItemData getItemData(TreeNode* node) {
	ItemData data;
	data.drawable = node->getData().getValue<Drawable*>(0);
	if(data.drawable) {
		data.drawableType = node->getData(1).getValue<DrawableType*>(0);
		node = node->getParent();
	}
	data.node = node->getData().getValue<SceneNode*>(0);
	data.nodeType = node->getData(1).getValue<SceneNodeType*>(0);
	return data;
}

// Gizmo node to detect if scene is destroyed
class SceneListenerNode : public SceneNode {
	LayoutViewer* m_target;
	public:
	SceneListenerNode(LayoutViewer* v) : SceneNode("Listener"), m_target(v) {}
	~SceneListenerNode() { m_target->sceneDestroyed(); }
};


LayoutViewer::LayoutViewer() {
}

void LayoutViewer::initialise() {
	m_panel = loadUI("layout.xml", editor_layout_gui);
	addToggleButton(m_panel, "editors", "scene");

	m_filter = m_panel->getWidget<Textbox>("filter");
	m_clearFilter = m_panel->getWidget<Button>("clearfilter");
	m_properties = m_panel->getWidget("details");
	m_gizmoWidget = m_panel->getWidget("gizmo");
	m_tree = m_panel->getWidget<TreeView>("tree");
	m_tree->eventCustom.bind(this, &LayoutViewer::itemEvent);
	m_tree->eventSelected.bind(this, &LayoutViewer::itemSelected);
	m_tree->eventCacheItem.bind(this, &LayoutViewer::refreshItem);
	m_filter->eventChanged.bind(this, &LayoutViewer::filterChanged);
	m_clearFilter->eventPressed.bind( [this](Button*) { m_filter->setText(""); refresh(); });

	m_gizmoWidget->getWidget<Button>("move")->eventPressed.bind([this](Button*){setGizmoMode(GizmoMode::Move);});
	m_gizmoWidget->getWidget<Button>("rotate")->eventPressed.bind([this](Button*){setGizmoMode(GizmoMode::Rotate);});
	m_gizmoWidget->getWidget<Button>("scale")->eventPressed.bind([this](Button*){setGizmoMode(GizmoMode::Scale);});
	m_gizmoWidget->getWidget<Button>("global")->eventPressed.bind([this](Button*){setGizmoSpace(GizmoSpace::Global);});
	m_gizmoWidget->getWidget<Button>("parent")->eventPressed.bind([this](Button*){setGizmoSpace(GizmoSpace::Parent);});
	m_gizmoWidget->getWidget<Button>("local")->eventPressed.bind([this](Button*){setGizmoSpace(GizmoSpace::Local);});

	// Stick gizmo widget here so it doesn't get lost
	m_gizmoWidget->removeFromParent();
	m_properties->deleteChildWidgets();
	m_properties->add(m_gizmoWidget);
	m_gizmoWidget->setVisible(false);

	// Setup internal types - These go at the end
	static bool typesInitialised = false;
	if(!typesInitialised) {
		typesInitialised = true;
		s_nodeTypes.push_back(new SceneNodeType());
		s_drawableTypes.push_back(new DrawableMeshType());
		s_drawableTypes.push_back(new DrawableType());
	}
}

LayoutViewer::~LayoutViewer() {
	destroyGizmo();
	delete m_listener;
}

void LayoutViewer::registerType(SceneNodeType* type) {
	s_nodeTypes.insert(s_nodeTypes.begin(), type);
}
void LayoutViewer::registerType(DrawableType* type) {
	s_drawableTypes.insert(s_drawableTypes.begin(), type);
}
SceneNodeType* LayoutViewer::getType(SceneNode* value) const {
	for(SceneNodeType* type: s_nodeTypes) if(type->match(value)) return type;
	return 0;
}
DrawableType* LayoutViewer::getType(Drawable* value) const {
	for(DrawableType* type: s_drawableTypes) if(type->match(value)) return type;
	return 0;
}

void LayoutViewer::setRoot(SceneNode* n) {
	TreeNode* tree = m_tree->getRootNode();
	if(tree->getData() != n) {
		tree->clear();
		tree->setValue(n);
		tree->refresh();
	}
	refresh();
}

void LayoutViewer::setGizmoRenderQueue(int q) {
	m_gizmoQueue = q;
	if(m_gizmo) m_gizmo->setRenderQueue(q);
}

bool LayoutViewer::isActive() const {
	return m_panel->isVisible();
}

void LayoutViewer::activate() {
	m_panel->setVisible(true);
	if(m_active) selectItem(m_active);
}

void LayoutViewer::deactivate() {
	itemSelected(0,0);
	destroyGizmo();
}

void LayoutViewer::cancel() {
	destroyGizmo();
}

TreeNode* findNode(TreeNode* node, const Any& data) {
	if(node->getData() == data) return node;
	for(TreeNode* n: *node) {
		TreeNode* f = findNode(n, data);
		if(f) return f;
	}
	return 0;
}
static void findCollapsedNodes(TreeNode* node, std::unordered_set<void*>& out) {
	for(TreeNode* n : *node) {
		if(n->size() && !n->isExpanded()) {
			ItemData data = getItemData(n);
			out.insert(data.drawable? (void*)data.drawable : (void*)data.node);
		}
		findCollapsedNodes(n, out);
	}
}
static void expandChildren(TreeNode* node, const std::unordered_set<void*>& collapsed) {
	for(TreeNode* n : *node) {
		if(n->size()) {
			ItemData data = getItemData(n);
			n->expand(collapsed.find(data.node) == collapsed.end());
			expandChildren(n, collapsed);
		}
	}
}

void LayoutViewer::refresh() {
	TreeNode* tree = m_tree->getRootNode();
	SceneNode* root = getEditor()->getSceneRoot(); // tree->getData().getValue<SceneNode*>(0);
	if(root) {
		if(!m_listener) m_listener = new SceneListenerNode(this);
		root->addChild(m_listener);

		Any selected;
		std::unordered_set<void*> collapsed;
		findCollapsedNodes(tree, collapsed);
		if(m_tree->getSelectedNode()) selected = m_tree->getSelectedNode()->getData();

		tree->clear();
		populate(tree, root, m_filter->getText());
		tree->expand(true);
		expandChildren(tree, collapsed);
		if(!selected.isNull()) {
			if(TreeNode* node = findNode(tree, selected)) {
				m_tree->scrollToItem(node);
				node->select();
			}
		}
		m_clearFilter->setEnabled(m_filter->getText()[0]);
		m_lastNodeCount = root->getChildCount();
	}
}

void LayoutViewer::populate(TreeNode* node, SceneNode* value, const char* filter) {
	node->setValue(value);
	SceneNodeType* type = getType(value);
	node->setValue(1, type);

	if(value == m_active && !m_drawable) node->select();
	if(!type || type->showAttachments()) {
		for(Drawable* d: value->attachments()) {
			if(!filterItem(d, filter)) continue;
			TreeNode* n = node->add(new TreeNode());
			if(m_drawable==d) n->select();
			n->setValue(d);
			n->setValue(1, getType(d));
		}
	}
	if(!type || type->showChildren()) {
		for(SceneNode* n: value->children()) {
			if(n == m_listener) continue;
			if(!filterItem(n, filter)) continue;
			TreeNode* item = node->add(new TreeNode());
			populate(item, n, filter);
		}
	}
}

bool LayoutViewer::filterItem(SceneNode* node, const char* filter) const {
	if(!filter || !filter[0]) return true;
	const char* name; int i; bool b;
	SceneNodeType* type = getType(node);
	if(!type) return false;
	type->getValues(node, name, i, b);
	if(strstr(name, filter)) return true;
	if(type->showAttachments()) {
		for(Drawable* d: node->attachments()) if(filterItem(d, filter)) return true;
	}
	if(type->showChildren()) {
		for(SceneNode* n: node->children()) if(filterItem(n, filter)) return true;
	}
	return false;
}

bool LayoutViewer::filterItem(Drawable* d, const char* filter) const {
	if(!filter || !filter[0]) return true;
	const char* name; int i; bool b;
	DrawableType* type = getType(d);
	if(!type) return false;
	type->getValues(d, name, i, b);
	return strstr(name, filter);
}

void LayoutViewer::filterChanged(Textbox*, const char* filter) {
	refresh();
}



void LayoutViewer::selectItem(SceneNode* n) {
	TreeNode* node = findNode(m_tree->getRootNode(), Any(n));
	if(node) {
		// Select mesh if there is only one ?
		if(n->getAttachmentCount() == 1 && node->size()>0) {
			node = node->at(0);
		}

		node->select();
		node->ensureVisible();
		m_tree->scrollToItem(node);
	}
	itemSelected(m_tree, node);
}


void LayoutViewer::refreshItem(gui::TreeNode* n, gui::Widget* w) {
	const char* name = "ERROR";
	bool visible = true;
	int icon = 0;
	bool isNode = n->getData().isType<SceneNode*>();

	if(isNode) {
		SceneNode* node = n->getData(0).getValue<SceneNode*>(0);
		SceneNodeType* type = n->getData(1).getValue<SceneNodeType*>(0);
		if(type) type->getValues(node, name, icon, visible);
		if(!name[0]) name = "Scene Node";
	}
	else {
		icon = 1; // default icon
		Drawable* drawable = n->getData(0).getValue<Drawable*>(0);
		DrawableType* type = n->getData(1).getValue<DrawableType*>(0);
		if(type) type->getValues(drawable, name, icon, visible);
	}

	w->getWidget(0)->as<Image>()->setImage(icon);
	w->getWidget(1)->as<Label>()->setCaption(name);
	w->getWidget(2)->as<Checkbox>()->setChecked(visible);
	w->getWidget(2)->setVisible(isNode);
}

void LayoutViewer::itemEvent(gui::TreeView*, gui::TreeNode* n, gui::Widget* w) {
	bool vis = cast<Checkbox>(w)->isChecked();
	SceneNode* node = n->getData().getValue<SceneNode*>(0);
	if(node) node->setVisible(vis);
	Drawable* drawable = n->getData().getValue<Drawable*>(0);
	if(drawable) drawable->setVisible(vis);
}

void LayoutViewer::itemSelected(gui::TreeView*, gui::TreeNode* n) {
	if(!m_properties) return;
	m_gizmoWidget->removeFromParent();
	m_properties->deleteChildWidgets();
	m_properties->add(m_gizmoWidget);
	m_gizmoWidget->setVisible(false);

	if(!n) {
		destroyGizmo();
		m_active = 0;
		m_drawable = 0;
		m_activeType = 0;
		return;
	}

	LayoutProperties panel(m_properties);
	ItemData item = getItemData(n);
	m_active = item.node;
	m_activeType = item.nodeType;
	m_drawable = item.drawable;
	if(!m_activeType) return;
	if(item.nodeType) item.nodeType->showProperties(panel, m_active);
	if(item.drawableType) item.drawableType->showProperties(panel, m_active, m_drawable);

	createGizmo();
	SceneNode* parent = m_active->getParent();
	m_gizmo->setPosition( m_active->getPosition() );
	m_gizmo->setOrientation( m_active->getOrientation() );
	m_gizmo->setScale( m_active->getScale() );
	m_gizmo->setBasis( parent? parent->getDerivedTransform(): Matrix() );
	setGizmoSpace(m_gizmoSpace);
}

void LayoutViewer::createGizmo() {
	if(m_gizmo || !getEditor()->getSceneRoot()) return;
	m_gizmo = new Gizmo;
	m_gizmo->setRenderQueue(m_gizmoQueue);
	getEditor()->getSceneRoot()->attach(m_gizmo);
	setGizmoMode(GizmoMode::Move);
	setGizmoSpace(GizmoSpace::Global);
}
void LayoutViewer::destroyGizmo() {
	if(!m_gizmo) return;
	getEditor()->getSceneRoot()->detach(m_gizmo);
	delete m_gizmo;
	m_gizmo = 0;
}

void LayoutViewer::setGizmoMode(GizmoMode mode) {
	if(m_gizmo) m_gizmo->setMode((Gizmo::Mode)mode);
	if(m_gizmoWidget) {
		m_gizmoWidget->getWidget("move")->setSelected(mode == GizmoMode::Move);
		m_gizmoWidget->getWidget("rotate")->setSelected(mode == GizmoMode::Rotate);
		m_gizmoWidget->getWidget("scale")->setSelected(mode == GizmoMode::Scale);
	}
}
void LayoutViewer::setGizmoSpace(GizmoSpace space) {
	m_gizmoSpace = space;
	if(m_gizmo) {
		switch(space) {
		case GizmoSpace::Global: m_gizmo->setRelative(Quaternion()); break;
		case GizmoSpace::Local: m_gizmo->setLocalMode(); break;
		case GizmoSpace::Parent: 
			if(!m_active->getParent()) m_gizmo->setRelative(Quaternion());
			else m_gizmo->setRelative(m_active->getParent()->getDerivedTransform());
			break;
		}
	}
	if(m_gizmoWidget) {
		m_gizmoWidget->getWidget("global")->setSelected(space == GizmoSpace::Global);
		m_gizmoWidget->getWidget("parent")->setSelected(space == GizmoSpace::Parent);
		m_gizmoWidget->getWidget("local")->setSelected(space == GizmoSpace::Local);
	}
}


void LayoutViewer::sceneDestroyed() {
	m_tree->getRootNode()->clear();
	destroyGizmo();
	m_listener = 0;
}

void LayoutViewer::update() {
	if(!m_panel->isVisible() || !getEditor()->getSceneRoot()) return;
	if(getEditor()->getSceneRoot()->getChildCount() != m_lastNodeCount) {
		refresh();
	}
}

bool LayoutViewer::mouseEvent(const MouseEventData& event) {
	if(!m_panel->isVisible() || !getEditor()->getSceneRoot()) return false;

	if(m_activeType) {
		m_activeType->updateTransform(m_properties);

		// Update gizmo
		if(m_gizmo) {
			bool moved = event.mouse.delta.x || event.mouse.delta.y;
			MouseRay ray(getEditor()->getCamera(), event.mouse.x, event.mouse.y, base::Game::width(), base::Game::height());
			if(!event.overGUI && event.mouse.pressed==1) {
				m_gizmo->onMouseDown(ray);
			}
			else if(event.mouse.released & 1) {
				m_gizmo->onMouseUp();
			}
			else if(moved && (!event.overGUI || m_gizmo->isHeld())) {
				m_gizmo->onMouseMove(ray);
			}
			
			if(!m_gizmo->isHeld()) {
				m_gizmo->setPosition( m_active->getPosition() );
				m_gizmo->setOrientation( m_active->getOrientation() );
				m_gizmo->setScale( m_active->getScale() );
			}
			else if(moved) {
				m_active->setPosition( m_gizmo->getPosition() );
				m_active->setOrientation( m_gizmo->getOrientation() );
				m_active->setScale( m_gizmo->getScale() );
			}

			if(m_gizmo->isHeld()) {
				promoteEventHandler();
				return true;
			}
		}
	}

	// Picking
	if(!event.overGUI && (!m_gizmo || !m_gizmo->isHeld()) && event.mouse.pressed==1) {
		if(auto result = getEditor()->trace(event.mouse)) {
			selectItem(result.node);
			getEditor()->cancelActiveTools(this);
		}
	}

	return false;

}


// ======================================================================== //

LayoutCategory LayoutCategory::addRow() {
	Widget* row = new Widget();
	row->setSize(100, 20);
	row->setLayout( new HorizontalLayout(0, 2) );
	row->setAnchor("lr");
	row->setAutosize(true);
	add(row);
	return LayoutCategory(row);
}

Label* LayoutCategory::addLabel(const char* text, int size) {
	Label* l = createWidget<Label>("label");
	if(size) l->setFontSize(size);
	l->setCaption(text);
	l->setAutosize(true);
	m_panel->add(l);
	return l;
}

LayoutCategory LayoutProperties::getCategory(const char* name) {
	CollapsePane* w = m_panel->getWidget<CollapsePane>("name");
	if(!w) {
		w = m_panel->getRoot()->createWidget<CollapsePane>("category");
		w->setLayout( new VerticalLayout() );
		w->setAnchor("lr");
		w->setAutosize(true);
		w->setCaption(name);
		w->setName(name);
		m_panel->add(w);
	}
	return LayoutCategory(w);
}

void LayoutProperties::addTransform(SceneNode* node) {
	const char* captions[] = { "Position", "Orientation", "Scale" };
	const float* values[] = { node->getPosition(), node->getOrientation(), node->getScale() };
	const int elements[] = { 3,4,3 };
	const char* names = "pos1234";
	char name[3] = {0,0,0};

	LayoutCategory cat = getCategory("Transform");
	if(Widget* gizmoWidget = m_panel->getWidget("gizmo")) {
		cat.add(gizmoWidget);
		gizmoWidget->setVisible(true);
	}
	for(int i=0; i<3; ++i) {
		cat.addLabel(captions[i], 12);
		name[0] = names[i];

		Widget* container = new Widget();
		container->setSize(100, 20);
		for(int j=0; j<elements[i]; ++j) {
			name[1] = names[j+3];
			SpinboxFloat* box = cat.createWidget<SpinboxFloat>("spinboxf");
			box->setValue( values[i][j] );
			box->setAnchor("lr");
			box->setName(name);
			box->eventChanged.bind([node, value=&values[i][j]](SpinboxFloat*, float v) { *const_cast<float*>(value)=v; node->notifyChange(); });
			container->add(box);
		}
		container->setLayout( new HorizontalLayout(0, 2) );
		container->setAnchor("lr");
		cat.add(container);
	}
}
void LayoutProperties::addMaterial(base::Material* mat) {
	base::Resources* res = base::Resources::getInstance();
	LayoutCategory cat = getCategory("Material");
	auto fix = [](const void* p, const char* s) { return p? s? s: "Unknown": "None"; };

	cat.addLabel(String("Material: ") + fix(mat, res? res->materials.getName(mat): 0));
	if(!mat) return;
	base::Pass* pass = mat->getPass(0);

	// Shader
	cat.addLabel(String("Shader: ") + fix(pass->getShader(), res?res->shaders.getName(pass->getShader()):0));

	// Textures
	if(pass->getTextureCount()) {
		cat.addLabel("Textures", 12);
		for(uint i=0; i<pass->getTextureCount(); ++i) {
			const base::Texture* tex = pass->getTexture(i);
			cat.addLabel(String("  ") + fix(tex, res?res->textures.getName(tex):0));
		}
	}

	// Variables
	if(pass->getShader()) {
		cat.addLabel("Variables", 12);
		base::Shader::UniformList vars = pass->getShader()->getUniforms();
		for(auto& v: vars) {
			cat.addLabel(String("  ") + v.name);
		}
	}
}


// ======================================================================== //

bool SceneNodeType::match(SceneNode*) const { return true; }
void SceneNodeType::getValues(SceneNode* node, const char*& name, int& icon, bool& visible) const {
	visible = node->isVisible();
	name = node->getName();
	icon = 0;
}
void SceneNodeType::setVisible(SceneNode* node, bool vis) const { node->setVisible(vis); }
void SceneNodeType::showProperties(LayoutProperties& panel, SceneNode* node) {
	m_node = node;
	panel.addTransform(node);
}

void SceneNodeType::updateTransform(Widget* panel) {
	auto set = [root=panel->getRoot()](const char* name, float value) {
		if(SpinboxFloat* box = root->getWidget<SpinboxFloat>(name)) {
			if(!box->hasFocus()) box->setValue(value);
		}
	};

	if(m_node->getPosition()!=m_pos) {
		m_pos = m_node->getPosition();
		set("p1", m_pos.x);
		set("p2", m_pos.y);
		set("p3", m_pos.z);
	}

	if(m_node->getOrientation() != m_rot) {
		m_rot = m_node->getOrientation();
		set("o1", m_rot.w);
		set("o2", m_rot.x);
		set("o3", m_rot.y);
		set("o4", m_rot.z);
	}

	if(m_node->getScale() != m_scale) {
		m_scale = m_node->getScale();
		set("s1", m_scale.x);
		set("s2", m_scale.y);
		set("s3", m_scale.z);
	}
}

// ======================================================================== //

bool DrawableType::match(Drawable*) const { return true; }
void DrawableType::getValues(Drawable* d, const char*& name, int& icon, bool& visible) const {
	visible = d->isVisible();
	name = "Drawable";
	icon = 1;
}
void DrawableType::setVisible(Drawable* d, bool vis) const { d->setVisible(vis); }
void DrawableType::showProperties(LayoutProperties& panel, SceneNode*, Drawable* d) { 
	panel.addMaterial(d->getMaterial());
}


// ======================================================================== //

bool DrawableMeshType::match(Drawable* d) const { return dynamic_cast<DrawableMesh*>(d); }
void DrawableMeshType::getValues(Drawable* d, const char*& name, int& icon, bool& visible) const {
	DrawableMesh* drawable = static_cast<DrawableMesh*>(d);
	visible = d->isVisible();
	if(!drawable->getMesh()) name = "Null Mesh";
	else if(drawable->getInstanceBuffer()) name = "Instanced mesh";
	else if(drawable->getMesh()->getWeightsPerVertex()) name = "Skinned Mesh";
	else name = "Mesh";
	icon = 1;
}
void DrawableMeshType::showProperties(LayoutProperties& panel, SceneNode*, Drawable* d) {
	LayoutCategory cat = panel.getCategory("Mesh");
	base::Mesh* mesh = static_cast<DrawableMesh*>(d)->getMesh();
	base::Skeleton* skeleton = static_cast<DrawableMesh*>(d)->getSkeleton();
	const char* modelName = "Unknown";
	const char* meshName = 0;

	// Find model
	if(base::Resources::getInstance()) {
		for(auto model: base::Resources::getInstance()->models) {
			if(model.value) for(auto& m: model.value->meshes()) {
				if(m.mesh == mesh) {
					modelName = model.key;
					meshName = m.name;
					goto found;
				}
			}
		}
	}
	found:

	cat.addLabel(String("Model: ") + modelName);
	if(meshName) cat.addLabel(String("Mesh: ") + meshName);

	if(skeleton) {
		char buffer[32];
		sprintf(buffer, "Skin: %d weights", (int)mesh->getWeightsPerVertex());
		cat.addLabel(buffer);
		sprintf(buffer, "Skeleton: %u bones", skeleton->getBoneCount());
		cat.addLabel(buffer);
	}

	panel.addMaterial(d->getMaterial());
}



