#include <base/gui/lists.h>
#include <base/gui/widgets.h>
#include <base/gui/renderer.h>
#include <base/gui/skin.h>
#include <base/gui/font.h>
#include <algorithm>
#include <cstdio>

using namespace gui;

// --------------------------------------------------------------------------------------------------- //

ItemList::ItemList() {
	m_items = new std::vector<ListItem>;
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
		m_items = new std::vector<ListItem>;
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
	m_items->clear();
	m_selected->clear();
	countChanged();
}

void ItemList::addItem(const ListItem& item) {
	m_items->push_back(item);
	countChanged();
}
void ItemList::insertItem(uint index, const ListItem& item) {
	if(index>=m_items->size()) m_items->push_back(item);
	else m_items->insert(m_items->begin() + index, item);
	countChanged();
}

void ItemList::removeItem(uint index) {
	if(index < m_items->size()) {
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

uint ItemList::getItemCount() const {
	return m_items->size();
}
const ListItem& ItemList::getItem(uint index) const {
	return m_items->at(index);
}
ListItem& ItemList::getItem(uint index) {
	return m_items->at(index);
}

int ItemList::findItem(const char* name) const {
	for(uint i=0; i<m_items->size(); ++i) {
		const char* text = m_items->at(i).getText();
		if(text && strcmp(text, name)==0) return i;
	}
	return -1;
}

void ItemList::sortItems(int flags) {
	if(flags & IGNORE_CASE) {
		bool inv = flags & INVERSE;
		std::sort(m_items->begin(), m_items->end(), [inv](const ListItem& a, const ListItem& b) {
			const char* sa = a.getText(); if(!sa) sa="";
			const char* sb = b.getText(); if(!sb) sb="";
			constexpr int shift = 'a' - 'A';
			while(*sa && *sb) {
				char u = *sa>'a'? *sa - shift: *sa;
				char v = *sa>'a'? *sa - shift: *sa;
				if((u<v) != inv) return false;
				++sa;
				++sb;
			}
			return (*sa < *sb) != inv;
		});
	}
	else {
		int inv = flags & INVERSE? -1: 1;
		std::sort(m_items->begin(), m_items->end(), [inv](const ListItem& a, const ListItem& b) {
			const char* sa = a.getText(); if(!sa) sa="";
			const char* sb = b.getText(); if(!sb) sb="";
			return strcmp(sa, sb) * inv < 0;
		});
	}
}


void ItemList::clearSelection() {
	m_selected->clear();
	selectionChanged();
}
void ItemList::selectItem(uint index, bool only) {
	if(only) clearSelection();
	if(index < getItemCount() && !isItemSelected(index)) m_selected->push_back(index);
	selectionChanged();
}
void ItemList::deselectItem(uint index) {
	for(size_t i=0; i<m_selected->size(); ++i) {
		if(m_selected->at(i) == index) {
			m_selected->erase(m_selected->begin()+i);
			selectionChanged();
			break;
		}
	}
}

bool ItemList::isItemSelected(uint index) const {
	if(index >= m_items->size() || m_selected->empty()) return false;
	for(uint i: *m_selected) if(i==index) return true;
	return false;
}

uint ItemList::getSelectionSize() const {
	return m_selected->size();
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

// ------------------------------- //

const char* ListItem::getText(uint index) const {
	String* str = getData(index).cast<String>();
	return str? str->str(): 0;
}
const Any& ListItem::getData(uint index) const {
	const Any null;
	return index<m_data.size()? m_data[index]: null;
}
void ListItem::setValue(uint index, const Any& value) {
	while(index >= m_data.size()) m_data.emplace_back();
	m_data[index] = value;
}
void ListItem::setValue(uint index, const char* value) { 
	setValue(index, Any(String(value)));
}


// --------------------------------------------------------------------------------------------------- //




Listbox::Listbox(const Rect& r, Skin* s) : Widget(r, s), m_scrollbar(0), m_multiSelect(false), m_itemHeight(20) {
}
Listbox::~Listbox() {
	if(m_itemWidget && !m_itemWidget->getParent(true)) delete m_itemWidget;
}
void Listbox::initialise(const Root* root, const PropertyMap& p) {
	// sub widgets
	m_scrollbar = getTemplateWidget<Scrollbar>("_scroll");
	m_client = getTemplateWidget("_client");
	if(!m_client) m_client = this;
	if(m_scrollbar) m_scrollbar->eventChanged.bind(this, &Listbox::scrollChanged);
	if(m_client!=this) m_client->setTangible(Tangible::CHILDREN);
	
	// Get item widget
	Widget* item = getWidget("_item");
	if(!item) item = getTemplateWidget("_item");
	if(item) {
		item->setVisible(false);
		m_itemHeight = item->getSize().y;
		// Wrong place?
		if(item->getParent(true) != m_client) {
			item = item->clone();
			add(item);
		}
		bindEvents(item);
	}
	else m_itemHeight = getSkin()->getFont()->getLineHeight(getSkin()->getFontSize());
	if(item && !m_itemWidget) m_itemWidget = item;

	// Read items
	ItemList::initialise(p);
	updateBounds();
}

void Listbox::setItemWidget(Widget* w) {
	if(!m_itemWidget->getParent()) delete m_itemWidget;
	for(Widget* w: m_cache) delete w;
	m_itemWidget = w;
	updateCache(true);
}
Widget* Listbox::getItemWidget() {
	return m_itemWidget;
}

void Listbox::setSize(int w, int h) {
	Widget::setSize(w, h);
	updateBounds();
	updateCache(false);
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

inline int listItemSubWidgetIndex(Widget* w) {
	if(w->getName()[0]=='_' && w->getName()[1]>='0' && w->getName()[2]==0) return w->getName()[1]-'0';
	else return -1;
}

template<class T>
void Listbox::fireCustomEventEvent(Widget* w, T data) {
	while(w->getParent(true) != m_client) w = w->getParent();
	uint index = w->getIndex() + m_cacheOffset;
	if(index < getItemCount()) {
		ListItem& item = getItem(index);
		if(!eventCacheItem && listItemSubWidgetIndex(w) >= 0) {
			item.setValue(listItemSubWidgetIndex(w), data);
		}
		if(eventCustom) eventCustom(this, index, w);
	}
}

void Listbox::bindEvents(Widget* item) {
	if(Button* b = item->cast<Button>()) b->eventPressed.bind([this](Button* b) { fireCustomEventEvent(b, true); });
	if(Checkbox* c = item->cast<Checkbox>()) c->eventChanged.bind([this](Button* c) { fireCustomEventEvent(c, c->isSelected()); });
	if(Textbox* t = item->cast<Textbox>()) t->eventSubmit.bind([this](Textbox* t) { fireCustomEventEvent(t, t->getText()); });
	for(Widget* w: *item) bindEvents(w);
}

void Listbox::cacheItem(ListItem& item, Widget* w) const {
	if(eventCacheItem) eventCacheItem(item, w);
	else {
		// Automatic version.
		auto setFromData = [&item](Widget* w, const Any& value) {
			if(value.isNull()) return;
			if(Checkbox* c = w->cast<Checkbox>()) c->setSelected(value.getValue(false));
			else if(const String* text = value.cast<String>()) {
				if(Label* l = w->cast<Label>()) l->setCaption(*text);
				else if(Textbox* t = w->cast<Textbox>()) t->setText(*text);
				else if(Icon* i = w->cast<Icon>()) i->setIcon(*text);
			}
			else if(Icon* i = w->cast<Icon>()) i->setIcon(value.getValue<int>(-1));
		};

		if(w->getWidgetCount() == 0 && w->getTemplateCount() == 0) {
			setFromData(w, item.getData());
		}
		else {
			for(Widget* c: *w) {
				int subItem = listItemSubWidgetIndex(c);
				if(subItem>=0) setFromData(c, item.getData(subItem));
			}
			for(int i=0; i<w->getTemplateCount(); ++i) {
				int subItem = listItemSubWidgetIndex(w->getTemplateWidget(i));
				if(subItem>=0) setFromData(w->getTemplateWidget(i), item.getData(subItem));
			}
		}
	}
}

void Listbox::updateCache(bool full) {
	if(!m_itemWidget) return;
	int offset = m_scrollbar? m_scrollbar->getValue(): 0;
	uint first = offset / m_itemHeight;
	uint last = std::min((offset + m_rect.height) / m_itemHeight+1, (int)getItemCount());
	
	uint requiredCount = last - first;
	if(requiredCount != m_cacheSize || first != m_cacheFirst || full) {
		// Resize item cache - Cache is a circular buffer, so need to add them in the right place
		if(m_cache.size() < requiredCount) {
			int count = requiredCount - m_cache.size();
			m_cache.insert(m_cache.begin() + m_cacheOffset, count, nullptr);
			for(int i=0; i<count; ++i) {
				m_cache[m_cacheOffset+i] = m_itemWidget->clone();
				add(m_cache[m_cacheOffset+i]);
			}
			m_cacheOffset += count;
		}

		if(full) m_cacheSize = 0;
		if(requiredCount) {
			// Before
			int cacheIndex = m_cacheOffset - (m_cacheFirst - first);
			if(cacheIndex < 0) cacheIndex += m_cache.size();
			for(uint i=first; i<m_cacheFirst; ++i, ++cacheIndex) {
				Widget* itemWidget = m_cache[cacheIndex % m_cache.size()];
				if(!itemWidget->getParent()) add(itemWidget);
				itemWidget->setPosition(0, i * m_itemHeight);
				cacheItem(getItem(i), itemWidget);
			}

			// After
			cacheIndex = (m_cacheOffset + m_cacheSize) % m_cache.size();
			for(uint i=m_cacheFirst + m_cacheSize; i<last; ++i, ++cacheIndex) {
				Widget* itemWidget = m_cache[cacheIndex % m_cache.size()];
				if(!itemWidget->getParent()) add(itemWidget);
				itemWidget->setPosition(0, i * m_itemHeight);
				cacheItem(getItem(i), itemWidget);
			}

			m_cacheOffset = (m_cacheOffset + (first - m_cacheFirst) + m_cache.size()) % m_cache.size();
			m_cacheFirst = first;
			m_cacheSize = requiredCount;
		}
		else m_cacheOffset = m_cacheSize = m_cacheFirst = 0;
		
		// Remove surplus
		for(size_t i=m_cacheSize; i<m_cache.size(); ++i) {
			m_cache[(i + m_cacheOffset) % m_cache.size()]->removeFromParent();
		}
	}

	// Set positions and states
	for(uint i=first, c=m_cacheOffset; i<last; ++i, ++c) {
		Widget* itemWidget = m_cache[c%m_cache.size()];
		itemWidget->setPosition(0, i * m_itemHeight - offset);
		itemWidget->setVisible(true);
		itemWidget->setSelected(isItemSelected(i));
	}
}

void Listbox::itemSelectionChanged() {
	updateCache(false);
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
	drawSkin();

	// Item Text, if no item widgets used
	if(!m_itemWidget && getItemCount()) {
		int offset = m_scrollbar? m_scrollbar->getValue(): 0;
		Renderer* renderer = getRoot()->getRenderer();
		renderer->push(m_rect);
		Transform parentTransform = renderer->getTransform();
		renderer->setTransform(m_client->getDerivedTransform());
		int first = offset / m_itemHeight;

		// Item text
		uint selColour = (getSkin()->getState(4).foreColour & 0xffffff) | 0x30000000;
		Point mouse = m_derivedTransform.untransform(getRoot()->getMousePos());
		Rect box(0, first * m_itemHeight - offset, m_rect.width, m_itemHeight);
		for(uint index=first; index<getItemCount() && box.y<m_rect.height; box.y+=m_itemHeight, ++index) {
			const char* text = getItem(index).getText();
			int state = getItemState(index, box, mouse);
			if(state&4) renderer->drawRect(box, selColour);
			if(text) renderer->drawText(box.position(), text, 0, getSkin(), state);
		}

		renderer->pop();
		renderer->setTransform(parentTransform);
	}

	drawChildren();
}
int Listbox::getItemState(uint item, const Rect& r, const Point& mouse) const {
	int state = getState() & 3;
	if((state == 1 || state == 2) && !r.contains(mouse)) state = 0;
	if(isItemSelected(item)) state |= 4;
	return state;
}

void Listbox::setMultiSelect(bool on) {
	m_multiSelect = on;
}

void Listbox::scrollChanged(Scrollbar* s, int v) {
	updateCache(false);
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
	updateCache(true);
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
		for(const ListItem& i: items()) c->addItem(i);
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

void Combobox::setText(const char* text) {
	if(m_client && m_text) {
		if(Label* lbl = m_text->cast<Label>()) lbl->setCaption(text);
		else if(Textbox* txt = m_text->cast<Textbox>()) txt->setText(text);
	}
}

const char* Combobox::getText() const {
	if(m_text) {
		if(Label* lbl = m_text->cast<Label>()) return lbl->getCaption();
		else if(Textbox* txt = m_text->cast<Textbox>()) return txt->getText();
	}
	if(const ListItem* selected = getSelectedItem()) {
		return selected->getText();
	}
	return 0;
}

void Combobox::selectItem(int index) {
	ItemList::selectItem(index);
	if(const ListItem* selected = getSelectedItem()) setText(selected->getText());
	else setText("");
}

