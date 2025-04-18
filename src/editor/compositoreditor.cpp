#include <base/editor/compositoreditor.h>
#include <base/editor/nodeeditor.h>
#include <base/gui/colourpicker.h>
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
	void setCompositor(Compositor* c, int alias) { m_compositor = c; m_alias = alias; }
	Compositor* getCompositor() const { return m_compositor; }
	int getAlias() const { return m_alias; }
	void setAlias(int id) { m_alias=id; }
	protected:
	Compositor* m_compositor;
	int m_alias; // index in chain
};
}

void updateExpandProperyInfo(Widget* p) {
	if(Label* info = cast<Label>(p->getTemplateWidget("info"))) {
		switch(p->getWidgetCount()) {
		case 0: info->setCaption("Empty"); break;
		case 1: info->setCaption("1 Item"); break;
		default: info->setCaption(String::format("%d Items", p->getWidgetCount())); break;
		}
	}
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

	m_graphEditor = new nodegraph::NodeEditor();
	m_graphEditor->setSize(graphContainer->getSize());
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
		if(!graph.graph) {
			graph.graph = Resources::getInstance()->graphs.get(item.getText());
			item.setValue(1, graph);
		}
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
	if(!m_compositor) {
		m_compositor = Resources::getInstance()->compositors.get(item.getText());
		item.setValue(1, m_compositor);
	}

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
	return item? item->getText(): " ";
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
	for(const auto& in: c->getInputs()) addConnectorWidget(inputs, in.name);
	inputs->resumeLayout();
	updateExpandProperyInfo(inputs);

	Widget* outputs = m_panel->getWidget("outputs");
	outputs->pauseLayout();
	outputs->deleteChildWidgets();
	for(const auto& out: c->getOutputs()) addConnectorWidget(outputs, out.name);
	outputs->resumeLayout();
	updateExpandProperyInfo(outputs);


	Widget* buffers = m_panel->getWidget("bufferlist");
	buffers->deleteChildWidgets();
	for(Compositor::Buffer* buffer: c->getBuffers()) {
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

	refreshTargetsList();
	Widget* passes = m_panel->getWidget("passlist");
	passes->deleteChildWidgets();
	for(const Compositor::Pass& pass : c->getPasses()) {
		Widget* w = 0;
		if(CompositorPassClear* clear = dynamic_cast<CompositorPassClear*>(pass.pass)) {
			static auto setBit = [](CompositorPassClear* c, ClearBits bit, Button* b) {
				c->setBits((ClearBits)(((int)c->getBits()&~(int)bit) | (b->isSelected()? (int)bit: 0)));
			};
			w = addDataWidget("clearpass", passes);
			Checkbox* depthBit = w->getWidget<Checkbox>("depthbit");
			Checkbox* colourBit = w->getWidget<Checkbox>("colourbit");
			Checkbox* stencilBit = w->getWidget<Checkbox>("stencilbit");

			depthBit->setChecked(clear->getBits()&CLEAR_DEPTH);
			colourBit->setChecked(clear->getBits()&CLEAR_COLOUR);
			stencilBit->setChecked(clear->getBits()&CLEAR_STENCIL);

			SpinboxFloat* depth = w->getWidget<SpinboxFloat>("depth");
			depth->setValue(clear->getDepth());
			depth->setEnabled(depthBit->isChecked());
			depth->eventChanged.bind([clear](SpinboxFloat*, float v) { clear->setDepth(v); });

			const float* c = clear->getColour();
			Widget* colour = w->getWidget("colour");
			colour->setVisible(colourBit->isChecked());
			colour->setColour(Colour(c[0], c[1], c[2], c[3]).toARGB());
			
			depthBit->eventChanged.bind([clear, depth](Button* b) { setBit(clear, CLEAR_DEPTH, b); depth->setEnabled(b->isSelected()); });
			colourBit->eventChanged.bind([clear, colour](Button* b) { setBit(clear, CLEAR_COLOUR, b); colour->setVisible(b->isSelected()); });
			stencilBit->eventChanged.bind([clear](Button* b) { setBit(clear, CLEAR_STENCIL, b); });

			colour->eventMouseDown.bind([clear, colour](Widget* w, const Point&, int) {
				const float* c = clear->getColour();
				ColourPicker* picker = new ColourPicker();
				picker->setSize(100, 80);
				picker->initialise(w->getRoot(), PropertyMap());
				picker->setHasAlpha(true);
				picker->setColour(Colour(c[0], c[1], c[2], c[3]));
				picker->eventChanged.bind([clear, colour](const Colour& c) { clear->setColour(c); colour->setColour(c.toARGB()); });
				(new Popup(picker, true))->popup(w);
			});
		}

		if(CompositorPassQuad* quad = dynamic_cast<CompositorPassQuad*>(pass.pass)) {
			w = addDataWidget("quadpass", passes);
			quad = 0;
		}
		if(CompositorPassScene* scene = dynamic_cast<CompositorPassScene*>(pass.pass)) {
			w = addDataWidget("scenepass", passes);
			w->getWidget<Textbox>("camera")->setText(scene->getCamera());
			Spinbox* first = w->getWidget<Spinbox>("first");
			Spinbox* last = w->getWidget<Spinbox>("last");
			first->setValue(scene->getRenderQueueRange().first);
			last->setValue(scene->getRenderQueueRange().second);
			first->eventChanged.bind([scene,last](Spinbox*, int v) {
				if(last->getValue()<v) last->setValue(v);
				scene->setRenderQueueRange(v, last->getValue());
			});
			last->eventChanged.bind([scene,first](Spinbox*, int v) {
				if(first->getValue()>v) first->setValue(v);
				scene->setRenderQueueRange(first->getValue(), v);
			});

			Textbox* cam = w->getWidget<Textbox>("camera");
			cam->setText(scene->getCamera());
			cam->eventSubmit.bind([scene](Textbox* t) { scene->setCamera(t->getText()); });
		}

		if(w) {
			Combobox* target = w->getWidget<Combobox>("target");
			target->shareList(m_targets);
			target->setText(pass.target);
			target->eventSelected.bind([this, pass=pass.pass](Combobox*, ListItem& item) {
				m_compositor->setPassTarget(pass, item);
			});
		}
	}
}

void CompositorEditor::refreshTargetsList() {
	m_targets->clearItems();
	for(Compositor::Buffer* buffer: m_compositor->getBuffers()) {
		m_targets->addItem(buffer->name);
	}
	for(const Compositor::Connector& in: m_compositor->getInputs()) {
		if(in.buffer && !m_targets->findItem(in.buffer)) m_targets->addItem(in.buffer);
	}
	for(const Compositor::Connector& out: m_compositor->getOutputs()) {
		if(out.buffer && !m_targets->findItem(out.buffer)) m_targets->addItem(out.buffer);
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
	w->getWidget<Textbox>("value")->eventSubmit.bind(this, &CompositorEditor::renameConnector);
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

template<class F>
void CompositorEditor::eachActiveNode(F&& func) {
	for(Widget* n: m_graphEditor) {
		CompositorNode* node = cast<CompositorNode>(n);
		if(node->getCompositor() == m_compositor) func(node);
	}
}

void CompositorEditor::addConnector(int mode, Widget* list, const char* name) {
	if(mode) name = makeUnique(name, [this](const char* n) { return m_compositor->getOutput(n)<0; });
	else name = makeUnique(name, [this](const char* n) { return m_compositor->getInput(n)<0; });

	addConnectorWidget(list, name);
	if(mode) m_compositor->addOutput(name);
	else m_compositor->addInput(name);
	updateExpandProperyInfo(list);

	eachActiveNode([mode, name](CompositorNode* node) {
		if(mode) node->addOutput(name);
		else node->addInput(name);
	});
	refreshTargetsList();
}

void CompositorEditor::renameConnector(Textbox* t) {
	Widget* list = t->getParent()->getParent();
	bool input = list->getName()[0] == 'i';
	int index = t->getParent()->getIndex();
	if(input) buildInputConnectors(list);
	else buildOutputConnectors(list);
	// ToDo: Make sure name is unique
	eachActiveNode([input, index, t](CompositorNode* node) {
		if(input) cast<Button>(node->getInputs()[index].widget)->setCaption(t->getText());
		else cast<Button>(node->getOutputs()[index].widget)->setCaption(t->getText());
	});
	refreshTargetsList();
	//ToDo: Update pass targets
}

void CompositorEditor::removeItem(Button* b) {
	Widget* item = b->getParent();
	Widget* list = item->getParent();
	if(list->getName() == "inputs") {
		m_compositor->removeInput(item->getWidget<Textbox>("value")->getText());
		eachActiveNode([i = item->getIndex()](CompositorNode* n) { n->removeConnector(nodegraph::INPUT, i); });
	}
	else if(list->getName() == "outputs") {
		m_compositor->removeOutput(item->getWidget<Textbox>("value")->getText());
		eachActiveNode([i = item->getIndex()](CompositorNode* n) { n->removeConnector(nodegraph::OUTPUT, i); });
	}
	else if(list->getName() == "buffers") {
	}
	else if(list->getName() == "textures") {
	}

	delete item;
	updateExpandProperyInfo(list);
	refreshTargetsList();
}

void CompositorEditor::buildInputConnectors(Widget* list) {
	while(!m_compositor->getInputs().empty()) m_compositor->removeInput(m_compositor->getInputs()[0].name);
	for(Widget* w: list) m_compositor->addInput(w->getWidget<Textbox>("value")->getText());
}
void CompositorEditor::buildOutputConnectors(Widget* list) {
	while(!m_compositor->getOutputs().empty()) m_compositor->removeOutput(m_compositor->getOutputs()[0].name);
	for(Widget* w: list) m_compositor->addOutput(w->getWidget<Textbox>("value")->getText());
}

// ------------------------------------------------ //

void CompositorEditor::addBuffer(Button*) {
	Widget* buffers = m_panel->getWidget("bufferlist");
	addBufferWidget(buffers);
	refreshTargetsList();
}
void CompositorEditor::removeBuffer(Button* b) {
	b->getParent()->removeFromParent();
	delete b->getParent();
	refreshTargetsList();
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
	m_panel->getWidget("passlist")->add(w);
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

	// Make sure it is selected in graph list
	ListItem* item = m_graphList->findItem(1, g);
	if(item) m_graphList->selectItem(item->getIndex());
	else {
		m_graphList->addItem("Unknown", g);
		m_graphList->selectItem(m_graphList->getItemCount()-1);
	}


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
		else if(from->getCompositor()->getOutputs().size() == 1 && to->getCompositor()->getInputs().size() == 1) {
			m_graphEditor->link(from, 0, to, 0);
		}
		// All matching pairs
		else for(const Compositor::Connector& out: from->getCompositor()->getOutputs()) {
			int t = to->getCompositor()->getInput(out.name);
			int o = &out - from->getCompositor()->getOutputs().data();
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
	for(const Compositor::Connector& in : c->getInputs()) node->addInput(in.name);
	for(const Compositor::Connector& out : c->getOutputs()) node->addOutput(out.name);
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
	if(current && current->getWidth() > 0) size.set(current->getWidth(), current->getHeight());
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
		const char* out = link.out>=0? a->getOutputs()[link.out].name: nullptr;
		const char* in = link.in>=0? b->getInputs()[link.in].name: nullptr;

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

