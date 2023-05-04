#pragma once

#include "widgets.h"
#include "any.h"

namespace gui {

// Item in a list box
class ListItem {
	public:
	const char* getText(uint index=0) const;
	const Any& getData(uint index=0) const;
	void setValue(uint index, const Any& value);
	void setValue(uint index, const char* value);
	void setValue(uint index, char* value) { setValue(index, (const char*)value); }
	template<class T> void setValue(uint index, const T& value) { setValue(index, Any(value)); }
	template<class T> void setValue(const T& value) { setValue(0, value); }
	template<class T> const T& getValue(int index, const T& fallback) const { return getData(index).getValue(fallback); }
	size_t size() const { return m_data.size(); }
	operator const char*() const { return getText(); }
	private:
	std::vector<Any> m_data;
};


/// Shared list management interface
class ItemList {
	public:
	ItemList();
	virtual ~ItemList();
	void shareList(ItemList*);									// Use the list data from another list

	void clearItems();											// Clear all items
	void removeItem(uint index);								// Remove an item
	uint getItemCount() const;									// Number of items

	enum SortFlags { INVERSE=1, IGNORE_CASE=2 };
	void sortItems(int sortFlags = 0);

	template<class ...T>
	void addItem(T...t) { insertItem(m_items->size(), t...); }
	template<class ...T>
	void insertItem(uint index, T...t) {
		m_items->insert(m_items->begin()+index, ListItem());
		addItemData(m_items->at(index), t...);
		countChanged();
	}

	template<class T>
	void setItemData(uint index, uint property, const T&& data) {
		if(index < m_items->size()) getItem(index).setValue(property, data);
	}

	void clearSelection();										// Deselect all
	void selectItem(uint index, bool only=true);				// select an item. if !only, add to selection
	void deselectItem(uint index);								// deselect specific item
	bool isItemSelected(uint index) const;						// is an item selected
	uint getSelectionSize() const;								// Number of selected items
	int findItem(const char* name) const;						// Find the index of an item by name
	

	class SelectionIterator {
		std::vector<ListItem>& list;
		std::vector<uint>::const_iterator it;
		public:
		SelectionIterator(std::vector<ListItem>& list, const std::vector<uint>::const_iterator& it) : list(list), it(it) {}
		SelectionIterator& operator++() { ++it; return *this; }
		SelectionIterator operator++(int) { SelectionIterator r(list,it); ++it; return r; }
		ListItem& operator*() { return list[*it]; }
		bool operator==(const SelectionIterator& s) { return it==s.it; }
	};
	class SelectionIterable {
		ItemList* list;
		public:
		SelectionIterable(ItemList* l) : list(l) {}
		SelectionIterator begin() { return SelectionIterator(*list->m_items, list->m_selected->cbegin()); }
		SelectionIterator end() { return SelectionIterator(*list->m_items, list->m_selected->cend()); }
		size_t size() const { return list->m_selected->size(); }
		ListItem& operator[](size_t i) { return list->m_items->at(list->m_selected->at(i)); }
	};


	// For iteration
	const std::vector<ListItem>& items() const { return *m_items; };
	SelectionIterable selectedItems() { return SelectionIterable(this); }
	const ListItem* getSelectedItem() const { return getSelectionSize()? &m_items->at(m_selected->at(0)): 0; }
	int getSelectedIndex() const { return getSelectionSize()? m_selected->at(0): -1; }
	const ListItem& getItem(uint index) const;
	ListItem& getItem(uint index);
	void addItem(const ListItem& item);
	void insertItem(uint index, const ListItem& item);

	protected:
	void initialise(const PropertyMap&);
	virtual void itemCountChanged() {}
	virtual void itemSelectionChanged() {}
	
	void addItemData(ListItem& item) {}
	template<class T, class ...R> void addItemData(ListItem& item, const T& value,  R...more) {
		item.setValue(item.size(), value);
		addItemData(item, more...);
	}

	private:
	std::vector<ListItem>* m_items;
	std::vector<uint>* m_selected;
	std::vector<ItemList*>* m_shared;
	void dropList();
	void countChanged();
	void selectionChanged();
};


/** Listbox - setup from template. Needs _scroll, _client, itemSkin */
class Listbox : public Widget, public ItemList {
	WIDGET_TYPE(Listbox);
	Listbox(const Rect& r, Skin*);
	~Listbox();
	void setMultiSelect(bool);
	void scrollToItem(uint index);
	void draw() const override;
	void updateBounds();
	void setSize(int w, int h) override;

	IconList* getIconList() const;
	void      setIconList(IconList*);
	int       getItemHeight() const;
	void      ensureVisible(int index);

	Widget*  getItemWidget(); // This may be in use by an item, or just be a template. Can be null.
	void     setItemWidget(Widget*);

	public:
	Delegate<void(Listbox*, int)> eventPressed;
	Delegate<void(Listbox*, int)> eventSelected;
	Delegate<void(Listbox*, int, Widget*)> eventCustom;
	Delegate<void(const ListItem&, Widget*)> eventCacheItem;

	private:
	void initialise(const Root*, const PropertyMap&) override;
	void scrollChanged(Scrollbar*, int);
	void itemCountChanged() override;
	void itemSelectionChanged() override;
	void onMouseButton(const Point&, int, int) override;
	bool onMouseWheel(int) override;
	void bindEvents(Widget* itemWidget);
	void updateCache(bool full);
	void cacheItem(ListItem& item, Widget* w) const;
	template<class T> void fireCustomEventEvent(Widget* w, T data);
	int getItemState(uint, const Rect&, const Point& mouse) const;
	Scrollbar* m_scrollbar;
	bool m_multiSelect;
	int m_itemHeight;
	Widget* m_itemWidget = nullptr;
	std::vector<Widget*> m_cache;
	uint m_cacheOffset = 0;
	uint m_cacheFirst = 0;
	uint m_cacheSize = 0;
};

/** Dropdown list */
class Combobox : public Widget, public ItemList {
	WIDGET_TYPE(Combobox);
	Combobox(const Rect&, Skin*);
	~Combobox();

	Widget* clone(const char*) const override;
	void setPosition(int x, int y) override;
	void setVisible(bool) override;
	void setText(const char* text);
	const char* getText() const;
	void selectItem(int index);

	public:
	Delegate<void(Combobox*, int)>           eventSelected;
	Delegate<void(Combobox*, const char*)>   eventTextChanged;
	Delegate<void(Combobox*)>                eventSubmit;
	Delegate<void(const ListItem&, Widget*)> eventCacheItem;

	protected:
	void initialise(const Root*, const PropertyMap&) override;
	void itemCountChanged() override;
	void buttonPress(Widget*, const Point&, int);
	void changeItem(Listbox*, int);
	void popupClicked(Widget*, const Point&, int);
	void popupLostFocus(Widget*);
	void textChanged(Textbox*, const char*);
	void textSubmit(Textbox*);
	void onLoseFocus() override;
	void showList();
	void hideList();

	protected:
	Listbox* m_list;
	Widget*  m_text;
	
};

/** Table - each element can be a custom widget 
 * Template needs _header, _client, scrollbars, _headerItem. all optional.
 * xml can define columns and data.
 * Columns can be sortable.
 * */
class Table : public Widget {
	WIDGET_TYPE(Table);
	Table(const Rect&, Skin*);
	~Table();
	void initialise(const Root*, const PropertyMap&) override;
	void draw() const override;
	void setSize(int,int) override;
	Widget* clone(const char* t) const override;

	public:
	void showHeader(bool visible);
	void setRowHeight(int Height);
	uint addColumn(const char* name, const char* title, int width=0, const char* itemTemplate=0);
	uint insertColumn(uint index, const char* name, const char* title, int width=0, const char* itemTemplate=0);
	void removeColumn(uint index);
	uint getColumnIndex(const char* name) const;
	const char* getColumnName(uint index) const;
	uint        getColumnCount() const;
	void setColumnWidth(uint index, int width);
	void setColumnTitle(uint index, const char* title);
	void setColumnIndex(uint index, uint newIndex);
	int  getColumnWidth(uint index) const;

	uint addRow();
	uint insertRow(uint index);
	uint addCustomRow(const Any&);
	void setCustomRow(uint row, const Any& value);
	uint insertCustomRow(uint index, const Any&);
	void removeRow(uint index);
	const Any& getRowData(uint row) const;
	void clearRows();
	uint getRowCount() const;
	Point getCellAt(const Point&, bool global=false) const;

	// These only work if using default row types
	bool        setValue(uint row, uint column, const Any& value);
	const Any&  getValue(uint row, uint column) const;
	void        sort(uint column, bool reverse=false);	// Note: Any class needs to be sortable.

	// use these when using custom row data
	void refreshCustomRow(uint row) { cacheRow(row); }
	void refreshAll() { cacheAll(); }

	// Templatey functions for ANy conversion
	template<class T> uint addCustomRow(const T& data) { return addCustomRow(Any(data)); }
	template<class T> void setCustomRow(uint row, const T& data) { setCustomRow(row, Any(data)); }
	template<class T> uint insertCustomRow(uint index, const T& data) { return insertCustomRow(index, Any(data)); }
	template<class T> bool setValue(uint row, uint column, const T& value) { return setValue(row,column,Any(value)); }

	public:
	Delegate<bool(Table*, int row, int columm, Widget* out)> eventCacheItem;	// Used to update display from data
	Delegate<void(Table*, int row, int column, Widget* sender)> eventCustom;	// Cell events from buttons etc.
	Delegate<void(Table*, int column)> eventColummPressed;						// Clicked on column header

	protected:
	void scrollChanged(Scrollbar*, int);
	void onMouseButton(const Point&, int, int) override;
	void onAdded() override;

	void textEvent(Textbox* t)	     { fireCustomEvent(t); }
	void buttonEvent(Button* b)      { fireCustomEvent(b); }
	void listEvent(Combobox* c, int) { fireCustomEvent(c); }
	void columnPressed(Button*);
	void fireCustomEvent(Widget*);

	protected:
	Widget*     m_headerTemplate;
	Widget*     m_header;
	Scrollpane* m_dataPanel;
	bool        m_sortable;	// Are columns sortable
	int         m_sorted;   // column to sort by. negative for inverse order. 0=none;
	int         m_rowHeight;

	typedef std::vector<Any> DefaultRowType;
	std::vector<Any> m_data;	// Table data

	struct Column { Widget* header; String name; String widgets; int pos; int width; };
	std::vector<Column> m_columns;

	// Cell widget cache
	uint m_cacheOffset;
	uint m_cacheSize;
	typedef std::vector<Widget*> RowCache;
	std::vector<RowCache> m_cellCache;
	void clearCache();
	void cacheRow(uint index);
	void cacheItem(uint row, uint col);
	void cacheAll();
	void cachePositions(uint fromRow=0, uint fromCol=0);
	void updateScrollSize();
};


}

