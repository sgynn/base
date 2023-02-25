#include <base/gui/lists.h>
#include <base/gui/widgets.h>
#include <base/gui/renderer.h>
#include <base/gui/skin.h>
#include <cstdio>

using namespace gui;

// --------------------------------------------------------------------------------------------------- //

ItemList::ItemList() {
	m_items = new std::vector<Item>;
	m_selected = new std::vector<uint>;
	m_shared = 0;
}
ItemList::~ItemList() {
	dropList();
}

void ItemList::shareList(ItemList* src) {
	if(src && src->m_items == m_items) return;	// Already sharing
	if(!src && !m_shared) return;

	std::vector<ItemList*>* oldShare = m_shared && m_shared->size()>1? m_shared: 0;

	// Delete existing list
	dropList();

	// Use list from src
	if(src) {
		m_items = src->m_items;
		m_selected = src->m_selected;
		
		if(!src->m_shared) {
			src->m_shared = new std::vector<ItemList*>;
			src->m_shared->push_back(src);
		}

		m_shared = src->m_shared;
		m_shared->push_back(this);
	}
	// Recreate own list
	else {
		m_items = new std::vector<Item>;
		m_selected = new std::vector<uint>;
	}
	countChanged();
	selectionChanged();

	// if already sharing, change them all (recursive)
	if(oldShare) oldShare->at(0)->shareList(src);

}

void ItemList::dropList() {
	if(!m_shared || m_shared->size()==1) {
		clearItems();
		delete m_items;
		delete m_selected;
		delete m_shared;
	} else {
		for(size_t i=0; i<m_shared->size(); ++i) {
			if(m_shared->at(i) == this) {
				m_shared->erase(m_shared->begin()+i);
				break;
			}
		}
	}
	m_shared = 0;
	m_items = 0;
	m_selected = 0;
}

void ItemList::countChanged() {
	if(m_shared) for(size_t i=0; i<m_shared->size(); ++i) m_shared->at(i)->itemCountChanged();
	else itemCountChanged();
}
void ItemList::selectionChanged() {
	if(m_shared) for(size_t i=0; i<m_shared->size(); ++i) m_shared->at(i)->itemSelectionChanged();
	else itemSelectionChanged();
}

void ItemList::clearItems() {
	for(uint i=0; i<m_items->size(); ++i) {
		free(m_items->at(i).name);
	}
	m_items->clear();
	m_selected->clear();
	countChanged();
}

void ItemList::addItem(const char* name, const Any& data, int icon) {
	m_items->push_back( Item() );
	m_items->back().name = name? strdup(name): 0;
	m_items->back().data = data;
	m_items->back().icon = icon;
	m_items->back().selected = false;
	m_items->back().checked = false;
	countChanged();
}

void ItemList::insertItem(uint index, const char* name, const Any& data, int icon) {
	Item itm;
	itm.name = name? strdup(name): 0;
	itm.data = data;
	itm.icon = icon;
	itm.selected = false;
	itm.checked = false;
	m_items->insert( m_items->begin() + index, itm );
	countChanged();
}

void ItemList::removeItem(uint index) {
	if(index < m_items->size()) {
		free(m_items->at(index).name);
		m_items->erase( m_items->begin() + index );
		// Update selection list
		uint erase = ~0u;
		for(uint i=0; i<m_selected->size(); ++i) {
			if(m_selected->at(i) > index) --m_selected->at(i);
			else if(m_selected->at(i) == index) erase = i;
		}
		if(erase < m_selected->size()) m_selected->erase( m_selected->begin() + erase );
		countChanged();
	}
}

void ItemList::setItemName(uint index, const char* name) {
	if(index < m_items->size()) {
		free(m_items->at(index).name);
		m_items->at(index).name = strdup(name);
	}
}
void ItemList::setItemData(uint index, const Any& data) {
	if(index < m_items->size()) {
		m_items->at(index).data = data;
	}
}

void ItemList::setItemChecked(uint index, bool c) {
	if(index < m_items->size()) {
		m_items->at(index).checked = c;
	}
}

void ItemList::setItemIcon(uint index, int icon) {
	if(index < m_items->size()) {
		m_items->at(index).icon = index;
	}
}

uint ItemList::getItemCount() const {
	return m_items->size();
}

const char* ItemList::getItem(uint index) const {
	return index<m_items->size()? m_items->at(index).name: 0;
}
const Any& ItemList::getItemData(uint index) const {
	static const Any NullAny;
	return index<m_items->size()? m_items->at(index).data: NullAny;
}
bool ItemList::getItemChecked(uint index) const {
	return index<m_items->size()? m_items->at(index).checked: false;
}
int ItemList::getItemIcon(uint index) const {
	return index<m_items->size()? m_items->at(index).icon: -1;
}

int ItemList::findItem(const char* name) const {
	for(uint i=0; i<m_items->size(); ++i) {
		if(strcmp(m_items->at(i).name, name)==0) return i;
	}
	return -1;
}


void ItemList::clearSelection() {
	for(uint i=0; i<m_selected->size(); ++i) m_items->at( m_selected->at(i) ).selected = false;
	m_selected->clear();
}
void ItemList::selectItem(uint index, bool only) {
	if(only) clearSelection();
	if(index>=m_items->size() || isItemSelected(index)) return;
	m_items->at(index).selected = true;
	m_selected->push_back(index);
}
void ItemList::deselectItem(uint index) {
	if(index<m_items->size() && m_items->at(index).selected) {
		m_items->at(index).selected = false;
		for(uint i=0; i<m_selected->size(); ++i) {
			if(m_selected->at(i) == index) {
				m_selected->erase(m_selected->begin()+i);
				break;
			}
		}
	}
}

bool ItemList::isItemSelected(uint index) const {
	return index < m_items->size() && m_items->at(index).selected;
}

uint ItemList::getSelectionSize() const {
	return m_selected->size();
}

const char* ItemList::getSelectedItem(uint s) const {
	return s<m_selected->size()? m_items->at( m_selected->at(s) ).name: 0;
}
const Any& ItemList::getSelectedData(uint s) const {
	static const Any NullAny;
	return s<m_selected->size()? m_items->at( m_selected->at(s) ).data: NullAny;
}
int ItemList::getSelectedIndex(uint s) const {
	return s<m_selected->size()? m_selected->at(s): -1;
}
int ItemList::getSelectedIcon(uint s) const {
	return s<m_selected->size()? m_items->at( m_selected->at(s) ).icon: -1;
}


void ItemList::initialise(const PropertyMap& p) {
	// Read items from propertymap
	if(p.contains("count")) {
		int count = atoi( p["count"] );
		char index[6];
		for(int i=0; i<=count; ++i) {
			sprintf(index, "%d", i);
			addItem( p[index] );
		}
	}
}


// --------------------------------------------------------------------------------------------------- //




Listbox::Listbox(const Rect& r, Skin* s) : Widget(r, s), m_scrollbar(0), m_multiSelect(false), m_itemHeight(20) {
}
void Listbox::initialise(const Root* root, const PropertyMap& p) {
	// sub widgets
	m_scrollbar = getTemplateWidget<Scrollbar>("_scroll");
	m_client = getTemplateWidget("_client");
	if(!m_client) m_client = this;
	if(m_scrollbar) m_scrollbar->eventChanged.bind(this, &Listbox::scrollChanged);
	if(m_client!=this) m_client->setTangible(Tangible::CHILDREN);
	
	// Get item template widgets
	m_check = getTemplateWidget("_check");
	m_icon = getTemplateWidget<Icon>("_icon");
	m_text = getTemplateWidget("_text");
	if(m_icon && m_icon->getParent()!=this && m_icon->getParent()!=m_client) m_icon = 0; // Hacky fix for scrollbar icons
	if(m_check) m_check->setVisible(false);
	if(m_icon) m_icon->setVisible(false);
	if(m_text) m_text->setVisible(false);

	// IconList
	if(root && p.contains("iconlist")) setIconList( root->getIconList(p["iconlist"]) );

	// Set item height
	m_itemHeight = 0;
	if(m_check) {
		int h = m_check->getPosition().y + m_check->getSize().y;
		if(h>m_itemHeight) m_itemHeight = h;
	}
	if(m_icon) {
		int h = m_icon->getPosition().y + m_icon->getSize().y;
		if(h>m_itemHeight) m_itemHeight = h;
	}
	if(m_text) {
		int h = m_text->getPosition().y + m_text->getSize().y;
		if(h>m_itemHeight) m_itemHeight = h;
	}
	if(m_itemHeight==0) m_itemHeight = 20;	// Fallback

	// Read items
	ItemList::initialise(p);
	updateBounds();
}

void Listbox::setSize(int w, int h) {
	Widget::setSize(w, h);
	updateBounds();
}

IconList* Listbox::getIconList() const {
	return m_icon? m_icon->getIconList(): 0;
}
void Listbox::setIconList(IconList* l) {
	if(m_icon) m_icon->setIcon(l, 0);
}

void Listbox::ensureVisible(int index) {
	if(m_scrollbar) {
		int p = index * m_itemHeight;
		int offset = m_scrollbar->getValue();
		int max = getAbsoluteClientRect().height - m_itemHeight;
		if(p-offset < 0) m_scrollbar->setValue(p);
		else if(p-offset > max) m_scrollbar->setValue(p-max);
	}
}

void Listbox::onMouseButton(const Point& p, int d, int u) {
	if(d==1) {
		int offset = m_scrollbar? m_scrollbar->getValue(): 0;
		int index = (p.y + offset) / m_itemHeight;
		if(index >=0 && index < (int)getItemCount()) {
			if(isItemSelected(index) && m_multiSelect) {
				deselectItem(index);
				if(eventSelected) eventSelected(this, index);
			}
			else {
				selectItem(index, !m_multiSelect);
				if(eventSelected) eventSelected(this, index);
			}
		}
	}
	Widget::onMouseButton(p, u, d);
}
bool Listbox::onMouseWheel(int w) {
	if(m_scrollbar) m_scrollbar->setValue( m_scrollbar->getValue() - w * m_scrollbar->getStep() );
	if(eventMouseWheel) eventMouseWheel(this, w);
	return m_scrollbar;
}

void Listbox::draw() const {
	if(!isVisible()) return;
	Widget::draw();
	// Items
	int offset = m_scrollbar? m_scrollbar->getValue(): 0;
	int height = m_client->getSize().y;
	int start = offset / m_itemHeight;
	int end   = (offset+height-1) / m_itemHeight;
	if(end >= (int)getItemCount()) end = getItemCount()-1;

	// Set up item rect
	Rect r = m_client->getRect();
	m_root->getRenderer()->push(r);
	r.height = m_itemHeight;
	r.y += start * m_itemHeight - offset;
	int startY = r.y;


	// cache states
	static std::vector<int> cache;
	if((int)cache.size() <= end-start) cache.resize(end-start+1);
	for(int i=start; i<=end; ++i) {
		cache[i-start] = getItemState(i, r);
		r.y += m_itemHeight;
	}

	// Draw checkboxes
	if(m_check) {
		Rect cr(m_check->getPosition(), m_check->getSize());
		cr.x += r.x;
		cr.y += startY;
		for(int i=start; i<=end; ++i) {
			m_root->getRenderer()->drawSkin(m_check->getSkin(), cr, m_check->getColourARGB(), getItemChecked(i)? 4: 0);
			cr.y += m_itemHeight;
		}
	}

	// Draw icons
	if(m_icon) {
		Rect cr(m_icon->getPosition(), m_icon->getSize());
		cr.x += r.x;
		cr.y += startY;
		for(int i=start; i<=end; ++i) {
			m_root->getRenderer()->drawIcon(m_icon->getIconList(), getItemIcon(i), cr);
			cr.y += m_itemHeight;
		}
	}

	// Draw item backgrounds
	if(m_text) {
		Rect cr(m_text->getPosition(), m_text->getSize());
		cr.x += r.x;
		cr.y += startY;
		for(int i=start; i<=end; ++i) {
			m_root->getRenderer()->drawSkin(m_text->getSkin(), cr, m_text->getColourARGB(), cache[i-start]);
			cr.y += m_itemHeight;
		}
		// Draw item text
		cr.y = startY + m_text->getPosition().y;
		for(int i=start; i<=end; ++i) {
			m_root->getRenderer()->drawText(cr.position(), getItem(i), 0, m_text->getSkin(), cache[i-start]);
			cr.y += m_itemHeight;
		}
	}
	m_root->getRenderer()->pop();
}
int Listbox::getItemState(uint item, const Rect& r) const {
	int state = getState() & 3;
	if((state == 1 || state == 2) && !r.contains(m_root->getMousePos())) state = 0;
	if(isItemSelected(item)) state |= 4;
	return state;
}

void Listbox::setMultiSelect(bool on) {
	m_multiSelect = on;
}

void Listbox::scrollChanged(Scrollbar* s, int v) {
}
void Listbox::scrollToItem(uint index) {
	if(!m_scrollbar) return;
	int top = index * m_itemHeight;
	int btm = top + m_itemHeight;
	int height = m_client->getSize().y;
	int offset = m_scrollbar->getValue();
	if(top < offset) m_scrollbar->setValue(top);
	else if(btm - height > offset) m_scrollbar->setValue(btm - height);
}
void Listbox::updateBounds() {
	if(!m_scrollbar) return;
	int h = m_client->getSize().y;
	int t = m_itemHeight * getItemCount();
	m_scrollbar->setRange(0, t>h? t-h: 0, 16);
	m_scrollbar->setBlockSize((float)h / t);
}

void Listbox::itemCountChanged() {
	updateBounds();
}

int Listbox::getItemHeight() const {
	return m_itemHeight;
}


// --------------------------------------------------------------------------------------------------- //



Combobox::Combobox(const Rect& r, Skin* s) : Widget(r, s) {
}
Combobox::~Combobox() {
	// Remove popup from root widget if it is linked from there
	if(m_list && m_list->getParent() != m_client) {
		m_list->getParent()->remove(m_list);
	}
}
void Combobox::initialise(const Root* root, const PropertyMap& p) {
	m_list = getTemplateWidget<Listbox>("_list");
	if(m_list) {
		m_list->setMultiSelect(false);
		m_list->eventSelected.bind(this, &Combobox::changeItem);
		m_list->eventMouseUp.bind(this, &Combobox::popupClicked);
		m_list->eventLostFocus.bind(this, &Combobox::popupLostFocus);
		m_list->setVisible(false);
	}

	// Selected item
	m_text = getTemplateWidget("_text");
	m_icon = getTemplateWidget<Icon>("_icon");
	if(root && p.contains("iconlist")) setIconList( root->getIconList(p["iconlist"]) );

	// Textbox callbacks
	Textbox* txt = m_text? m_text->cast<Textbox>(): 0;
	if(txt) {
		txt->eventChanged.bind(this, &Combobox::textChanged);
		txt->eventSubmit.bind(this, &Combobox::textSubmit);
		txt->setText( p.get("text", "") );
	}

	// Open list button
	Widget* button = getTemplateWidget("_button");
	if(button) button->eventLostFocus.bind(this, &Combobox::popupLostFocus);
	else button = m_client;
	button->eventMouseDown.bind(this, &Combobox::buttonPress);
	button->eventLostFocus.bind(this, &Combobox::popupLostFocus);
	
	// Read items
	ItemList::initialise(p);
}

Widget* Combobox::clone(const char* ws) const {
	Widget* w = Widget::clone(ws);
	Combobox* c = w->cast<Combobox>();
	if(c && getItemCount()) {
		for(uint i=0; i<getItemCount(); ++i) {
			c->addItem(getItem(i), getItemData(i), getItemIcon(i));
		}
	}
	return w;
}

void Combobox::setVisible(bool v) {
	Widget::setVisible(v);
	hideList();
}

void Combobox::setPosition(int x, int y) {
	hideList();
	Widget::setPosition(x, y);
}

void Combobox::itemCountChanged() {
	if(m_list) m_list->updateBounds();
}

void Combobox::changeItem(Listbox*, int index) {
	selectItem(index);
	if(eventSelected) eventSelected(this, index);
}

void Combobox::buttonPress(Widget*, const Point&, int) {
	if(!m_list) return; //error
	if(m_list->isVisible()) hideList();
	else showList();
}

void Combobox::popupClicked(Widget*, const Point&, int) {
	hideList();
}
void Combobox::popupLostFocus(Widget*) {
	onLoseFocus();
}
void Combobox::textChanged(Textbox*, const char* c) {
	hideList();
	clearSelection();
	if(eventTextChanged) eventTextChanged(this, c);
}
void Combobox::textSubmit(Textbox*) {
	if(eventSubmit) eventSubmit(this);
}

void Combobox::onLoseFocus() {
	if(m_list && !m_list->hasFocus()) {
		hideList();
	}
	Widget::onLoseFocus();
}


void Combobox::showList() {
	if(!m_list) return;
	m_list->shareList(this);
	Point pos = m_list->getAbsolutePosition();
	m_root->getRootWidget()->add(m_list);
	m_list->setPosition( pos.x, pos.y );
	int height = getItemCount() * m_list->getItemHeight();
	int limit = getRoot()->getRootWidget()->getSize().y - pos.y;
	if(height == 0) height = 20;
	if(height > 800) height = 800;
	if(height > limit) height = limit;
	m_list->setSize( m_list->getSize().x, height);
	m_list->setVisible(true);
}
void Combobox::hideList() {
	if(!m_list) return;
	m_list->setVisible(false);
	// Move back to child
	if(m_root && m_list->getParent() == m_root->getRootWidget()) {
		Point pos = m_list->getAbsolutePosition() - getAbsolutePosition();
		m_root->getRootWidget()->remove(m_list);
		add(m_list, pos.x, pos.y);
	}
}

void Combobox::setIconList(IconList* l) {
	if(m_icon) m_icon->setIcon(l, 0);
	if(m_list) m_list->setIconList(l);
}
IconList* Combobox::getIconList() const {
	if(m_icon) return m_icon->getIconList();
	else if(m_list) return m_list->getIconList();
	else return 0;
}


void Combobox::setText(const char* text) {
	if(m_client && m_text) {
		if(Label* lbl = m_text->cast<Label>()) lbl->setCaption(text);
		else if(Textbox* txt = m_text->cast<Textbox>()) txt->setText(text);
	}
}

const char* Combobox::getText() const {
	if(!m_client) return m_list->getSelectedItem();
	else if(m_text) {
		if(Label* lbl = m_text->cast<Label>()) return lbl->getCaption();
		else if(Textbox* txt = m_text->cast<Textbox>()) return txt->getText();
	}
	return 0;
}

void Combobox::selectItem(int index) {
	ItemList::selectItem(index);
	if(getSelectionSize()) {
		setText( getSelectedItem() );
		if(m_icon) m_icon->setIcon( getSelectedIcon() );
	}
	else {
		setText("");
		if(m_icon) m_icon->setIcon(-1);
	}
}

