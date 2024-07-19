#include <base/editor/nodeeditor.h>
#include <base/gui/skin.h>
#include <base/gui/renderer.h>
#include <base/input.h>
#include <cmath>

using namespace gui;
using namespace nodegraph;

NodeEditor::NodeEditor() {
}

NodeEditor::~NodeEditor() {
}

void NodeEditor::onMouseMove(const Point& last, const Point& pos, int b) {
	Widget::onMouseMove(last, pos, b);
	if(b && hasFocus()) {
		Point move = pos - last;
		for(Widget* w: *this) {
			w->setPosition(w->getPosition() + move);
		}
	}
	// Get link under mouse
	m_overLink = 0;
	if(!b && !m_dragLink) for(Widget* w: *this) {
		if(Node* n = cast<Node>(w)) {
			for(const Link& link: n->getLinks(OUTPUT)) {
				if(overLink(link,  pos, 10)) { m_overLink = &link; break; }
			}
		}
	}
}
void NodeEditor::onMouseButton(const Point& pos, int d, int u) {
	Widget::onMouseButton(pos, d, u);
	if(d==1) for(Widget* w: *this) w->setSelected(false);
	if(d==4 && m_overLink) unlink(const_cast<Link*>(m_overLink));
}

void NodeEditor::onKey(int code, wchar_t chr, KeyMask mask) {
	if(code == base::KEY_DEL) {
		int count = getWidgetCount();
		for(int i=0; i<count; ++i) {
			Node* node = cast<Node>(getWidget(i));
			if(node && node->isSelected() && node->m_canBeDeleted) {
				unlink(node);
				remove(node);
				if(eventDelete) eventDelete(node);
				delete node;
				--i;
			}
		}
		if(count!=getWidgetCount() && eventChanged) eventChanged(this);
	}
}

// =========================================================================== //

static void calculateControlPoints(Point p[4]) {
	p[1].set(p[0].x+60, p[0].y);
	p[2].set(p[3].x-60, p[3].y);
	if(p[1].x > p[3].x) {
		int m = abs(p[1].y - p[2].y);
		if(m<60) {
			p[1].x -= 60-m;
			p[2].x += 60-m;
		}
	}
}

static Point calculateBezierPoint(const Point* control, float t) {
	static const float factorial[] = {1,1,2,6,24,120,720}; // Factorial lookup table
	float p[2] = {0,0};
	for(int k=0; k<4; ++k) {
		float kt = t>0||k>0? powf(t,k): 1; // t^k
		float rt = t<1||k<3? powf(1-t,3-k): 1; // (1-t)^ik
		float ni = factorial[3] / (factorial[k]*factorial[3-k]);
		float bias = ni * kt * rt;
		p[0] += control[k].x * bias;
		p[1] += control[k].y * bias;
	}
	return Point(p[0]+0.5, p[1]+0.5);
}

Point NodeEditor::getConnectionPoint(const Connector& c) const {
	Point r = c.widget->getSize();
	r.y /= 2;
	if(!c.output) r.x = 0;
	for(Widget* w = c.widget; w!=this; w=w->getParent(true)) r += w->getPosition();
	return r;
}

void NodeEditor::getSplineControlPoints(const Link& link, Point points[4]) const {
	points[0] = getConnectionPoint(link.a->m_outputs[link.ia]);
	points[3] = getConnectionPoint(link.b->m_inputs[link.ib]);
	calculateControlPoints(points);
}

void NodeEditor::drawCurve(const Point* control, unsigned colour, float width, int size) const {
	Point out[128];
	float t = 0;
	float step = 1.0 / (size-1);
	for(int i=0; i<size; ++i) {
		if(i==size-1) t = 1; // fix floating point error
		out[i] = calculateBezierPoint(control, t);
		t += step;
	}
	getRoot()->getRenderer()->drawLineStrip(size, out, width, Point(0,0), colour);
}


void NodeEditor::draw() const {
	if(m_rect.width<=0 || m_rect.height<=0 || !isVisible()) return;
	if(m_skin) m_root->getRenderer()->drawSkin(m_skin, m_rect, m_colour, getState(), 0);
	
	getRoot()->getRenderer()->setTransform(m_derivedTransform);

	// Draw links
	Point bezier[4];
	for(Widget* w: *this) {
		Node* n = cast<Node>(w);
		for(const Link& link: n->getLinks(OUTPUT)) {
			uint colour = link.type < m_connectorTypes.size()? m_connectorTypes[link.type].colour: 0xffffffff;
			getSplineControlPoints(link, bezier);
			drawCurve(bezier, colour, m_overLink==&link? 2: 1);
		}
	}

	// Current link
	if(m_dragLink) {
		int k = m_dragLink->output * 3;
		bezier[k^3] = getConnectionPoint(*m_dragLink);
		bezier[k] = m_derivedTransform.untransform(getRoot()->getMousePos());
		calculateControlPoints(bezier);
		drawCurve(bezier, 0xffffffff, 2);
	}
	

	// Draw nodes
	drawChildren();
}

bool NodeEditor::overLink(const Link& link, const Point& pos, float distance) const {
	Point bezier[4];
	getSplineControlPoints(link, bezier);
	
	// AABB Test
	Rect r(bezier[0].x, bezier[0].y, 0, 0);
	r.include(bezier[1]);
	r.include(bezier[2]);
	r.include(bezier[3]);
	r.position() -= distance;
	r.size() += distance*2;
	if(!r.contains(pos)) return false;

	// Bezier test
	int size = 64;
	float t = 0;
	float step = 1.f / size;
	Point last = bezier[0];
	for(int i=1; i<=size; ++i) {
		if(i==size) t = 1; // fix floating point error
		Point p = calculateBezierPoint(bezier, t);
		if(p != last) {
			// closest point on line
			Point ab = p - last;
			Point am = pos - last;
			float t = (ab.x*am.x + ab.y*am.y) / (ab.x*ab.x + ab.y*ab.y);
			t = t<0?0: t>1?1: t;
			Point delta(last.x + ab.x * t - pos.x, last.y + ab.y * t - pos.y);

			if(abs(delta.x) + abs(delta.y) < distance) return true;
		}
		last = p;
		t += step;
	}
	
	return false;
}

// ================================================================================== //

void NodeEditor::setConnectorIconSet(IconList* set) {
	m_connectorIcons = set;
}

void NodeEditor::setConnectorType(unsigned type, int icon, unsigned colour, unsigned validMask) {
	while(m_connectorTypes.size() <= type) m_connectorTypes.push_back({0, 0xffffffff, 1u<<m_connectorTypes.size()});
	m_connectorTypes[type].icon = icon;
	m_connectorTypes[type].colour = colour | 0xff000000;
	m_connectorTypes[type].validMask = validMask;
}

bool NodeEditor::isValid(unsigned a, unsigned b) const {
	if(a == b) return true;
	uint mask = a < m_connectorTypes.size()? m_connectorTypes[a].validMask: 1u<<a;
	if(mask == 0) mask = 1 << a;
	return mask & 1 << b;
}

bool NodeEditor::validateConnection(const Connector* a, const Connector* b) const {
	if(!a || !b || a->output==b->output) return false;
	if(a->output && !isValid(a->type, b->type)) return false;
	if(b->output && !isValid(b->type, a->type)) return false;
	return true;
}

bool NodeEditor::link(Node* nodeA, int out, Node* nodeB, int in) {
	if(out<0 || out>=(int)nodeA->m_outputs.size()) return false;
	if(in<0 || in>=(int)nodeB->m_inputs.size()) return false;
	const Connector* from = &nodeA->m_outputs[out];
	const Connector* to = &nodeB->m_inputs[in];
	if(!validateConnection(to, from)) return false;

	// Link already exists
	for(Link* l: nodeA->m_links) {
		if(l->a==nodeA && l->b==nodeB && l->ia==from->index && l->ib==to->index) return true;
	}

	// Disconnect
	if(from->single) nodeA->disconnect(OUTPUT, from->index);
	if(to->single) nodeB->disconnect(INPUT, to->index);

	// Connect
	Link* link = new Link{ nodeA, nodeB, from->index, to->index, from->type }; 
	nodeA->m_links.push_back(link);
	nodeB->m_links.push_back(link);
	return true;
}

void NodeEditor::unlink(Node* a, int out, Node* b, int in) {
	for(size_t i=0; i<a->m_links.size(); ++i) {
		Link* link = a->m_links[i];
		if(link->a==a && link->b==b && link->ia==out && link->ib==in) {
			unlink(link);
			return;
		}
	}
}

void NodeEditor::unlink(Link* link) {
	auto erase = [](std::vector<Link*>& list, Link* item) {
		for(size_t i=0; i<list.size(); ++i) {
			if(list[i] == item) {
				list[i] = list.back();
				list.pop_back();
			}
		}
	};
	erase(link->a->m_links, link);
	erase(link->b->m_links, link);
	if(eventUnlinked) eventUnlinked(*link);
	delete link;
}

void NodeEditor::unlink(Node* node) {
	while(!node->m_links.empty()) unlink(node->m_links[0]);
}

int NodeEditor::areNodesConnected(const Node* a, const Node* b, unsigned mask, unsigned limit) {
	if(a==b) return 0;
	struct Item { const Node* node; unsigned depth; };
	std::vector<Item> m_list;
	auto add = [&m_list](const Node* node, unsigned depth) {
		for(const Item& i: m_list) if(i.node == node) return;
		m_list.push_back({node, depth});
	};
	add(a, 0);
	for(size_t i=0; i<m_list.size(); ++i) {
		if(m_list[i].depth > limit) break;
		if(m_list[i].node == b) return m_list[i].depth;
		for(const Link& link: m_list[i].node->getLinks(nodegraph::INPUT)) {
			if(mask & 1<<link.type) add(link.a, m_list[i].depth+1);
		}
		for(const Link& link: m_list[i].node->getLinks(nodegraph::OUTPUT)) {
			if(mask & 1 << link.type) add(link.b, m_list[i].depth+1);
		}
	}
	return 0;
}


// ================================================================================== //

Node::Node() : m_connectorTemplate{0,0} {
	setTangible(Tangible::ALL);
	m_connectorClient[0] = m_connectorClient[1] = this;
}

void Node::initialise(const Root* r, const PropertyMap& p) {
	Button::initialise(r, p);
	
	auto in = p.find("input");
	auto out = p.find("output");
	if(in!=p.end()) m_connectorTemplate[0] = r->getTemplate(in->value);
	if(out!=p.end()) m_connectorTemplate[1] = r->getTemplate(out->value);

	if(Widget* c = getTemplateWidget("_client1")) m_connectorClient[0] = m_connectorClient[1] = c;
	if(Widget* c = getTemplateWidget("_in")) m_connectorClient[0] = c;
	if(Widget* c = getTemplateWidget("_out")) m_connectorClient[1] = c;
}

void Node::copyData(const Widget* from) {
	if(const Node* node = cast<Node>(from)) {
		m_connectorTemplate[0] = node->m_connectorTemplate[0];
		m_connectorTemplate[1] = node->m_connectorTemplate[1];
		m_inputs = node->m_inputs;
		m_outputs = node->m_outputs;
		// Get new node connector widgets
		for(Widget* w: *node) {
			if(const Connector* c = node->getConnector(w)) {
				Widget* n = getWidget(w->getIndex());
				auto& list = c->output? m_outputs: m_inputs;
				list[c->index].widget = cast<Button>(n);
			}
		}
	}
}

void Node::setConnectorTemplates(const gui::Widget* input, const gui::Widget* output) {
	m_connectorTemplate[0] = input;
	m_connectorTemplate[1] = output;
}

NodeEditor* Node::getEditor() const {
	return cast<NodeEditor>(getParent());
}

Connector* Node::createConnector(const char* name, unsigned mode, unsigned type, bool single) {
	if(!m_connectorTemplate[mode]) { printf("Error: Graph connector templates not set\n"); return 0; }
	Widget* w = m_connectorTemplate[mode]->clone();
	if(!w) return 0;
	m_client = m_connectorClient[mode];
	std::vector<Connector>& list = mode? m_outputs: m_inputs;
	unsigned index = list.size();
	int step = w->getSize().y;
	int x = mode && m_connectorClient[0] == m_connectorClient[1]? getClientRect().width-w->getSize().x: 0;
	add(w, x, index * step);
	if(Label* l = cast<Label>(w)) l->setCaption(name);
	w->setName(name);
	m_client = this;
	refreshLayout();
	// Link type
	if(getEditor() && type < getEditor()->m_connectorTypes.size()) {
		if(Button* b = cast<Button>(w)) {
			b->setIcon(getEditor()->m_connectorTypes[type].icon);
			b->setIconColour(getEditor()->m_connectorTypes[type].colour);
		}
	}
	// Resize ?
	int diff = index*step+step - getClientRect().height;
	if(diff > 0) setSize( m_rect.width, m_rect.height+diff );
	// Events
	w->eventMouseDown.bind(this, &Node::connectorPress);
	w->eventMouseUp.bind(this, &Node::connectorRelease);
	w->eventMouseMove.bind(this, &Node::connectorDrag);
	// Connector
	list.push_back(Connector{w, mode, type, single?1u:0u, index });
	return &list.back();
}

void Node::addInput(const char* name, unsigned type, bool single) {
	createConnector(name, 0, type, single);
}

void Node::addOutput(const char* name, unsigned type, bool single) {
	createConnector(name, 1, type, single);
}

int Node::getConnectorCount(ConnectorMode mode) const {
	return mode? m_outputs.size(): m_inputs.size();
}

const char* Node::getConnectorName(ConnectorMode mode, unsigned index) const {
	const std::vector<Connector>& list = mode? m_outputs: m_inputs;
	return list[index].widget->getName();
}

unsigned Node::getConnectorType(ConnectorMode mode, unsigned index) const {
	const std::vector<Connector>& list = mode? m_outputs: m_inputs;
	return list[index].type;
}

bool Node::editConnector(ConnectorMode mode, unsigned index, const char* name, unsigned type, bool single) {
	std::vector<Connector>& list = mode==INPUT? m_inputs: m_outputs;
	if(index >= list.size()) return false;
	cast<Button>(list[index].widget)->setCaption(name);
	list[index].single = single;
	list[index].type = type;
	return true;
}

bool Node::editConnector(ConnectorMode mode, unsigned index, const char* name, unsigned type) {
	return editConnector(mode, index, name, type, mode==INPUT);
}

bool Node::removeConnector(ConnectorMode mode, unsigned index) {
	std::vector<Connector>& list = mode==INPUT? m_inputs: m_outputs;
	if(index>=list.size()) return false;
	disconnect(mode, index);
	delete list[index].widget;
	list.erase(list.begin() + index);
	for(Link* link: m_links) {
		if(mode==INPUT && link->b == this && link->ib > index) --link->ib;
		if(mode==OUTPUT && link->a == this && link->ia > index) --link->ia;
	}
	return true;
}

void Node::setScale(float s) {
	for(int i=0; i<getTemplateCount(); ++i) {
		if(Label* l = getTemplateWidget(i)->as<Label>()) {
			l->setFontSize(l->getSkin()->getFontSize() * s);
		}
	}
}

// -------------------------------------------------------------------- //

const Connector* Node::getConnector(Widget* w) const {
	for(const Connector& c: m_inputs) if(c.widget==w) return &c;
	for(const Connector& c: m_outputs) if(c.widget==w) return &c;
	return 0;
}

Widget* Node::getConnectorWidget(ConnectorMode mode, unsigned index) const {
	const std::vector<Connector>& list = mode? m_outputs: m_inputs;
	return list[index].widget;
}

const Connector* Node::getTarget(Widget* src, const Point& p) const {
	// p is local to w
	Point pos = p;
	for(Widget* w=src; w!=getEditor(); w=w->getParent(true)) pos += w->getPosition();
	if(Widget* w = getEditor()->getWidget(pos)) {
		Node* node = getNodeFromWidget(w);
		if(!node || node == this) return nullptr;
		return node->getConnector(w);
	}
	return 0;
}

int Node::disconnect(ConnectorMode mode, size_t index) {
	std::vector<Link*> rm;
	for(const Link& link : getLinks(mode, index)) rm.push_back(const_cast<Link*>(&link));
	for(Link* link : rm) getEditor()->unlink(link);
	return rm.size();
}

int Node::isLinked(ConnectorMode mode, int index) const {
	int count = 0;
	for(const Link& link: getLinks(mode, index)) (void)link, ++count;
	return count;
}

Node* Node::getNodeFromWidget(Widget* w) {
	while(w) {
		if(Node* node = cast<Node>(w)) return node;
		w = w->getParent();
	}
	return nullptr;
}

void Node::connectorPress(Widget* w, const Point& p, int b) {
	const Connector* c = getConnector(w);

	// If single and connected - move existing link
	if(c->single) {
		Link* link = 0;
		for(const Link& l: getLinks((ConnectorMode)c->output, c->index)) link = const_cast<Link*>(&l);
		if(link) {
			Connector* other = nullptr;
			if(c->output) other = &link->b->m_inputs[link->ib];
			else other = &link->a->m_outputs[link->ia];
			getEditor()->unlink(link);
			getEditor()->m_dragLink = other;
			other->widget->setMouseFocus();
			other->widget->setFocus();
			if(getEditor()->eventChanged) getEditor()->eventChanged(getEditor());
			return;
		}
	}

	// Fade out invalid targets
	for(Widget* w: getParent()) {
		for(Connector& t: cast<Node>(w)->m_inputs) {
			t.widget->setAlpha(getEditor()->validateConnection(c, &t)? 1: 0.4);
		}
		for(Connector& t: cast<Node>(w)->m_outputs) {
			t.widget->setAlpha(getEditor()->validateConnection(c, &t)? 1: 0.4);
		}
	}

	getEditor()->m_dragLink = c;
}

void Node::connectorRelease(Widget* w, const Point& p, int b) {
	for(Widget* w: getParent()) {
		for(Connector& t: cast<Node>(w)->m_inputs) t.widget->setAlpha(1);
		for(Connector& t: cast<Node>(w)->m_outputs) t.widget->setAlpha(1);
	}
	getEditor()->m_dragLink = nullptr;
	const Connector* to = getTarget(w, p);
	const Connector* from = getConnector(w);
	if(!getEditor()->validateConnection(to, from)) return;
	
	// Make sure they are the right way round
	if(from && !from->output) { 
		const Connector* tmp = from;
		from = to, to = tmp;
	}

	// Get nodes from connectors
	Node* nodeA = getNodeFromWidget(from->widget);
	Node* nodeB = getNodeFromWidget(to->widget);

	if(getEditor()->link(nodeA, from->index, nodeB, to->index)) {
		if(getEditor()->eventLinked) getEditor()->eventLinked({nodeA, nodeB, from->index, to->index, from->type});
		if(getEditor()->eventChanged) getEditor()->eventChanged(getEditor());
	}
}

void Node::connectorDrag(Widget* w, const Point& p, int b) {
	/*
	if(!b) return;
	const Connector* to = getTarget(w, p);
	bool valid = getEditor()->validateConnection(to, getConnector(w));
	// change something
	static Widget* last = nullptr;
	if(last) last->setColour(0xffffff);
	if(to) to->widget->setColour( valid? 0xffffff: 0x606060 );
	last = to? to->widget: nullptr;
	*/
}

// -------------------------------------------------------------------- //
void Node::onKey(int code, wchar_t chr, KeyMask mask) {
	Super::onKey(code, chr, mask);
	getEditor()->onKey(code, chr, mask);
}
void Node::onMouseButton(const Point& p, int d,int u) {
	if(d==1) m_heldPoint = p;
	if(u) m_heldPoint.x = 0;

	if(!isSelected() && !(getRoot()->getKeyMask() & KeyMask::Shift)) {
		for(Widget* w : *getEditor()) if(w!=this) w->setSelected(false);
	}
	setSelected(true);
	Super::onMouseButton(p, d, u);
}

void Node::onMouseMove(const Point& l, const Point& p, int b) {
	if(b==1 && m_heldPoint.x!=0) {
		Point delta = p - m_heldPoint;
		for(Widget* w : *getEditor()) if(w->isSelected()) w->setPosition( w->getPosition() + delta );
	}
	Super::onMouseMove(l,p,b);
}

void Node::forceStartDrag() {
	m_heldPoint = getSize();
	m_heldPoint.x /= 2;
	m_heldPoint.y /= 2;
	setPosition(getRoot()->getMousePos() - m_heldPoint - getParent()->getAbsolutePosition());
	setMouseFocus();
}

// -------------------------------------------------------------------- //
LinkIterable Node::getLinks(ConnectorMode m) const {
	return LinkIterable(this, m);
}

LinkIterable Node::getLinks(ConnectorMode m, unsigned index) const {
	return LinkIterable(this, m, index);
}

