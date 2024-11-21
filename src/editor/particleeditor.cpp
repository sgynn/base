#include <base/editor/particleeditor.h>
#include <base/particledef.h>
#include <base/gui/renderer.h>
#include <base/gui/layouts.h>
#include <base/gui/widgets.h>
#include <base/gui/lists.h>
#include <base/gui/colourpicker.h>
#include <base/gui/menubuilder.h>
#include <base/resources.h>
#include <base/script.h>
#include <base/file.h>
#include <base/virtualfilesystem.h>
#include <base/editor/editor.h>
#include <base/editor/nodeeditor.h>
#include <base/editor/gradientbox.h>
#include <base/editor/graphbox.h>
#include <list>
#include <map>


#include <base/editor/embed.h>
BINDATA(editor_particle_gui, EDITOR_DATA "/particles.xml")

#define CONNECT_SOURCE m_panel 
#define CONNECT_CLASS  ParticleEditor
#define CONNECT(Type, name, event, ...) { Type* w = CONNECT_SOURCE->getWidget<Type>(name); if(w) w->event.bind(this, &CONNECT_CLASS::__VA_ARGS__); }
#define CONNECT_I(Type, name, event, ...) { Type* w = CONNECT_SOURCE->getWidget<Type>(name); if(w) w->event.bind(__VA_ARGS__); }
#define CONNECT_SUB(Type, name, subname, event, ...) { Widget* w = CONNECT_SOURCE->getWidget(name); if(w) { Type* s=w->getTemplateWidget<Type>(subname); if(s) { s->event.bind(this, &CONNECT_CLASS::__VA_ARGS__);} } }
#define CONNECT_SUB_I(Type, name, subname, event, ...) { Widget* w = CONNECT_SOURCE->getWidget(name); if(w) { w=w->getTemplateWidget<Type>(subname); if(w) {  w->event.bind(__VA_ARGS__);} } }

using namespace gui;
using namespace editor;
using script::Variable;


// Graph nodes
namespace editor {
class ParticleNode : public nodegraph::Node {
	WIDGET_TYPE(ParticleNode)
	void destroyData() {
		switch(m_type) {
		case ParticleEditor::EmitterNode:    delete (particle::Emitter*)m_data; break;
		case ParticleEditor::AffectorNode:   delete (particle::Affector*)m_data; break;
		case ParticleEditor::RenderDataNode: delete (particle::RenderData*)m_data; break;
		case ParticleEditor::EventNode:      delete (particle::Event*)m_data;  break;
		case ParticleEditor::SystemNode:     break;
		}
	}
	void setData(ParticleEditor::NodeType type, const char* className, particle::Object* data) {
		m_type = type;
		m_class = className;
		m_data = data;
		m_canBeDeleted = type != ParticleEditor::SystemNode;
	}
	const char* getNodeName() const { return m_class; }
	ParticleEditor::NodeType getNodeType() const { return m_type; }
	particle::Object* getNodeData() const { return m_data; }
	protected:
	ParticleEditor::NodeType m_type;
	String m_class;
	particle::Object* m_data = 0;
};
}

// ================================================================================================ //

void ParticleEditorComponent::initialise() {
	Root::registerClass<ParticleNode>();
	Root::registerClass<GradientBox>();
	particle::registerInternalStructures(); // In case this was not done yet

	m_panel = loadUI("particles.xml", editor_particle_gui);
	if(m_panel) m_panel->setVisible(false);
}

void ParticleEditorComponent::setParticleManager(particle::Manager* manager) {
	m_manager = manager;
}

bool ParticleEditorComponent::newAsset(const char*& name, const char*& file, const char*& body) const {
	name = "Particles";
	file = "particles.pt";
	body = "system = { emitters = [] }";
	return true;
}

bool ParticleEditorComponent::assetActions(MenuBuilder& menu, const Asset& asset) {
	if(!asset.file.name.endsWith(".pt")) return false;

	menu.addAction("Save", [this, &asset]() {
		particle::System* system = base::Resources::getInstance()->particles.get(asset.resource? asset.resource: asset.file.name);
		if(system) {
			ParticleEditor::save(system, asset.file.getFullPath());
			for(ParticleEditor* e: m_editors) if(e->m_system==system) e->m_modified = false;
			getEditor()->assetChanged(asset, false);
		}
	});

	return true;
}

Widget* ParticleEditorComponent::openAsset(const Asset& asset) {
	if(!asset.file.name.endsWith(".pt")) return nullptr;
	auto& res = base::Resources::getInstance()->particles;
	const char* name = asset.resource? asset.resource: asset.file.name;
	particle::System* system = res.get(name);
	ParticleEditor* editor = showParticleSystem(system, name);
	return editor? editor->getPanel(): nullptr;
}

ParticleEditor* ParticleEditorComponent::showParticleSystem(particle::System* system, const char* name) {
	if(!system) return nullptr;

	// Already open
	for(ParticleEditor* e: m_editors) {
		if(e->getParticleSystem() == system) {
			e->getPanel()->raise();
			return e;
		}
	}

	// Spawn a new editor
	Widget* panel = m_panel->clone();
	panel->setVisible(true);
	getEditor()->getGUI()->getRootWidget()->add(panel);
	ParticleEditor* editor = new ParticleEditor(this, system, panel, name);
	m_editors.push_back(editor);

	// Cascade window
	bool overlap = true;
	while(overlap) {
		overlap = false;
		for(Widget* w: getEditor()->getGUI()->getRootWidget()) {
			if(w != panel && w->getPosition() == panel->getPosition()) {
				overlap = true;
				break;
			}
		}
		if(overlap) panel->setPosition(panel->getPosition() + 16);
	}
	return editor;
}

void ParticleEditorComponent::saveAll() {
	base::Resources& res = *base::Resources::getInstance();
	for(ParticleEditor* e: m_editors) {
		if(e->isModified()) {
			const char* name = res.particles.getName(e->getParticleSystem());
			if(name) {
				const base::VirtualFileSystem::File& file = res.getFileSystem().getFile(name);
				e->save(e->getParticleSystem(), file.getFullPath());
				printf("Saved %s\n", file.getFullPath().str());
				e->m_modified = false;
				getEditor()->assetChanged(Asset{ResourceType::Particle, name, file}, false);
			}
			else printf("Error: Particle system not in resourve manager\n");
		}
	}
}

bool ParticleEditorComponent::drop(Widget* w, const Point& p, const Asset& asset, bool apply) {
	if(!cast<Textbox>(w)) return false;
	for(Widget* parent = w; parent; parent=parent->getParent()) {
		if(cast<gui::Window>(parent)) {
			for(ParticleEditor* e: m_editors) {
				if(e->m_panel == parent) return e->drop(w,p,asset,apply);
			}
			return false;
		}
	}
	return false;
}


// ================================================================================================ //

inline script::String toString(const Variable& v) { return v.toString(); } 
template<class T> void replaceWidget(Widget* base, const char* name) {
	if(Widget* placeholder = base->getWidget(name)) {
		Widget* w = new T();
		w->setRect(placeholder->getRect());
		w->setName(name);
		w->setAnchor(placeholder->getAnchor());
		placeholder->getParent()->add(w, placeholder->getIndex());
		placeholder->removeFromParent();
		delete placeholder;
	}
}

ParticleEditor::ParticleEditor(ParticleEditorComponent* parent, particle::System* sys, Widget* panel, const char* name) 
	: m_parent(parent)
	, m_panel(panel)
	, m_manager(parent->getParticleManager())
	, m_system(sys)
{
	auto insertItemSorted = [](Listbox* list, const char* item, NodeType type) {
		for(uint i=0; i<list->getItemCount(); ++i) {
			if(list->getItem(i).getValue(1, type) != type) continue;
			if(strcmp(list->getItem(i), item) > 0) {
				list->insertItem(i, item, type);
				return;
			}
		}
		return list->addItem(item, type);
	};


	// Populate types
	Listbox* affectors = m_panel->getWidget<Listbox>("affectors");
	for(auto& i: particle::s_affectorFactory) if(i.value->create) insertItemSorted(affectors, i.value->type, AffectorNode);

	Listbox* emitters = m_panel->getWidget<Listbox>("emitters");
	for(auto& i: particle::s_emitterFactory) if(i.value->create) insertItemSorted(emitters, i.value->type, EmitterNode);

	Listbox* renderers = m_panel->getWidget<Listbox>("renderers");
	for(auto& i: particle::s_renderDataFactory) if(i.value->create) insertItemSorted(renderers, i.value->type, RenderDataNode);
	for(auto& i: particle::s_eventFactory) if(i.value->create) insertItemSorted(renderers, i.value->type, EventNode);

	// Set up node graph
	Widget* graphContainer = m_panel->getWidget("graph");
	m_graph = new nodegraph::NodeEditor();
	m_graph->setSize(graphContainer->getSize());
	graphContainer->add(m_graph);
	m_graph->setAnchor("tlrb");
	graphContainer->eventMouseWheel.bind([](Widget* w, int delta) {
		ScaleBox* s = cast<ScaleBox>(w);
		Transform::Pos anchor = s->getDerivedTransform().untransformf(s->getRoot()->getMousePos());
		s->setScale(s->getScale() * (1+delta * 0.1));
		Transform::Pos result = s->getDerivedTransform().untransformf(s->getRoot()->getMousePos());
		Point shift(result.x - anchor.x, result.y - anchor.y);
		for(Widget* w: *w->getWidget(0)) w->setPosition(w->getPosition() + shift);
	});

	m_graph->eventLinked.bind(this, &ParticleEditor::linked);
	m_graph->eventUnlinked.bind(this, &ParticleEditor::unlinked);
	m_graph->eventDelete.bind(this, &ParticleEditor::deleteNode);


	CONNECT(Listbox, "emitters",  eventMouseMove, createNodeFromDrag);
	CONNECT(Listbox, "affectors", eventMouseMove, createNodeFromDrag);
	CONNECT(Listbox, "renderers", eventMouseMove, createNodeFromDrag);

	Popup::ButtonDelegate func;
	func.bind(this, &ParticleEditor::propertyModeChanged);
	m_modePopup = new Popup();
	m_modePopup->setSize(80,10);
	m_modePopup->addItem(m_panel->getRoot(), "button", "value",  "Value",  func);
	m_modePopup->addItem(m_panel->getRoot(), "button", "random", "Random", func);
	m_modePopup->addItem(m_panel->getRoot(), "button", "map",    "Mapped", func);
	m_modePopup->addItem(m_panel->getRoot(), "button", "graph",  "Graph",  func);


	m_gradientEditor = m_panel->getRoot()->getWidget("gradienteditor");
	m_graphEditor = m_panel->getRoot()->getWidget("grapheditor");
	replaceWidget<GradientBox>(m_gradientEditor, "gradient");
	replaceWidget<GraphBox>(m_graphEditor, "graph");
	m_gradientEditor->setVisible(false);
	m_graphEditor->setVisible(false);

	selectNode(nullptr);

	if(gui::Window* wnd = cast<gui::Window>(panel)) {
		wnd->eventClosed.bind([this](gui::Window* w) { delete this; });
	}

	#undef CONNECT_SOURCE
	#define CONNECT_SOURCE m_gradientEditor
	CONNECT_I(GradientBox, "gradient", eventRangeChanged, [this](float min, float max) {
		m_gradientEditor->getWidget<Label>("min")->setCaption(toString(min));
		m_gradientEditor->getWidget<Label>("max")->setCaption(toString(max));
	});
	CONNECT_I(GradientBox, "gradient", eventSelected, [this](GradientBox* g, int sel) {
		uint colour = sel>=0? g->gradient.data[sel].value: 0xffffffff;
		char buf[16]; sprintf(buf, "#%X", colour);
		m_gradientEditor->getWidget<Textbox>("colour")->setText(buf);
		m_gradientEditor->getWidget<SpinboxFloat>("key")->setValue(sel<0? 0.f: g->gradient.data[sel].key);
		m_gradientEditor->getWidget("swatch")->setColour(colour);
	});
	CONNECT_I(GradientBox, "gradient", eventChanged, [this](GradientBox* g, int sel) {
		m_gradientEditor->getWidget<SpinboxFloat>("key")->setValue(sel<0? 0.f: g->gradient.data[sel].key);
		applyGradient();
	});
	CONNECT_I(SpinboxFloat, "key", eventChanged, [this](SpinboxFloat* s, float v) {
		GradientBox* g = m_gradientEditor->getWidget<GradientBox>("gradient");
		g->setKey(g->selected, v);
		g->resetRange();
		applyGradient();
	});
	CONNECT_I(Textbox, "colour", eventSubmit, [this](Textbox* text) {
		GradientBox* g = m_gradientEditor->getWidget<GradientBox>("gradient");
		Widget* swatch = m_gradientEditor->getWidget("swatch");
		const char* t = text->getText();
		if(t[0]=='#') ++t;
		char* end;
		uint v = strtoull(t, &end, 16);
		if(strlen(t) <= 6) v |= 0xff000000;
		if(end > t) g->setColour(g->selected, v);
		else g->eventSelected(g, g->selected);
		swatch->setColourARGB(g->gradient.data[g->selected].value);
		g->setFocus();
		applyGradient();
	});
	CONNECT_I(Widget, "swatch", eventMouseDown, [this](Widget* w, const Point& p, int b) {
		Popup* popup = w->getRoot()->getWidget<Popup>("colourpopup");
		if(!popup) { // Should be created elsewhere
			ColourPicker* picker = new ColourPicker();
			picker->setSize(140, 100);
			picker->initialise(w->getRoot(), PropertyMap());
			picker->eventChanged.bind([this](const Colour& c) {
				GradientBox* g = m_gradientEditor->getWidget<GradientBox>("gradient");
				g->setColour(g->selected, c.toARGB());
				g->eventSelected(g, g->selected);
				applyGradient();
			});
			popup = new Popup();
			popup->setName("colourpopup");
			popup->setAutosize(true);
			popup->add(picker);
		}
		popup->getWidget(0)->as<ColourPicker>()->setColour(w->getColourARGB());
		popup->popup(w);
	});
	#undef CONNECT_SOURCE
	#define CONNECT_SOURCE m_graphEditor
	CONNECT_I(GraphBox, "graph", eventSelected, [this](GraphBox* g, int sel) {
		vec2 p = sel<0? vec2(0,0): vec2(g->graph.data[sel].key, g->graph.data[sel].value);
		m_graphEditor->getWidget<SpinboxFloat>("x")->setValue(p.x);
		m_graphEditor->getWidget<SpinboxFloat>("y")->setValue(p.y);
	});
	CONNECT_I(GraphBox, "graph", eventChanged, [this](GraphBox* g, int sel) {
		particle::Value v;
		v.setGraph(g->graph);
		g->eventSelected(g, sel);
		m_changeValueDelegate(particle::getValueAsVariable(v));
	});
	CONNECT_I(SpinboxFloat, "x", eventChanged, [this](SpinboxFloat*, float v) {
		vec2 p(v, m_graphEditor->getWidget<SpinboxFloat>("y")->getValue());
		GraphBox* g = m_graphEditor->getWidget<GraphBox>("graph");
		g->setKey(g->selected, p);
		g->resetRange();
	});
	CONNECT_I(SpinboxFloat, "y", eventChanged, [this](SpinboxFloat*, float v) {
		vec2 p(m_graphEditor->getWidget<SpinboxFloat>("x")->getValue(), v);
		GraphBox* g = m_graphEditor->getWidget<GraphBox>("graph");
		g->setKey(g->selected, p);
		g->resetRange();
	});

	setParticleSystem(sys, name);
}

ParticleEditor::~ParticleEditor() {
	m_graphEditor->setVisible(false);
	m_gradientEditor->setVisible(false);
	m_panel->removeFromParent();
	delete m_panel;
	for(size_t i=0; i<m_parent->m_editors.size(); ++i) {
		if(m_parent->m_editors[i] == this) {
			m_parent->m_editors[i] = m_parent->m_editors.back();
			m_parent->m_editors.pop_back();
			break;
		}
	}
}

bool ParticleEditor::drop(Widget* w, const Point& p, const Asset& asset, bool apply) {
	Textbox* target = cast<Textbox>(w);
	const char* name = nullptr;
	if(asset.type == ResourceType::Texture) name = asset.resource;
	else if(asset.type == ResourceType::Material) name = asset.resource;
	else if(asset.type == ResourceType::Model) name = asset.resource;
	else if(asset.type == ResourceType::None) {
		base::Resources& res = *base::Resources::getInstance();
		base::StringView ext = strrchr(asset.file.name, '.');
		if(ext==".png" || ext==".dds") name = m_parent->getResourceNameFromFile(res.textures, asset.file.name);
		else if(ext==".bm" || ext==".obj") name = m_parent->getResourceNameFromFile(res.models, asset.file.name);
		else if(ext==".mat") name = m_parent->getResourceNameFromFile(res.materials, asset.file.name);
	}

	if(name && apply) {
		if(name[0]=='/') ++name;
		target->setText(name);
		target->eventSubmit(target);
	}
	return name;
}


// ------------------------------------------------------------------------------------ //


template<class T>
static const char* getParticleClassName(T* object) {
	static std::map<size_t, const char*> typeMap;
	if(typeMap.empty()) {
		for(auto& i: particle::s_emitterFactory) {
			if(!i.value->create) continue;
			auto* o = i.value->create();
			typeMap[typeid(*o).hash_code()] = i.key;
			delete o;
		}
		for(auto& i: particle::s_affectorFactory) {
			if(!i.value->create) continue;
			auto* o = i.value->create();
			typeMap[typeid(*o).hash_code()] = i.key;
			delete o;
		}
		for(auto& i: particle::s_renderDataFactory) {
			if(!i.value->create) continue;
			auto* o = i.value->create();
			typeMap[typeid(*o).hash_code()] = i.key;
			delete o;
		}
	}
	// Events are all one class so typeMap wont work
	if(particle::Event* e = dynamic_cast<particle::Event*>((particle::Object*)object)) {
		static const char* eventTypes[4] = {0,0,0,0};
		if(!eventTypes[0]) {
			for(auto& e: particle::s_eventFactory) {
				particle::Event* tmp = e.value->create();
				eventTypes[(int)tmp->getType()] = e.key;
				delete tmp;
			}
		}
		const char* typeName = eventTypes[(int)e->getType()];
		return typeName? typeName: "Event";
	}

	size_t id = typeid(*object).hash_code();
	auto it = typeMap.find(id);
	return it!=typeMap.end()? it->second: "";
}

void ParticleEditor::setParticleSystem(particle::System* system, const char* title) {
	if(!m_panel) return;
	if(gui::Window* w = cast<gui::Window>(m_panel)) {
		if(!title) w->setCaption("Particle Editor");
		else w->setCaption(String("Particle Editor - ") + title);
	}
	for(Widget* w: *m_graph) cast<ParticleNode>(w)->setData(NodeType::EmitterNode, 0, 0);
	m_graph->deleteChildWidgets();
	m_system = system;
	if(!system) return;

	Point p(20, 50);
	ParticleNode* systemNode = createGraphNode(SystemNode, "System", system);
	systemNode->setPosition(p);
	p.x += 100;

	for(particle::Emitter* e: system->emitters()) {
		if(e->eventOnly) continue;
		createOrLinkNode(systemNode, e, EmitterNode, p);
	}
}

void ParticleEditor::createOrLinkNode(ParticleNode* from, particle::Object* to, NodeType type, Point& pos) {
	// Find existing node
	for(Widget* w: m_graph) {
		if(ParticleNode* node = cast<ParticleNode>(w)) {
			if(node->getNodeData() == to) {
				m_graph->link(from, 0, node, 0);
				return;
			}
		}
	}


	// Create new node
	ParticleNode* target = nullptr;
	target = createGraphNode(type, getParticleClassName(to), to);
	target->setPosition(pos.x, pos.y);
	m_graph->link(from, 0, target, 0);
	
	// Child nodes
	pos.x += 180;
	switch(type) {
	case EmitterNode:
	{
		particle::Emitter* emitter = static_cast<particle::Emitter*>(to);
		if(emitter->getRenderer()) createOrLinkNode(target, emitter->getRenderer(), RenderDataNode, pos);
		for(particle::Affector* a: emitter->affectors()) {
			createOrLinkNode(target, a, AffectorNode, pos);
		}
		for(particle::Event* e: emitter->events()) {
			createOrLinkNode(target, e, EventNode, pos);
		}
		break;
	}
	case EventNode:
	{
		particle::Object* o = static_cast<particle::Event*>(to)->target;
		if(dynamic_cast<particle::Affector*>(o)) createOrLinkNode(target, o, AffectorNode, pos);
		else if(dynamic_cast<particle::Event*>(o)) createOrLinkNode(target, o, EventNode, pos);
		else if(dynamic_cast<particle::Emitter*>(o)) createOrLinkNode(target, o, EmitterNode, pos);
		break;
	}
	default: break;
	}
	pos.x -= 180;
	pos.y += 50;
}


// ------------------------------------------------------------------------------------ //


void ParticleEditor::createNodeFromDrag(Widget* w, const Point& mpos, int b) {
	if(m_graph && w && b==1 && cast<Listbox>(w)->getSelectedIndex()>=0) {
		Point pos = mpos +  w->getAbsolutePosition();
		if(m_graph->getAbsoluteRect().contains(pos)) {
			for(Widget* w: *m_graph) w->setSelected(false);
			const ListItem& item = *cast<Listbox>(w)->getSelectedItem();
			const char* className = item;
			NodeType type = item.findValue(SystemNode);
			particle::Object* object = 0;
			switch(type) {
			case RenderDataNode: object = particle::s_renderDataFactory[className]->create(); break;
			case AffectorNode: object = particle::s_affectorFactory[className]->create(); break;
			case EmitterNode: object = particle::s_emitterFactory[className]->create(); break;
			case EventNode: object = particle::s_eventFactory[className]->create(); break;
			case SystemNode: break;
			}
			if(object) {
				ParticleNode* node = createGraphNode(type, className, object);
				node->setSelected(true);
				node->forceStartDrag();
			}
			else {
				const char* types[] = { "SystemNode", "EmitterNode", "AffectorNode", "RenderDataNode", "EventNode" };
				printf("Failed to create %s node %s\n", types[type], className);
			}
			cast<Listbox>(w)->clearSelection();
		}
	}
}

ParticleNode* ParticleEditor::createGraphNode(NodeType type, const char* className, particle::Object* data) {
	ParticleNode* node = m_panel->getRoot()->createWidget<ParticleNode>("graphnode");
	if(!node) return 0;
	char caption[64];
	sprintf(caption, "  %s  ", className);
	m_graph->add(node);
	node->setData(type, className, data);
	node->setCaption(caption);
	node->setConnectorTemplates(m_panel->getRoot()->getTemplate("connector"), m_panel->getRoot()->getTemplate("connector"));
	node->eventPressed.bind(this, &ParticleEditor::selectNode);

	m_graph->setConnectorType(1, 0, 0xffffff, 6);
	m_graph->setConnectorType(2, 0, 0xffffff, 3);
	
	// Connectors
	switch(type) {
	case SystemNode:
		node->addOutput("", 0, false);	// emitter
		node->setColour(0xc080ff);
		break;
	case EmitterNode:
		node->addInput("", 0, false);	// system, event
		node->addOutput("", 1, false);	// affector, renderer, event
		node->setColour(0x40ff40);
		break;
	case RenderDataNode:
		node->addInput("", 2, false);	// emitter
		node->setColour(0xff4040);
		break;
	case AffectorNode:
		node->addInput("", 1, false);	// emitter, event
		node->setColour(0xff40a0ff);
		break;
	case EventNode:
		node->addInput("", 1, false);	// emitter, event
		node->addOutput("", 2, false);	// emitter, affector, event
		node->setColour(0xffffff00);
		break;
	}

	node->getTemplateWidget("_text")->setColour(node->getColourARGB());
	node->refreshLayout();
	return node;
}

// =================================================================================================== //

void ParticleEditor::notifyChange() {
	if(m_modified) return;
	m_modified = true;
	const char* resource = base::Resources::getInstance()->particles.getName(m_system);
	if(resource) {
		const base::VirtualFileSystem::File& file = base::Resources::getInstance()->getFileSystem().getFile(resource);
		m_parent->getEditor()->assetChanged({ ResourceType::Particle, resource, file }, true);
	}
}

void ParticleEditor::reinitialise() {
	if(!m_manager) {
		printf("Particle Manager not set - existing particle systems will not update\n");
	}
	else for(particle::Instance* i: *m_manager) {
		if(i->getSystem() == m_system) i->initialise();
	}
}

void ParticleEditor::deleteNode(nodegraph::Node* n) {
	cast<ParticleNode>(n)->destroyData();
	selectNode(nullptr);
}

inline bool isRendererReferenced(particle::System* sys, particle::RenderData* r) {
	for(particle::Emitter* e: sys->emitters()) if(e->getRenderer() == r) return true;
	return false;
}

inline particle::Emitter* getEmitter(Widget* node) {
	ParticleNode* n = cast<ParticleNode>(node);
	return n && n->getNodeType() == ParticleEditor::EmitterNode? static_cast<particle::Emitter*>(n->getNodeData()): nullptr;
}

void ParticleEditor::linked(const nodegraph::Link& link) {
	// If an event target is an emitter, make sure it is added to the system
	auto addEventTargetEmitters = [this](ParticleNode* eventNode) {
		bool added = false;
		for(const nodegraph::Link& link: eventNode->getLinks(nodegraph::OUTPUT)) {
			if(particle::Emitter* emitter = getEmitter(link.b)) {
				if(!emitter->getSystem()) {
					emitter->eventOnly = true;
					m_system->addEmitter(emitter);
					added = true;
				}
			}
		}
		if(added) reinitialise();
	};


	ParticleNode* a = cast<ParticleNode>(link.a);
	ParticleNode* b = cast<ParticleNode>(link.b);
	if(a->getNodeType() == EmitterNode && b->getNodeType() == AffectorNode) {
		((particle::Emitter*)a->getNodeData())->addAffector((particle::Affector*)b->getNodeData());
		notifyChange();
	}
	else if(a->getNodeType() == EmitterNode && b->getNodeType() == RenderDataNode) {
		// Unlink existing render node
		for(const nodegraph::Link& link : a->getLinks(nodegraph::OUTPUT, 0)) {
			if(link.b!=b && cast<ParticleNode>(link.b)->getNodeType() == RenderDataNode) {
				m_graph->unlink(link.a, link.ia, link.b, link.ib);
				break;
			}
		}

		((particle::Emitter*)a->getNodeData())->setRenderer((particle::RenderData*)b->getNodeData());
		if(!isRendererReferenced(m_system, (particle::RenderData*)b->getNodeData())) {
			m_system->addRenderer((particle::RenderData*)b->getNodeData());
			notifyChange();
		}
		reinitialise();
	}
	else if(a->getNodeType() == SystemNode && b->getNodeType() == EmitterNode) {
		particle::Emitter* emitter = getEmitter(b);
		emitter->eventOnly = false;
		m_system->addEmitter(emitter);
		notifyChange();
		reinitialise();
	}
	else if(a->getNodeType() == EmitterNode && b->getNodeType() == EventNode) {
		if(b->isLinked(nodegraph::OUTPUT, 0)) {
			getEmitter(a)->addEvent((particle::Event*)b->getNodeData());
			addEventTargetEmitters(b);
			notifyChange();
		}
	}
	else if(a->getNodeType() == EventNode) {
		particle::Event* event = (particle::Event*)a->getNodeData();
		event->target = (particle::Object*)b->getNodeData();
		// Add event to parent emitter if target is set
		for(const nodegraph::Link& link: a->getLinks(nodegraph::INPUT)) { // 0 or 1
			if(particle::Emitter* emitter = getEmitter(link.a)) emitter->addEvent(event);
			addEventTargetEmitters(a);
			notifyChange();
		}
	}
}

void ParticleEditor::unlinked(const nodegraph::Link& link) {
	ParticleNode* a = cast<ParticleNode>(link.a);
	ParticleNode* b = cast<ParticleNode>(link.b);
	if(a->getNodeType() == EmitterNode && b->getNodeType() == AffectorNode) {
		((particle::Emitter*)a->getNodeData())->removeAffector((particle::Affector*)b->getNodeData());
		notifyChange();
	}
	else if(a->getNodeType() == EmitterNode && b->getNodeType() == RenderDataNode) {
		particle::RenderData* data = (particle::RenderData*)b->getNodeData();
		((particle::Emitter*)a->getNodeData())->setRenderer(0);
		if(data->getSystem() && !isRendererReferenced(m_system, data)) {
			m_system->removeRenderer((particle::RenderData*)b->getNodeData());
		}
		notifyChange();
		reinitialise();
	}
	else if(a->getNodeType() == SystemNode && b->getNodeType() == EmitterNode) {
		particle::Emitter* emitter = getEmitter(b);
		emitter->eventOnly = true;
		notifyChange();
	}
	else if(a->getNodeType() == EmitterNode && b->getNodeType() == EventNode) {
		((particle::Emitter*)a->getNodeData())->removeEvent((particle::Event*)b->getNodeData());
		notifyChange();
	}
	else if(a->getNodeType() == EventNode) {
		for(const nodegraph::Link& link: a->getLinks(nodegraph::INPUT)) { // 0 or 1
			// Remove event if no target
			if(particle::Emitter* emitter = getEmitter(link.a)) {
				emitter->removeEvent((particle::Event*)a->getNodeData());
				notifyChange();
			}
		}
		((particle::Event*)a->getNodeData())->target = nullptr;
	}

	// Remove any floating emitters
	ParticleNode* systemNode = cast<ParticleNode>(m_graph->getWidget(0));
	assert(systemNode->getNodeType() == SystemNode);
	for(Widget* w: m_graph) {
		if(particle::Emitter* emitter = getEmitter(w)) {
			if(emitter->getSystem() && !m_graph->areNodesConnected(cast<ParticleNode>(w), systemNode)) {
				m_system->removeEmitter(emitter);
				reinitialise();
			}
		}
	}
}

// =================================================================================================== //

void ParticleEditor::showGradientEditor(GradientBox* target) {
	GradientBox* box = m_gradientEditor->getWidget<GradientBox>("gradient");
	const particle::Gradient& g = target->gradient;
	box->gradient = g;
	box->resetRange();
	box->createMarkers();
	box->editable = true;
	box->eventSelected(box, 0);
	m_gradientEditor->setVisible(true);
	m_gradientEditor->raise();
}

void ParticleEditor::applyGradient() {
	GradientBox* box = m_gradientEditor->getWidget<GradientBox>("gradient");
	Variable gradient = particle::getGradientAsVariable(box->gradient);
	m_changeValueDelegate(gradient);
}

void ParticleEditor::propertyModeChanged(Button* b) {
	int index = b->getIndex();
	m_modePopup->hide();
	m_changeTypeDelegate((ValueType)index);
}

void ParticleEditor::showGraphEditor(const Variable& var) {
	GraphBox* box = m_graphEditor->getWidget<GraphBox>("graph");
	box->graph = particle::loadValue(var).getGraph();
	box->resetRange();
	box->editable = true;
	m_graphEditor->setVisible(true);
	m_graphEditor->raise();
}


ParticleEditor::ValueType ParticleEditor::getValueType(const Variable& var) {
	if(var.isNumber()) return ValueType::Constant;
	else if(var.isVector()) return ValueType::Random;
	else if(var.isArray() && var.size()==2 && var.get(0u).isNumber()) return ValueType::Mapped;
	else if(var.isObject() || var.isArray()) return ValueType::Graph;
	return ValueType::Constant; // error - invalid data
}

template<class T>
inline Variable makeArray(const T& a, const T& b) {
	Variable r;
	r.makeArray();
	r.set(0u, a);
	r.set(1u, b);
	return r;
}

Variable ParticleEditor::convertValueType(ValueType newType, const Variable& data) {
	auto midVec = [](const vec2& v) { return (v.x+v.y)*0.5; };
	ValueType oldType = getValueType(data);
	switch(newType) {
	case ValueType::Constant:
		switch(oldType) {
		case ValueType::Constant: return data;
		case ValueType::Random: return midVec(data);
		case ValueType::Mapped: return ((float)data.get(0u) + (float)data.get(1u)) * 0.5f;
		case ValueType::Graph: return 0.f;
		}
	case ValueType::Random:
		switch(oldType) {
		case ValueType::Constant: return vec2((float)data);
		case ValueType::Random: return data;
		case ValueType::Mapped: return vec2(data.get(0u), data.get(1u));
		case ValueType::Graph: return vec2(0.f);
		}
	case ValueType::Mapped:
		switch(oldType) {
		case ValueType::Constant: return makeArray(data, data);
		case ValueType::Random: return makeArray(data.get("x"), data.get("y"));
		case ValueType::Mapped: return data;
		case ValueType::Graph: return makeArray(0,0);
		}
	case ValueType::Graph:
		switch(oldType) {
		case ValueType::Constant: return makeArray(vec2(0,data), vec2(0,data));
		case ValueType::Random:  return makeArray(vec2(0, midVec(data)), vec2(0,midVec(data)));
		case ValueType::Mapped: return makeArray(vec2(0, data.get(0u)), vec2(1, data.get(1u)));
		case ValueType::Graph: return data;
		};
	}
	return data;
}

template<class T> void refreshMaterial(particle::Manager*, T* item) {}
void refreshMaterial(particle::Manager* m, particle::RenderData* item) {
	if(!m) return;
	for(particle::Instance* i: *m) {
		if(i->getSystem() == item->getSystem()) i->createMaterial(item);
	}
}

template<class T>
void ParticleEditor::createPropertiesPanel(particle::Definition<T>* def, T* item) {
	Widget* panel = m_panel->getWidget("properties");
	panel->deleteChildWidgets();
	panel->pauseLayout();

	Label* label = panel->createChild<Label>("label");
	label->setCaption(def->type);
	label->setColour(0xffff00);
	label->setFontSize(18);
	label->setAutosize(true);
	
	int split = 0;
	HorizontalLayout* rowLayout = 0;
	std::list<particle::Definition<T>*> hierachy;
	while(def) {
		hierachy.push_front(def);
		def = def->parent;
	}


	for(auto& def : hierachy) {
		for(auto& i: def->properties) {
			script::Variable var = i.get(item);
			static const char* types[] = { "Value", "Int", "Float", "Bool", "Vector", "String", "Enum", "Gradient" };
			printf("%s %s : %s\n", types[(int)i.type], i.key.toString().str(), var.toString(2).str());

			if(!rowLayout) rowLayout = new HorizontalLayout();
			Widget* row = new Widget();
			row->setAnchor("lr");
			row->setAutosize(true);
			row->setLayout(rowLayout);
			panel->add(row);

			Label* label = row->createChild<Label>("label");
			label->setCaption(i.key.toString());
			label->setAutosize(true);

			auto createSpinbox = [row](float value) {
				SpinboxFloat* w = row->createChild<SpinboxFloat>("spinboxf");
				w->setAnchor("lr");
				w->setValue(value);
				w->setStep(0.1, 0.01);
				return w;
			};

			split = std::max(split, label->getSize().x);
			switch(i.type) {
			case particle::PropertyType::Bool:
			{
				Checkbox* check = row->createChild<Checkbox>("checkbox");
				check->setChecked(var);
				check->eventChanged.bind([this, &i, item](Button* b) {
					i.set(item, b->isSelected());
					if(i.key == "start") reinitialise();
					notifyChange();
				});
				break;
			}
			case particle::PropertyType::Int:
			{
				Spinbox* value = row->createChild<Spinbox>("spinbox");
				value->setRange(0, 1000000);
				value->setAnchor("lr");
				value->setValue(var);
				value->eventChanged.bind([this, &i, item](Spinbox*, int v) {
					i.set(item, v);
					notifyChange();
				});
				break;
			}
			case particle::PropertyType::Float:
			{
				SpinboxFloat* value = row->createChild<SpinboxFloat>("spinboxf");
				value->setAnchor("lr");
				value->setValue(var);
				value->eventChanged.bind([this, &i, item](SpinboxFloat*, float v) {
					i.set(item, v);
					notifyChange();
				});
				break;
			}
			case particle::PropertyType::Vector:
			{
				SpinboxFloat* x = createSpinbox(var.get("x"));
				SpinboxFloat* y = createSpinbox(var.get("y"));
				SpinboxFloat* z = createSpinbox(var.get("z"));
				auto func = [this, &i, item, x,y,z](SpinboxFloat*, float) {
					i.set(item, vec3(x->getValue(), y->getValue(), z->getValue()));
					notifyChange();
				};
				x->eventChanged.bind(func);
				y->eventChanged.bind(func);
				z->eventChanged.bind(func);
				break;
			}
			case particle::PropertyType::String:
			{
				Textbox* value = row->createChild<Textbox>("editbox");
				value->setAnchor("lr");
				value->setText(var);
				value->eventLostFocus.bind([](Widget* t) { cast<Textbox>(t)->eventSubmit(cast<Textbox>(t)); });
				value->eventSubmit.bind([this, &i, item](Textbox* t) {
					i.set(item, t->getText());
					refreshMaterial(m_manager, item);
					notifyChange();
				});
				break;
			}
			case particle::PropertyType::Enum:
			{
				Combobox* value = row->createChild<Combobox>("droplist");
				for(const char* e: i.enumValues) value->addItem(e);
				value->setAnchor("lr");
				value->selectItem(value->findItem((const char*)var)->getIndex());
				value->eventSelected.bind([this, &i, item](Combobox* c, ListItem& value) {
					i.set(item, value.getText());
					notifyChange();
				});
				break;
			}
			case particle::PropertyType::Value:
			{
				auto createValueEditor = [this, row, &i, item, createSpinbox](const Variable& var) {
					while(row->getWidgetCount() > 2) {
						Widget* w = row->getWidget(1);
						w->removeFromParent();
						delete w;
					}
					SpinboxFloat* w;

					switch(getValueType(var)) {
					case ValueType::Constant:
						w = createSpinbox(var);
						w->eventChanged.bind([this, &i, item](SpinboxFloat*, float v) {
							i.set(item, v);
							notifyChange();
						});
						break;
					case ValueType::Random:
						w = createSpinbox(var.get("x"));
						w->eventChanged.bind([](SpinboxFloat* s, float) { s->getParent()->getWidget(2)->as<SpinboxFloat>()->eventChanged(s,0); });
						w = createSpinbox(var.get("y"));
						w->eventChanged.bind([this, &i, item](SpinboxFloat* s, float) {
							vec2 value(s->getParent()->getWidget(1)->as<SpinboxFloat>()->getValue(), s->getParent()->getWidget(2)->as<SpinboxFloat>()->getValue());
							i.set(item, value);
							notifyChange();
						});
						break;
					case ValueType::Mapped:
						w = createSpinbox(var.get(0u));
						w->eventChanged.bind([](SpinboxFloat* s, float) { s->getParent()->getWidget(2)->as<SpinboxFloat>()->eventChanged(s,0); });
						w = createSpinbox(var.get(1u));
						w->eventChanged.bind([this, &i, item](SpinboxFloat* s, float) {
							Variable value;
							value.makeArray();
							value.set(0u, s->getParent()->getWidget(1)->as<SpinboxFloat>()->getValue());
							value.set(1u, s->getParent()->getWidget(2)->as<SpinboxFloat>()->getValue());
							i.set(item, value);
							notifyChange();
						});
						break;
					case ValueType::Graph:
						Button* b = row->createChild<Button>("button");
						b->setAnchor("lr");
						b->setCaption("Edit");
						b->eventPressed.bind([this, &i, item](Button* b) {
							m_changeValueDelegate.bind([this, &i, item](const Variable& var) { i.set(item, var); notifyChange(); });
							showGraphEditor(i.get(item));
						});
						break;
					}
					if(Widget* w = row->getWidget("mode")) w->raise();
				};

				createValueEditor(var);

				static const char* typeGlyphs[] = { "V", "R", "M", "G" }; // Todo: use icons
				Button* mode = row->createChild<Button>("button", "mode");
				mode->setCaption(typeGlyphs[(int)getValueType(var)]);
				mode->setSize(16, 16);
				mode->eventPressed.bind([this, &i, item, createValueEditor](Button* b) {
					m_changeTypeDelegate.bind([this, b, &i, item, createValueEditor](ValueType type) {
						b->setCaption(typeGlyphs[(int)type]);
						Variable data = i.get(item);
						Variable newVar = convertValueType(type, data);
						createValueEditor(newVar);
						i.set(item, newVar);
						notifyChange();
					});
					m_modePopup->popup(b, Popup::RIGHT);
				});

				break;
			}
			case particle::PropertyType::Gradient:
			{
				GradientBox* w = new GradientBox();
				w->setAnchor("lr");
				row->add(w);
				w->gradient = particle::loadGradient(var);
				w->resetRange();
				w->eventMouseUp.bind([this, w, &i, item](Widget*, const Point&, int b) {
					m_changeValueDelegate.bind([this, w, &i, item](const Variable& var) {
						w->gradient = particle::loadGradient(var);
						w->resetRange();
						i.set(item, var);
						notifyChange();
					});
					showGradientEditor(w);
				});
			}
			default: break;
			}
		}
		def = def->parent;
	}

	for(Widget* w: *panel) {
		if(Widget* l = w->getWidget(0)) {
			l->setAutosize(false);
			l->setSize(split+8, l->getSize().y);
		}
	}


	panel->resumeLayout();
}

void ParticleEditor::selectNode(Button* b) {
	if(!b) {
		Widget* panel = m_panel->getWidget("properties");
		panel->deleteChildWidgets();
		return;
	}

	ParticleNode* node = cast<ParticleNode>(b);
	const char* name = node->getNodeName();
	void* data = node->getNodeData();

	printf("-- Node selected: %s --\n", node->getNodeName());

	// Create properties panel
	switch(node->getNodeType()) {
	case EmitterNode: createPropertiesPanel(particle::s_emitterFactory[name], (particle::Emitter*)data); break;
	case AffectorNode: createPropertiesPanel(particle::s_affectorFactory[name], (particle::Affector*)data); break;
	case RenderDataNode: createPropertiesPanel(particle::s_renderDataFactory[name], (particle::RenderData*)data); break;
	case EventNode: createPropertiesPanel(particle::s_eventFactory[name], (particle::Event*)data); break;
	case SystemNode: break;
	}
	
	m_gradientEditor->setVisible(false);
	m_graphEditor->setVisible(false);
}


// ===================================================================================================== //

template<class T> const base::HashMap<particle::Definition<T>*>& getFactory();
template<> const base::HashMap<particle::Definition<particle::RenderData>*>& getFactory<particle::RenderData>() { return particle::s_renderDataFactory; }
template<> const base::HashMap<particle::Definition<particle::Affector>*>& getFactory<particle::Affector>() { return particle::s_affectorFactory; }
template<> const base::HashMap<particle::Definition<particle::Emitter>*>& getFactory<particle::Emitter>() { return particle::s_emitterFactory; }
template<> const base::HashMap<particle::Definition<particle::Event>*>& getFactory<particle::Event>() { return particle::s_eventFactory; }

template<class T> void saveProperties(FILE* fp, const T* data, int indent) {
	const char* type = getParticleClassName(data);
	particle::Definition<T>* def = getFactory<T>().get(type, 0);
	static const char* tabs = "\t\t\t\t\t";
	fprintf(fp, "%.*stype = \"%s\"\n", indent, tabs, type);
	while(def) {
		for(auto& i: def->properties) {
			fprintf(fp, "%.*s%s = %s\n", indent, tabs, i.key.toString().str(), i.get(data).toString(999, true, false).str());
		}
		def = def->parent;
	}
}

using ObjectReferenceMap = std::map<const particle::Object*, int>;
ParticleEditor::NodeType getNodeTypeFromObject(particle::Object* data) {
	if(dynamic_cast<const particle::Emitter*>(data)) return ParticleEditor::EmitterNode;
	else if(dynamic_cast<const particle::Event*>(data)) return ParticleEditor::EventNode;
	else if(dynamic_cast<const particle::Affector*>(data)) return ParticleEditor::AffectorNode;
	else if(dynamic_cast<const particle::RenderData*>(data)) return ParticleEditor::RenderDataNode;
	else if(dynamic_cast<const particle::System*>(data)) return ParticleEditor::SystemNode;
	assert(false);
	return ParticleEditor::SystemNode;
}

template<class T> void saveObject(FILE* fp, const char* list, const T* data, const ObjectReferenceMap& ref, int indent);
void saveObject(FILE* fp, const char* key, const particle::Object* data, const ObjectReferenceMap& ref, int indent);
template<class T> void saveReferences(FILE* fp, const T* data, const ObjectReferenceMap& ref, int indent) {}
void saveReferences(FILE* fp, const particle::Emitter* data, const ObjectReferenceMap& ref, int indent) {
	if(data->getRenderer()) {
		saveObject(fp, "particle", data->getRenderer(), ref, indent);
	}
	for(particle::Affector* a : data->affectors()) {
		saveObject(fp, "affectors[]", a, ref, indent);
	}
	for(particle::Event* v : data->events()) {
		saveObject(fp, "events[]", v, ref, indent);
	}
}
void saveReferences(FILE* fp, const particle::Event* data, const ObjectReferenceMap& ref, int indent) {
	if(data->target) {
		saveObject(fp, "target", data->target, ref, indent);
	}
}

template<class T>
void saveObject(FILE* fp, const char* key, const T* data, const ObjectReferenceMap& ref, int indent) {
	static const char* tabs = "\t\t\t\t\t";
	auto it = ref.find(data);
	if(it!=ref.end()) fprintf(fp, "%.*s%s = objects[%d]\n", indent, tabs, key, it->second);
	else {
		fprintf(fp, "%.*s%s = {\n", indent, tabs, key);
		saveReferences(fp, data, ref, indent+1);
		saveProperties(fp, data, indent+1);
		fprintf(fp, "%.*s}\n", indent, tabs);
	}
}
void saveObject(FILE* fp, const char* key, const particle::Object* data, const ObjectReferenceMap& ref, int indent) {
	if(const particle::Emitter* o = dynamic_cast<const particle::Emitter*>(data)) saveObject(fp, key, o, ref, indent);
	else if(const particle::Event* o = dynamic_cast<const particle::Event*>(data)) saveObject(fp, key, o, ref, indent);
	else if(const particle::Affector* o = dynamic_cast<const particle::Affector*>(data)) saveObject(fp, key, o, ref, indent);
	else if(const particle::RenderData* o = dynamic_cast<const particle::RenderData*>(data)) saveObject(fp, key, o, ref, indent);
}


bool ParticleEditor::save(particle::System* system, const char* filename) {
	if(!system) return false;
	FILE* fp = fopen(filename, "w");
	if(!fp) return false;

	// Extract anything referenced to not inline
	struct ObjectInfo { int references = 0; };
	std::map<const particle::Object*, ObjectInfo> info;
	for(particle::Emitter* e: system->emitters()) {
		// Only add refrences for emitters linked to the system node
		if(!e->eventOnly) ++info[e].references;
		++info[e->getRenderer()].references;
		for(particle::Affector* a: e->affectors()) ++info[a].references;
		for(particle::Event* v: e->events()) ++info[v->target].references;
	}

	fprintf(fp, "system = {\n");
	fprintf(fp, "\tpool = %d\n", system->getPoolSize());

	// Add multi-referenced objects first - Order matters - need dependency tracking
	int refIndex = 0;
	char nameBuffer[32];
	ObjectReferenceMap references;
	for(auto& i: info) {
		if(i.second.references > 1) {
			references[i.first] = refIndex;
			sprintf(nameBuffer, "objects[%d]", refIndex);
			saveObject(fp, nameBuffer, i.first, references, 1);
			++refIndex;
		}
	}

	for(particle::Emitter* e: system->emitters()) {
		if(!e->eventOnly) saveObject(fp, "emitters[]", e, references, 1);
	}
	fprintf(fp, "}\n");
	fclose(fp);
	return true;
}

