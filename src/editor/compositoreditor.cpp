#include <base/editor/compositoreditor.h>
#include <base/editor/nodeeditor.h>
#include <base/compositor.h>
#include <base/material.h>
#include <base/resources.h>
#include <base/texture.h>
#include <base/xml.h>
#include <algorithm>
#include <cstdio>

#include <base/editor/embed.h>
BINDATA(editor_compositor_gui, EDITOR_DATA "/compositor.xml")

using namespace gui;
using namespace base;
using namespace editor;
using base::Texture;

// Graph nodes
namespace editor {
class CompositorNode : public nodegraph::Node {
	WIDGET_TYPE(CompositorNode)
	CompositorNode(const Rect& r, Skin* s): Node(r,s) {}
	void setCompositor(Compositor* c, int alias) { m_compositor = c; m_alias = alias; }
	Compositor* getCompositor() const { return m_compositor; }
	int getAlias() const { return m_alias; }
	void setAlias(int id) { m_alias=id; }
	protected:
	Compositor* m_compositor;
	int m_alias; // index in chain
};
}



#define CONNECT(Type, name, event, callback) { Type* w = m_panel->getWidget<Type>(name); if(w) w->event.bind(this, &CompositorEditor::callback); }
#define CONNECT_I(Type, name, event, callback) { Type* w = m_panel->getWidget<Type>(name); if(w) w->event.bind(callback); }
#define CONNECT_SUB(Type, name, subname, event, ...) { Widget* w = m_panel->getWidget(name); if(w) { Type* s=w->getTemplateWidget<Type>(subname); if(s) { s->event.bind(this, &CompositorEditor::__VA_ARGS__);} } }
#define CONNECT_SUB_I(Type, name, subname, event, ...) { Widget* w = m_panel->getWidget(name); if(w) { w=w->getTemplateWidget<Type>(subname); if(w) {  w->event.bind(__VA_ARGS__);} } }

void CompositorEditor::initialise() {
	m_panel = loadUI("compositor.xml", editor_compositor_gui);
	if(!m_panel) { printf("ERROR: Missing compositor editor widget\n"); return; }
	Root::registerClass<CompositorNode>();
	addToggleButton(m_panel, "editors", "compositor");

	m_graphList = m_panel->getWidget<Listbox>("graphlist");
	m_nodeList = m_panel->getWidget<Listbox>("nodelist");
	Widget* graphContainer = m_panel->getWidget("graph");
	for(Widget* w: *graphContainer) w->setVisible(false); // Hide templates

	m_graphEditor = new nodegraph::NodeEditor(graphContainer->getClientRect(), graphContainer->getSkin());
	graphContainer->add(m_graphEditor);
	m_graphEditor->setAnchor("tlrb");
	m_graphEditor->eventChanged.bind(this, &CompositorEditor::graphChanged);

	m_sceneTemplate = m_panel->getWidget("scenepass");
	m_clearTemplate = m_panel->getWidget("clearpass");
	m_quadTemplate = m_panel->getWidget("quadpass");
	m_copyTemplate = m_panel->getWidget("copypass");
	m_bufferTemplate = m_panel->getWidget("buffer");


	graphContainer->eventMouseWheel.bind([this](Widget* w, int delta) {
		ScaleBox* s = cast<ScaleBox>(w);
		Transform::Pos anchor = s->getDerivedTransform().untransformf(s->getRoot()->getMousePos());
		s->setScale(s->getScale() * (1+delta * 0.1));
		Transform::Pos result = s->getDerivedTransform().untransformf(s->getRoot()->getMousePos());
		Point shift(result.x - anchor.x, result.y - anchor.y);
		for(Widget* w: *m_graphEditor) w->setPosition(w->getPosition() + shift);
	});

	CONNECT(Listbox, "nodelist", eventMouseMove, dragCompositor);
	CONNECT(Listbox, "nodelist", eventSelected, selectCompositor);
	CONNECT(Listbox, "graphlist", eventSelected, selectGraph);
	CONNECT(Button, "newgraph", eventPressed, newGraph);
	CONNECT(Button, "newnode", eventPressed, newCompositor);
	CONNECT(Button, "addbuffer", eventPressed, addBuffer);

	m_passTypePopup = m_panel->getWidget<Popup>("passtypes");
	CONNECT(Button, "addpass", eventPressed,  addPass);
	CONNECT(Button, "newscene", eventPressed, addScenePass);
	CONNECT(Button, "newquad", eventPressed,  addQuadPass);
	CONNECT(Button, "newclear", eventPressed, addClearPass);
	CONNECT(Button, "newcopy", eventPressed,  addCopyPass);

	CONNECT_SUB(Button, "inputs", "add", eventPressed, addConnector, 0, w, "input");
	CONNECT_SUB(Button, "outputs", "add", eventPressed, addConnector, 1, w, "output");

	// Set up buffer format lists - could be done in guiedit since this is static
	m_formats = new ItemList();
	m_formats->addItem("R8", Texture::R8);
	m_formats->addItem("RG8", Texture::RG8);
	m_formats->addItem("RGB8", Texture::RGB8);
	m_formats->addItem("RGBA8", Texture::RGBA8);
	m_formats->addItem("R16F", Texture::R16F);
	m_formats->addItem("RG16F", Texture::RG16F);
	m_formats->addItem("RGB16F", Texture::RGB16F);
	m_formats->addItem("RGBA16F", Texture::RGBA16F);
	m_formats->addItem("R32F", Texture::R32F);
	m_formats->addItem("RG32F", Texture::RG32F);
	m_formats->addItem("RGB32F", Texture::RGB32F);
	m_formats->addItem("RGBA32F", Texture::RGBA32F);
	m_formats->addItem("R11G11B10F", Texture::R11G11B10F);
	m_formats->addItem("R5G6B5", Texture::R5G6B5);

	m_formats->addItem("D24S8", Texture::D24S8);
	m_formats->addItem("D16", Texture::D16);
	m_formats->addItem("D24", Texture::D24);
	m_formats->addItem("D32", Texture::D32);

	m_targets = new ItemList();
	refreshCompositorList();
	setCompositor(0);
}

CompositorEditor::~CompositorEditor() {
	delete m_targets;
	delete m_formats;
}

void CompositorEditor::activate() {
	if(m_panel) m_panel->setVisible(true);
}

void CompositorEditor::deactivate() {
}

bool CompositorEditor::isActive() const {
	return m_panel && m_panel->isVisible();
}

void CompositorEditor::refresh() {
	if(!m_panel) return;
	if(Workspace* w = getEditor()->getWorkspace()) {
		setGraph(GraphInfo{const_cast<CompositorGraph*>(w->getGraph())});
		ListItem null;
		selectGraph(m_graphList, null);
	}
}

void CompositorEditor::refreshCompositorList() {
	Resources* res = Resources::getInstance();
	m_nodeList->clearItems();
	m_graphList->clearItems();
	if(!res) return;
	for(auto c: res->compositors) addCompositor(c.key, c.value);
	for(auto g: res->graphs) m_graphList->addItem(g.key, GraphInfo{g.value});
	m_nodeList->sortItems();
	m_graphList->sortItems(ItemList::IGNORE_CASE | ItemList::INVERSE);
}

void CompositorEditor::addCompositor(const char* name, Compositor* c) {
	m_nodeList->addItem(name, c);
	m_nodeList->sortItems();
}

void CompositorEditor::newGraph(Button*) {
	CompositorGraph* graph = new CompositorGraph();
	graph->add(Compositor::Output);
	m_graphList->addItem("New Graph", GraphInfo{graph});
	m_graphList->selectItem(m_graphList->getItemCount()-1);
	setGraph(graph);
}

void CompositorEditor::selectGraph(Listbox* list, ListItem& item) {
	if(item.isValid()) {
		GraphInfo graph = item.getValue<GraphInfo>(1, GraphInfo{0});
		setGraph(graph);
		applyGraph(graph.graph);
	}
	else if(m_graph) {
		ListItem* item = list->findItem([this](ListItem&i){return i.getValue<GraphInfo>(1, GraphInfo{0}).graph==m_graph;});
		if(item) list->selectItem(item->getIndex());
	}
}

void CompositorEditor::newCompositor(Button*) {
	m_compositor = new Compositor();
	addCompositor("New Compositor", m_compositor);
	setCompositor(m_compositor);
}

void CompositorEditor::selectCompositor(Listbox* list, ListItem& item) {
	m_compositor = item.getValue<Compositor*>(1, nullptr);
	setCompositor(m_compositor);
}

void CompositorEditor::dragCompositor(Widget* w, const Point& pos, int b) {
	if(b && m_nodeList->getSelectedIndex()>=0 && m_graph) {
		Point p = pos + w->getAbsolutePosition();
		if(m_graphEditor->getAbsoluteRect().contains(p)) {
			for(Widget* w: *m_graphEditor) w->setSelected(false);
			CompositorNode* node = createGraphNode(m_compositor, 0);
			node->setSelected(true);
			node->forceStartDrag();
		}
	}
}

// ------------------------------------------------ //

const char* CompositorEditor::getCompositorName(Compositor* c, const char* outputName) const {
	if(c == Compositor::Output) return outputName;
	const ListItem* item = m_nodeList->findItem(1, c);
	return item? item->getText(): "";
}

void CompositorEditor::setCompositor(Compositor* c) {
	// Select in list (by item data)
	if(c != m_compositor) {
		m_compositor = c;
		ListItem* item = m_nodeList->findItem(1, c);
		if(item) {
			m_nodeList->selectItem(item->getIndex());
			m_nodeList->ensureVisible(item->getIndex());
		}
	}

	m_panel->getWidget("details")->setVisible(c && c!=Compositor::Output);
	if(!c) return;

	// Inputs
	Widget* inputs = m_panel->getWidget("inputs");
	inputs->pauseLayout();
	inputs->deleteChildWidgets();
	uint index = 0;
	while(const char* name = c->getInputName(index++)) addConnectorWidget(inputs, name);
	inputs->resumeLayout();

	index = 0;
	Widget* outputs = m_panel->getWidget("outputs");
	outputs->pauseLayout();
	outputs->deleteChildWidgets();
	clearItemPanel(outputs);
	while(const char* name = c->getOutputName(index++)) addConnectorWidget(outputs, name);
	outputs->resumeLayout();


	index = 0;
	Widget* buffers = m_panel->getWidget("buffers");
	clearItemPanel(buffers);
	while(Compositor::Buffer* buffer = c->getBuffer(index++)) {
		Widget* w = addBufferWidget(buffers);
		bool relative = buffer->relativeWidth != 0;
		w->getWidget<Textbox>("name")->setText(buffer->name);
		w->getWidget<Checkbox>("relative")->setChecked(relative);
		w->getWidget<SpinboxFloat>("width")->setValue(relative? buffer->relativeWidth: buffer->width);
		w->getWidget<SpinboxFloat>("height")->setValue(relative? buffer->relativeHeight: buffer->height);
		
		Widget* buffers = w->getWidget("buffers");
		auto addBuffer = [this, buffers](Texture::Format format) {
			if(format == Texture::NONE) return;
			Widget* w = addDataWidget("listitem", buffers);
			Combobox* value = w->getWidget<Combobox>("value");
			value->shareList(m_formats);
			if(ListItem* item = m_formats->findItem(1, format)) value->selectItem(item->getIndex());
		};
		addBuffer(buffer->depth.format);
		addBuffer(buffer->colour[0].format);
		addBuffer(buffer->colour[1].format);
		addBuffer(buffer->colour[2].format);
		addBuffer(buffer->colour[3].format);
	}

	index = 0;
	Widget* passes = m_panel->getWidget("passes");
	clearItemPanel(passes);
	while(CompositorPass* pass = c->getPass(index++)) {
		Widget* w = 0;
		if(CompositorPassClear* clear = dynamic_cast<CompositorPassClear*>(pass)) {
			w = addDataWidget("clearpass", passes);
			w->getWidget<Checkbox>("colourbit")->setEnabled(clear->getBits()&CLEAR_COLOUR);
			w->getWidget<Checkbox>("depthbit")->setEnabled(clear->getBits()&CLEAR_DEPTH);
			w->getWidget<Checkbox>("stencilbit")->setEnabled(clear->getBits()&CLEAR_STENCIL);
		}
		if(CompositorPassQuad* quad = dynamic_cast<CompositorPassQuad*>(pass)) {
			w = addDataWidget("quadpass", passes);
			quad = 0;
		}
		if(CompositorPassScene* scene = dynamic_cast<CompositorPassScene*>(pass)) {
			w = addDataWidget("scenepass", passes);
			w->getWidget<Textbox>("camera")->setText(scene->getCamera());
		}

		if(w) w->getWidget<Combobox>("target")->setText(c->getPassTarget(index-1));
	}
}

void CompositorEditor::clearItemPanel(Widget* panel) {
	while(panel->getWidgetCount()>1) {
		Widget* w = panel->getWidget( panel->getWidgetCount() - 2);
		w->removeFromParent();
		delete w;
	}
}

Widget* CompositorEditor::addDataWidget(const char* name, Widget* parent) {
	Widget* w = m_panel->getWidget(name)->clone();
	w->getWidget<Button>("remove")->eventPressed.bind(this, &CompositorEditor::removeItem);
	w->setVisible(true);
	parent->add(w);
	return w;
}

Widget* CompositorEditor::addConnectorWidget(Widget* parent, const char* name) {
	Widget* w = addDataWidget("textitem", parent);
	w->getWidget<Textbox>("value")->setText(name);
	return w;
}
Widget* CompositorEditor::addBufferWidget(Widget* parent) {
	Widget* w = addDataWidget("buffer", parent);
	return w;
}

// ------------------------------------------------ //

template<class F> const char* makeUnique(const char* name, F&& isUnique) {
	if(isUnique(name)) return name;
	static char tmp[128];
	int index = 0;
	while(true) {
		sprintf(tmp, "%s.%d", name, index++);
		if(isUnique(tmp)) return tmp;
	}
	return name;
}

void CompositorEditor::addConnector(int mode, Widget* list, const char* name) {
	if(mode) name = makeUnique(name, [this](const char* n) { return m_compositor->getOutput(n)<0; });
	else name = makeUnique(name, [this](const char* n) { return m_compositor->getInput(n)<0; });

	addConnectorWidget(list, name);
	if(mode) m_compositor->addOutput(name);
	else m_compositor->addInput(name);
}
void CompositorEditor::clearConnectors(int mode, Widget* list) {
	list->deleteChildWidgets();
	if(mode) buildOutputConnectors(list);
	else buildInputConnectors(list);
}
void CompositorEditor::renameConnector(Textbox* t) {
	Widget* list = t->getParent()->getParent();
	bool input = list->getName()[0] == 'i';
	if(input) buildInputConnectors(list);
	else buildOutputConnectors(list);
	// ToDo: Make sure name is unique
}

void CompositorEditor::removeItem(Button* b) {
	Widget* item = b->getParent();
	String listName = item->getParent()->getName();
	if(listName == "inputs") {
		m_compositor->removeInput(item->getWidget<Textbox>("value")->getText());
	}
	else if(listName == "outputs") {
		m_compositor->removeOutput(item->getWidget<Textbox>("value")->getText());
	}
	else if(listName == "buffers") {
	}
	else if(listName == "textures") {
	}

	item->getParent()->remove(item);
	delete item;
}

void CompositorEditor::buildInputConnectors(Widget* list) {
	while(const char* name = m_compositor->getInputName(0)) m_compositor->removeInput(name);
	for(Widget* w: list) m_compositor->addInput(w->getWidget<Textbox>("value")->getText());
	// ToDo: update any existing graph nodes
}
void CompositorEditor::buildOutputConnectors(Widget* list) {
	while(const char* name = m_compositor->getOutputName(0)) m_compositor->removeOutput(name);
	for(Widget* w: list) m_compositor->addInput(w->getWidget<Textbox>("value")->getText());
	// ToDo: update any existing graph nodes
}

// ------------------------------------------------ //

void CompositorEditor::addBuffer(Button*) {
	Widget* buffers = m_panel->getWidget("buffers");
	addBufferWidget(buffers);
}
void CompositorEditor::removeBuffer(Button* b) {
	b->getParent()->removeFromParent();
	delete b->getParent();
}

// ------------------------------------------------ //

void CompositorEditor::addPass(Button* b) {
	if(!m_compositor) return;
	m_passTypePopup->popup(b);
}
void CompositorEditor::addQuadPass(Button*)  { addPass("quadpass",  new CompositorPassQuad()); }
void CompositorEditor::addScenePass(Button*) { addPass("scenepass", new CompositorPassScene(0, 10)); }
void CompositorEditor::addClearPass(Button*) { addPass("clearpass", new CompositorPassClear()); }
void CompositorEditor::addCopyPass(Button*)  { addPass("copypass",  new CompositorPassCopy(0)); }
void CompositorEditor::addPass(const char* widget, CompositorPass* pass) {
	Widget* w = m_panel->getWidget(widget)->clone();
	w->getWidget<Button>("remove")->eventPressed.bind(this, &CompositorEditor::removePass);
	w->setVisible(true);
	Widget* passes = m_panel->getWidget("passes");
	passes->add(w, passes->getWidgetCount() - 1);
	w->getWidget<Combobox>("target")->shareList(m_targets);
	m_compositor->addPass("", pass);
	m_passTypePopup->hide();
}
void CompositorEditor::removePass(Button* b) {
	Widget* passWidget = b->getParent();
	//int index = passWidget->getIndex();
	passWidget->removeFromParent();
	delete passWidget;
	// m_compositor->deletePass(index);
}



// ------------------------------------------------ //

void CompositorEditor::setGraph(CompositorGraph* graph) {
	setGraph(GraphInfo{graph});
}

void CompositorEditor::setGraph(const GraphInfo& g) {
	if(m_graph == g.graph) return;
	if(m_graph) m_graphEditor->deleteChildWidgets();
	m_graph = g.graph;
	if(!m_graph) return;
	CompositorGraph* graph = m_graph;

	// Nodes
	for(uint i=0; i<graph->size(); ++i) {
		CompositorNode* node = createGraphNode(graph->getCompositor(i), i);
		if(i<g.nodes.size()) node->setPosition(g.nodes[i]);
		else node->setPosition(60 + i*140, i*80);
	}
	
	// Links
	for(const CompositorGraph::Link& link: graph->links()) {
		CompositorNode* from = getGraphNode(link.a);
		CompositorNode* to = getGraphNode(link.b);
		// Named
		if(link.out>=0 && link.in>=0) {
			m_graphEditor->link(from, link.out, to, link.in);
		}
		// Only one possible connector
		else if(!from->getCompositor()->getOutputName(1) && !to->getCompositor()->getInputName(1)) {
			m_graphEditor->link(from, 0, to, 0);
		}
		// All matching pairs
		else for(int o=0; from->getCompositor()->getOutputName(o); ++o) {
			int t = to->getCompositor()->getInput(from->getCompositor()->getOutputName(o));
			if(t>=0) m_graphEditor->link(from, o, to, t);
		}
	}

	// Automatic layout pass
	if(g.nodes.empty()) layoutGraph();
}

void CompositorEditor::layoutGraph() {
	CompositorNode* output = getGraphNode(Compositor::Output);
	if(!output) return;
	
	int k = 50;
	Point pos(0,0);
	std::vector<CompositorNode*> list;
	output->setPosition(pos.x, pos.y);
	Rect bounds = output->getRect();
	list.push_back(output);
	while(!list.empty() && --k) {
		std::vector<CompositorNode*> next;
		for(CompositorNode* node: list) {
			for(const nodegraph::Link& link: node->getLinks(nodegraph::INPUT)) {
				next.push_back(cast<CompositorNode>(link.a));
			}
		}
		pos.x -= 200;
		pos.y = next.size() / 2 * 60;
		for(CompositorNode* n: next) {
			n->setPosition(pos);
			bounds.include(n->getRect());
			pos.y += 120;
		}
		list.swap(next);
	}
	// Shift them all
	Point shift = -bounds.position();
	for(Widget* w: *m_graphEditor) w->setPosition(w->getPosition() + shift);
}


CompositorNode* CompositorEditor::createGraphNode(Compositor* c, int alias) {
	CompositorNode* node = m_panel->getRoot()->createWidget<CompositorNode>("graphnode");
	m_graphEditor->add(node);
	node->setCompositor(c, alias);
	node->setCaption(getCompositorName(c));
	node->setConnectorTemplates(m_panel->getRoot()->getTemplate("nodeinput"), m_panel->getRoot()->getTemplate("nodeoutput"));
	node->eventPressed.bind(this, &CompositorEditor::selectNode);
	if(c == Compositor::Output) node->setPersistant(true);
	uint index = 0;
	while(const char* name = c->getInputName(index++)) node->addInput(name);
	index = 0;
	while(const char* name = c->getOutputName(index++)) node->addOutput(name);
	return node;
}

CompositorNode* CompositorEditor::getGraphNode(Compositor* c, int index) {
	for(Widget* w: *m_graphEditor) {
		CompositorNode* node = cast<CompositorNode>(w);
		if(node && node->getCompositor() == c && --index<0) return node;
	}
	return 0;
}
CompositorNode* CompositorEditor::getGraphNode(int alias) {
	for(Widget* w: *m_graphEditor) {
		CompositorNode* node = cast<CompositorNode>(w);
		if(node && node->getAlias() == alias) return node;
	}
	return 0;
}

void CompositorEditor::selectNode(Button* b) {
	Compositor* c = cast<CompositorNode>(b)->getCompositor();
	if(c == Compositor::Output) return;
	setCompositor(c);
}

// ---------------------------------------------------------------------- //

void CompositorEditor::graphChanged(nodegraph::NodeEditor*) {
	CompositorGraph* graph = buildGraph();
	applyGraph(graph);

	// Replace graph in editor list
	ListItem* item = m_graphList->getSelectedItem();
	delete item->getValue<CompositorGraph*>(1, nullptr);
	GraphInfo data{graph};
	for(Widget* w: *m_graphEditor) data.nodes.push_back(w->getPosition());
	item->setValue(1, data);
	m_graph = graph;
}

CompositorGraph* CompositorEditor::buildGraph() const {
	CompositorGraph* graph = new CompositorGraph();
	
	std::vector<CompositorNode*> nodes;
	for(Widget* w: *m_graphEditor) {
		CompositorNode* node = cast<CompositorNode>(w);
		if(node) {
			node->setAlias(nodes.size());
			nodes.push_back(node);
		}
	}

	for(CompositorNode* n: nodes) graph->add(n->getCompositor());

	using namespace nodegraph;
	auto id = [](Node* n) { return cast<CompositorNode>(n)->getAlias(); };
	for(CompositorNode* n: nodes) {
		for(const Link& link: n->getLinks(OUTPUT)) {
			graph->link(id(link.a), id(link.b), link.a->getConnectorName(OUTPUT,link.ia), link.b->getConnectorName(INPUT,link.ib));
		}
	}

	return graph;
}

bool CompositorEditor::applyGraph(CompositorGraph* graph) {
	Point size(256, 256);
	Workspace* current = getEditor()->getWorkspace();
	if(current) size.set(current->getWidth(), current->getHeight());
	Workspace* workspace = new Workspace(graph);
	printf("[Compiling compositor graph: %s]\n", m_graphList->getSelectedItem()->getText());
	if(workspace->compile(size.x, size.y)) {
		printf("Compiled succesfully\n");
		if(current) workspace->copyCameras(current);
		getEditor()->changeWorkspace(workspace);
		exportGraph(graph);
		return true;
	}
	return false;
}


void CompositorEditor::exportGraph(CompositorGraph* graph) const {
	const ListItem* item = m_graphList->findItem([graph](const ListItem& i) { return i.getValue(1, GraphInfo{0}).graph == graph; });
	const char* name = item? item->getText(): "";
	static const char* output = "out";
	XMLElement g("graph");
	g.setAttribute("name", name);
	g.setAttribute("output", output);

	// Aliases
	char nameBuffer[64];
	std::vector<String> names;
	for(size_t i=0; i<graph->size(); ++i) {
		Compositor* c = graph->getCompositor(i);
		const char* name = getCompositorName(c, output);
		bool first = true;
		for(size_t j=0; j<i && first; ++j) {
			first = graph->getCompositor(i) != graph->getCompositor(j);
		}
		if(first) names.push_back(name);
		else {
			snprintf(nameBuffer, 64, "%s~%d", name, (int)i);
			names.push_back(nameBuffer);
			XMLElement& alias = g.add("alias");
			alias.setAttribute("name", nameBuffer);
			alias.setAttribute("compositor", name);
		}
	}

	// print all connections
	for(const CompositorGraph::Link& link: graph->links()) {
		Compositor* a = graph->getCompositor(link.a);
		Compositor* b = graph->getCompositor(link.b);
		const char* out = a->getOutputName(link.out);
		const char* in = b->getInputName(link.in);

		XMLElement& l = g.add("link");
		l.setAttribute("a", names[link.a].str());
		l.setAttribute("b", names[link.b].str());
		if(out && in && strcmp(out, in)==0) l.setAttribute("key", out);
		else {
			if(out) l.setAttribute("sa", out);
			if(in) l.setAttribute("sb", in);
		}
	}

	// dump
	XML xml;
	xml.setRoot(g);
	printf("%s", xml.toString());
}

