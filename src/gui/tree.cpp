#include <base/gui/tree.h>
#include <base/gui/widgets.h>
#include <base/gui/renderer.h>
#include <base/gui/skin.h>
#include <base/gui/font.h>

using namespace gui;

TreeNode::TreeNode(const char* text)
	: m_treeView(0), m_parent(0), m_selected(false), m_expanded(false), m_depth(0), m_displayed(1), m_cached(0) {
	if(text) m_data.push_back( Any(String(text)) );
}

TreeNode::~TreeNode() {
	if(m_treeView && m_cached) m_treeView->removeCache(this);
	for(uint i=0; i<m_children.size(); ++i) delete m_children[i];
}
size_t TreeNode::size() const { return m_children.size(); }
TreeNode* TreeNode::getParent() const { return m_parent; }
TreeNode* TreeNode::operator[](int i) { return m_children[i]; }
TreeNode* TreeNode::at(int i) { return m_children.at(i); }
TreeNode* TreeNode::back() { return m_children.empty()? 0: m_children.back(); }
TreeNode* TreeNode::add( const char* text, const Any& data ) {
	TreeNode* node = add(new TreeNode( String(text) ));
	if(!data.isNull()) node->setData(1, data);
	return node;
}
TreeNode* TreeNode::add( TreeNode* node ) {
	node->setParentNode(this);
	m_children.push_back(node);
	if(m_expanded) changeDisplayed(m_displayed + node->m_displayed);
	return node;
}
TreeNode* TreeNode::insert(int index, const char* text) { return insert(index, new TreeNode( String(text) )); }
TreeNode* TreeNode::insert(int index, TreeNode* node) {
	node->setParentNode(this);
	m_children.insert(m_children.begin() + index, node);
	if(m_expanded) changeDisplayed(m_displayed + node->m_displayed);
	return node;
}
void TreeNode::setParentNode(TreeNode* parent) {
	if(m_parent && m_parent!=parent) m_parent->remove(this);
	m_treeView = parent->m_treeView;
	m_parent   = parent;
	m_depth    = parent->m_depth + 1;
	m_selected = false;
	for(uint i=0; i<m_children.size(); ++i) m_children[i]->setParentNode(this);
	if(m_treeView) m_treeView->m_needsUpdating = true;
}
TreeNode* TreeNode::remove(int index) {
	if(index<0 || index>=(int)m_children.size()) return 0;
	TreeNode* node = m_children[index];
	if(m_expanded) changeDisplayed(m_displayed - node->m_displayed);
	if(m_treeView->m_selectedNode == node) m_treeView->m_selectedNode = 0;
	if(m_cached) m_treeView->removeCache(this);
	node->m_parent = 0;
	node->m_treeView = 0;
	m_children.erase( m_children.begin() + index );
	if(m_children.empty() && (m_parent || !m_treeView->m_hideRootNode)) m_expanded = false;
	if(m_treeView) m_treeView->m_needsUpdating = true;
	return node;
}
TreeNode* TreeNode::remove(TreeNode* node) {
	return remove( node->getIndex() );
}
TreeNode* TreeNode::remove() {
	if(m_parent) return m_parent->remove(this);
	return 0;
}

int TreeNode::getIndex() const {
	if(!m_parent) return -1;
	const std::vector<TreeNode*>& list = m_parent->m_children;
	for(uint i=0; i<list.size(); ++i) if(list[i]==this) return (int)i;
	return -1;
}
void TreeNode::clear() {
	if(m_children.empty()) return;
	for(uint i=0; i<m_children.size(); ++i) delete m_children[i];
	m_children.clear();
	if(m_parent || !m_treeView->m_hideRootNode)
		m_expanded = false;
	changeDisplayed(1);
	if(m_treeView) {
		m_treeView->m_needsUpdating = true;
		m_treeView->m_selectedNode = 0;
	}
}

void TreeNode::changeDisplayed(int display) {
	for(TreeNode* n = m_parent; n; n=n->m_parent) {
		if(!n->isExpanded()) break;
		n->m_displayed += display - m_displayed;
	}
	m_displayed = display;
}

void TreeNode::select() {
	if(m_treeView) {
		m_treeView->clearSelection();
		m_treeView->m_selectedNode = this;
		if(m_cached) m_cached->setSelected(true);
		m_selected = true;
	}
}

void TreeNode::expand(bool e) {
	bool isHiddenRootNode = !m_parent && m_treeView && m_treeView->m_hideRootNode;
	if(isHiddenRootNode) e = true;
	else if(m_children.empty()) e = false;
	if(m_expanded == e) return;
	m_expanded = e;
	int display = isHiddenRootNode? 0: 1;
	if(e) for(TreeNode* n: *this) display += n->m_displayed;
	else for(TreeNode* n: *this) n->m_cached = 0;
	changeDisplayed(display);
	if(m_treeView) m_treeView->m_needsUpdating = true;
}
void TreeNode::expandAll() {
	expand(true);
	for(uint i=0; i<m_children.size(); ++i) m_children[i]->expandAll();
}
void TreeNode::ensureVisible() {
	if(m_parent) {
		m_parent->expand(true);
		m_parent->ensureVisible();
	}
}
bool TreeNode::isExpanded() const { return m_expanded; }
bool TreeNode::isSelected() const { return m_selected; }
const Any& TreeNode::getData(size_t i) const {
	if(i<m_data.size()) return m_data[i];
	static const Any nothing;
	return nothing;
}
void TreeNode::setData(size_t i, const Any& data) {
	if(m_data.size() <= i) m_data.resize(i+1, Any());
	m_data[i] = data;
}
void TreeNode::setData(const Any& data) { setData(0, data); }
void TreeNode::setText(const char* text) { setData(0, String(text)); }
const char* TreeNode::getText(size_t index) const {
	static const String emptyString;
	const Any& value = getData(index);
	const char* text = value.getValue<const char*>(0);
	if(!text) text = value.getValue<char*>(0);
	if(!text) text = value.getValue(emptyString);
	return text;
}

void TreeNode::refresh() {
	if(m_cached) m_treeView->cacheItem(this, m_cached);
}

std::vector<TreeNode*>::const_iterator TreeNode::begin() const { return m_children.begin(); }
std::vector<TreeNode*>::const_iterator TreeNode::end() const { return m_children.end(); }

// --------------------------------------------------------------------------------------------------- //

TreeView::TreeView(const Rect& r, Skin* s) : Widget(r, s), m_hideRootNode(false), m_lineColour(0xff808080), m_rootNode(0), m_selectedNode(0), m_needsUpdating(true) {
	setRootNode( new TreeNode("root") );
}
TreeView::~TreeView() {
	delete m_rootNode;
}

TreeNode* TreeView::getRootNode() { return m_rootNode; }
TreeNode* TreeView::getSelectedNode() { return m_selectedNode; }
void TreeView::expandAll() { m_rootNode->expandAll(); }
void TreeView::setRootNode(TreeNode* node) {
	if(m_rootNode == node) return;
	if(node->m_parent) node->m_parent->remove(node);
	if(m_rootNode) delete m_rootNode;
	m_rootNode = node;
	m_rootNode->m_treeView = this;
	for(uint i=0; i<node->m_children.size(); ++i) node->m_children[i]->setParentNode(node);
	if(m_hideRootNode) node->expand(true);
}
void TreeView::showRootNode(bool s) {
	m_needsUpdating |= s==m_hideRootNode;
	m_hideRootNode = !s;
	if(m_hideRootNode) m_rootNode->expand(true);
}

void TreeView::initialise(const Root*, const PropertyMap& p) {
	// sub widgets
	m_scrollbar = getTemplateWidget<Scrollbar>("_scroll");
	m_client = getTemplateWidget("_client");
	if(!m_client) m_client = this;
	if(m_scrollbar) m_scrollbar->eventChanged.bind(this, &TreeView::scrollChanged);
	if(m_client!=this) m_client->setTangible(Tangible::CHILDREN);

	// Client must not have a skin as stuff is drawn under it.
	if(m_client!= this) m_client->setSkin(0);

	// Get item template widgets
	m_expand = getTemplateWidget("_expand");
	Widget* item = getWidget("_item");
	if(!item) item = getTemplateWidget("_item");

	if(m_expand) m_expand->setVisible(false);
	if(item) {
		item->setVisible(false);
		m_itemHeight = item->getSize().y;
		for(ItemWidget& w: m_itemWidgets) {
			w.widget->setVisible(false);
			if(w.node) w.node->m_cached = 0;
		}
		m_itemWidgets.clear();
		// Wrong place ?
		if(getClientWidget()->getWidgetCount() == 0) {
			item = item->clone();
			add(item);
		}
		m_itemWidgets.push_back(ItemWidget{item,0});
		bindEvents(item);
	}
	else if(m_skin->getFont()) {
		m_itemHeight = m_skin->getFont()->getSize("X", m_skin->getFontSize()).y;
	}
	m_hideRootNode = false;
	m_indent = 20;

	char* e = 0;
	if(p.contains("hideroot") && atoi(p["hideroot"])) showRootNode(false);
	if(p.contains("indent")) m_indent = atoi(p["indent"]);
	if(p.contains("lines") && p["lines"][0]=='#') m_lineColour = strtol(p["lines"]+1, &e, 16);
	m_lineColour |= 0xff000000;
	m_needsUpdating = true;
}
Widget* TreeView::clone(const char* type) const {
	Widget* w = Widget::clone(type);
	TreeView* t = w->cast<TreeView>();
	if(t) {
		t->m_lineColour = m_lineColour;
		t->m_indent = m_indent;
		t->m_hideRootNode = m_hideRootNode;
	}
	return w;
}

bool TreeView::onMouseWheel(int w) {
	if(m_scrollbar) {
		m_scrollbar->setValue( m_scrollbar->getValue() - w * m_scrollbar->getStep() );
		m_needsUpdating = true;
		Widget::onMouseWheel(w);
	}
	return m_scrollbar;
}
void TreeView::onMouseButton(const Point& p, int d, int u) {
	if(m_needsUpdating) updateCache();
	// Single selection for now. ToDo: mutli selection modes
	Point m = p;
	m.y -= m_cacheOffset;
	m_zip = 0;
	int index = m.y / m_itemHeight;
	if(index >= 0 && index < (int)m_drawCache.size()) {
		TreeNode* node = m_drawCache[index].node;
		m.y -= (m.y/m_itemHeight)*m_itemHeight;
		m.x -= node->m_depth * m_indent;
		if(m_hideRootNode) m.x += m_indent;

		// Expand button
		if(d==1) {
			if(m_expand && node->size()>0 && m_expand->getRect().contains(m)) {
				node->expand( !node->isExpanded() );
				if(eventExpanded && node->isExpanded()) eventExpanded(this, node);
				else if(eventCollapsed && !node->isExpanded()) eventCollapsed(this, node);
				m_zip = node->isExpanded()? 1: 2;
			}
			// Selection
			else if(m.x > 0 && !node->isSelected()) {
				if(eventSelected) eventSelected(this, node);
				node->select();
			}
		}

		// Generic clicked event
		if(d && eventPressed && (!m_expand || !m_expand->getRect().contains(m))) {
			eventPressed(this, node, d);
		}

	}
	else if(m_selectedNode && d==1) {
		if(eventSelected) eventSelected(this, 0);
		clearSelection();
	}
	Widget::onMouseButton(p, d, u);
}

void TreeView::onMouseMove(const Point& a, const Point& p, int b) {
	if(m_zip) {
		Point m = p;
		m.y -= m_cacheOffset;
		int index = m.y / m_itemHeight;
		if(index >= 0 && index < (int)m_drawCache.size()) {
			bool expand = m_zip == 1;
			TreeNode* node = m_drawCache[index].node;
			if(node->size() && node->isExpanded() != expand) {
				m.y -= (m.y/m_itemHeight)*m_itemHeight;
				m.x -= node->m_depth * m_indent;
				if(m_hideRootNode) m.x += m_indent;
				if(m_expand->getRect().contains(m)) {
					node->expand(expand);
					if(eventExpanded && node->isExpanded()) eventExpanded(this, node);
					else if(eventCollapsed && !node->isExpanded()) eventCollapsed(this, node);
				}
			}
		}
	}
}

void TreeView::setSize(int w, int h) {
	Widget::setSize(w,h);
	m_needsUpdating = true;
}

void TreeView::clearSelection() {
	if(m_selectedNode) {
		if(m_selectedNode->m_cached) m_selectedNode->m_cached->setSelected(false);
		m_selectedNode->m_selected = false;
		m_selectedNode = 0;
	}
}

void TreeView::scrollChanged(Scrollbar* s, int v) {
	m_needsUpdating = true;
}

void TreeView::scrollToItem(TreeNode* n) {
	// is it visible
	for(uint i=0; i<m_drawCache.size(); ++i) {
		if(m_drawCache[i].node == n) return;
	}
	
	// Anything need expanding?
	TreeNode* p = n->m_parent;
	while(p) {
		p->expand(true);
		p = p->m_parent;
	}
	if(m_needsUpdating) updateCache();

	// Find vertical position
	int pos = 0;
	p = n->m_parent;
	while(p) {
		++pos;
		int ix = n->getIndex();
		for(int i=0; i<ix; ++i) pos += p->at(i)->m_displayed;
		n = p; p = n->m_parent;
	}
	if(m_hideRootNode) --pos;

	// Scroll to location
	if(!m_scrollbar) return;
	int top = pos * m_itemHeight;
	int btm = top + m_itemHeight;
	int height = m_client->getSize().y;
	int offset = m_scrollbar->getValue();
	if(top < offset) m_scrollbar->setValue(top);
	else if(btm - height > offset) m_scrollbar->setValue(btm - height);
}

void TreeView::updateCache() const {
	// Update scrollbar
	if(m_scrollbar) {
		int h = m_client->getSize().y;
		int t = m_itemHeight * m_rootNode->m_displayed;
		int max = t>h? t-h: 0;
		m_scrollbar->setRange(0, max, 16);
		m_scrollbar->setBlockSize(max>0? (float)h/t: 1);
	}
	// Items
	int offset = m_scrollbar? m_scrollbar->getValue(): 0;
	int height = m_client->getSize().y;
	// Build cache
	m_drawCache.clear();
	m_additionalLines.clear();
	for(ItemWidget& w: m_itemWidgets) w.widget->setVisible(false);
	if(m_hideRootNode) {
		for(uint i=0, h=0; i<m_rootNode->size(); ++i)
			h += buildCache(m_rootNode->at(i), h, offset, offset + height);
	}
	else buildCache(m_rootNode, 0, offset, offset + height);
	m_cacheOffset = (offset / m_itemHeight) * m_itemHeight - offset;

	// Cache widgets
	if(!m_itemWidgets.empty()) {
		std::vector<TreeNode*> missing;
		// Reuse existing widgets
		for(CacheItem& i: m_drawCache) {
			if(i.node->m_cached) i.node->m_cached->setVisible(true);
			else missing.push_back(i.node);
		}
		// Free unused widgets
		for(ItemWidget& i: m_itemWidgets) {
			if(!i.widget->isVisible() && i.node) {
				i.node->m_cached = 0;
				i.node = 0;
			}
		}
		// Allocate new widgets
		size_t availiable = 0;
		for(TreeNode* n: missing) {
			while(availiable<m_itemWidgets.size() && m_itemWidgets[availiable].node) ++availiable;
			if(availiable<m_itemWidgets.size()) {
				n->m_cached = m_itemWidgets[availiable].widget;
				m_itemWidgets[availiable].node = n;
			}
			else {
				n->m_cached = m_itemWidgets[0].widget->clone();
				m_itemWidgets.push_back( ItemWidget{n->m_cached,n} );
				m_client->add(n->m_cached);
				const_cast<TreeView*>(this)->bindEvents(n->m_cached);
			}
			int indent = n->m_depth * m_indent + m_indent;
			n->m_cached->setVisible(true);
			n->m_cached->setSelected(n->m_selected);
			if(m_hideRootNode) indent -= m_indent;
			if((n->m_cached->getAnchor() & 0xf) == 0x5)
				n->m_cached->setSize(getAbsoluteClientRect().width - indent, m_itemHeight);
			cacheItem(n, n->m_cached);
			++availiable;
		}
		// Set positions
		for(size_t i=0; i<m_drawCache.size(); ++i) {
			TreeNode* n = m_drawCache[i].node;
			int indent = n->m_depth * m_indent + m_indent;
			if(m_hideRootNode) indent -= m_indent;
			n->m_cached->setPosition(indent, i * m_itemHeight + m_cacheOffset);
		}
	}

	m_needsUpdating = false;
}

int TreeView::buildCache(TreeNode* n, int y, int top, int bottom) const {
	if(y + n->m_displayed * m_itemHeight < top) return n->m_displayed * m_itemHeight;
	if(y > bottom) return 0;

	// Add to cache
	if(y + m_itemHeight > top) {
		m_drawCache.push_back( CacheItem{n,0} );
	}

	// Recurse to children
	int h = m_itemHeight;
	if(n->isExpanded()) {
		int line = 0;
		int lineIndex = 0;
		for(uint i=0; i<n->size(); ++i) {
			line = h;
			lineIndex = m_drawCache.size();
			h += buildCache(n->at(i), y+h, top, bottom);
		}
		if(line) {
			if(lineIndex < (int)m_drawCache.size()) {
				m_drawCache[lineIndex].up = line - m_itemHeight/2;
			}
			else {
				m_additionalLines.push_back( Point((n->m_depth+1) * m_indent, line - m_itemHeight/2) );
			}
		}
	}
	return h;
}

inline void setFromData(Widget* w, const Any& value) {
	if(value.isNull()) return;
	if(w->getType() == Checkbox::staticType()) {
		w->cast<Checkbox>()->setChecked( value.getValue(false) );
		return;
	}
	const char* text = value.getValue<const char*>(0);
	if(!text) text = value.getValue<String>(0);
	if(w->isType(Label::staticType()))
		w->cast<Label>()->setCaption(text? text: "");
	else if(w->isType(Textbox::staticType()))
		w->cast<Textbox>()->setText(text);
	else if(w->isType(Icon::staticType())) {
		if(text) w->cast<Icon>()->setIcon(text);
		else w->cast<Icon>()->setIcon( value.getValue<int>(-1) );
	}
}

inline int treeItemSubWidgetIndex(Widget* w) {
	if(w->getName()[0]=='_' && w->getName()[1]>='0' && w->getName()[2]==0) return w->getName()[1]-'0';
	else return -1;
}

void TreeView::cacheItem(TreeNode* node, Widget* w) const {
	if(eventCacheItem) eventCacheItem(node, w);
	else {
		// Automatic version.
		if(w->getWidgetCount() == 0) {
			setFromData(w, node->getData(0));
		}
		else for(Widget* c: *w) {
			int subItem = treeItemSubWidgetIndex(c);
			if(subItem>=0) setFromData(c, node->getData(subItem));
		}
	}
}

void TreeView::refresh() {
	for(CacheItem& i: m_drawCache) {
		cacheItem(i.node, i.node->m_cached);
	}
}

void TreeView::removeCache(TreeNode* node) {
	for(TreeView::ItemWidget& i: m_itemWidgets) {
		if(i.node==node) {
			i.node = 0;
			node->m_cached = 0;
			for(TreeNode* n: node->m_children) removeCache(n);
			return;
		}
	}
}

void TreeView::bindEvents(Widget* item) {
	if(item->cast<Button>()) item->cast<Button>()->eventPressed.bind(this, &TreeView::buttonEvent);
	if(item->cast<Checkbox>()) item->cast<Checkbox>()->eventChanged.bind(this, &TreeView::buttonEvent);
	if(item->cast<Textbox>()) item->cast<Textbox>()->eventSubmit.bind(this, &TreeView::textEvent);
	for(Widget* w: *item) bindEvents(w);
}

TreeNode* TreeView::findNode(Widget* w) {
	while(w->getParent() != this && w->getParent() != getClientWidget()) w = w->getParent();
	for(ItemWidget& i: m_itemWidgets)
		if(i.widget == w) return i.node;
	return 0;
}
void TreeView::buttonEvent(Button* b) {
	TreeNode* node = findNode(b);
	if(node) {
		if(!eventCacheItem && b->cast<Checkbox>() && treeItemSubWidgetIndex(b)>=0 ) {
			node->setData(treeItemSubWidgetIndex(b), b->cast<Checkbox>()->isChecked());
		}
		if(eventCustom) eventCustom(this, node, b);
	}
}
void TreeView::textEvent(Textbox* t) {
	TreeNode* node = findNode(t);
	if(node) {
		if(!eventCacheItem && treeItemSubWidgetIndex(t)>=0 ) {
			node->setData(treeItemSubWidgetIndex(t), String(t->getText()));
		}
		if(eventCustom) eventCustom(this, node, t);
	}
}

void TreeView::draw() const {
	if(!isVisible()) return;
	drawSkin();

	if(m_needsUpdating) updateCache();
	if(m_drawCache.empty()) return;

	// Set up item rect
	Renderer* renderer = m_root->getRenderer();
	Transform parentTransform = renderer->getTransform();
	Rect r = m_client->getRect();
	renderer->setTransform(m_derivedTransform);
	renderer->push(r);
	r.height = m_itemHeight;
	r.y += m_cacheOffset;
	if(m_hideRootNode) r.x -= m_indent;

	// Draw lines
	Rect line;
	Point offset(3, m_itemHeight/2);
	if(m_expand) {
		offset = m_expand->getPosition();
		offset.x += m_expand->getSize().x / 2;
		offset.y += m_expand->getSize().y / 2;
	}
	for(uint i=0; i<m_drawCache.size(); ++i) {
		int indent = m_drawCache[i].node->m_depth * m_indent;
		line.set(r.x + offset.x + indent, r.y + i*m_itemHeight + offset.y, m_indent - offset.x, 1);
		renderer->drawRect(line, m_lineColour);
		// Vertical lines
		if(m_drawCache[i].up) {
			line.set(line.x, line.y - m_drawCache[i].up, 1, m_drawCache[i].up);
			renderer->drawRect(line, m_lineColour);
		}
	}
	// Add lines connecting items further down
	for(uint i=0; i<m_additionalLines.size(); ++i) {
		line.set(r.x + offset.x + m_additionalLines[i].x, 0, 1, m_additionalLines[i].y);
		line.y = r.y + m_drawCache.size() * m_itemHeight - line.height;
		renderer->drawRect(line, m_lineColour);

	}

	// Draw expanders
	if(m_expand) {
		Point offset = m_expand->getPosition();
		Rect exp( offset, m_expand->getSize() );
		for(uint i=0; i<m_drawCache.size(); ++i) {
			if(m_drawCache[i].node->size()) {
				exp.x = r.x + m_drawCache[i].node->m_depth * m_indent + offset.x;
				exp.y = r.y + i * m_itemHeight + offset.y;
				int state = m_drawCache[i].node->isExpanded()? 4: 0;	// ToDo: other states
				renderer->drawSkin(m_expand->getSkin(), exp, m_expand->getColourARGB(), state);
			}
		}
	}


	// Item Text
	if(m_itemWidgets.empty()) {
		// Item text
		static const String emptyString;
		Rect exp(0, 0, m_rect.width, m_itemHeight);
		for(uint i=0; i<m_drawCache.size(); ++i) {
			const char* text = m_drawCache[i].node->getText(0);
			if(text) {
				exp.x = r.x + m_drawCache[i].node->m_depth * m_indent + m_indent;
				exp.y = r.y + i * m_itemHeight;
				int state = m_drawCache[i].node->isSelected()? 4: 0;	// ToDo: other state
				renderer->drawText(exp.position(), text, 0, getSkin(), state);
			}
		}
	}
	renderer->pop();
	renderer->setTransform(parentTransform);


	drawChildren();
}

