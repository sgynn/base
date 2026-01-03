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
	template<class T> const T& findValue(const T& fallback=0) const { for(const Any& i: m_data) if(i.isType<T>()) return i.getValue(fallback); return fallback; }
	size_t size() const { return m_data.size(); }
	uint getIndex() const { return m_index; }
	operator const char*() const { return getText(); }
	bool isValid() const { return m_index != (uint)-1; }
	private:
	friend class ItemList;
	std::vector<Any> m_data;
	uint m_index = -1;
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
		updateItemIndices(index);
		countChanged();
	}

	template<class T>
	void setItemData(uint index, uint property, const T&& data) {
		if(index < m_items->size()) getItem(index).setValue(property, data);
	}

	void clearSelection();					// Deselect all
	void selectItem(uint index);			// Add item to selected list
	void deselectItem(uint index);			// deselect specific item
	bool isItemSelected(uint index) const;	// is an item selected
	uint getSelectionSize() const;			// Number of selected items


	ListItem* findItem(const char* text);
	const ListItem* findItem(const char* text) const;
	template<class F> ListItem* findItem(const F&& predicate) {
		for(ListItem& i: *m_items) if(predicate(i)) return &i;
		return nullptr;
	}
	template<class F> const ListItem* findItem(const F&& predicate) const {
		for(ListItem& i: *m_items) if(predicate(i)) return &i;
		return nullptr;
	}
	template<class T>
	ListItem* findItem(uint subIndex, const T& value) { 
		for(ListItem& i: *m_items) if(i.getData(subIndex)==value) return &i;
		return nullptr;
	}


	class SelectionIterator {
		std::vector<ListItem>& list;
		std::vector<uint>::const_iterator it;
		public:
		SelectionIterator(std::vector<ListItem>& list, const std::vector<uint>::const_iterator& it) : list(list), it(it) {}
		SelectionIterator& operator++() { ++it; return *this; }
		SelectionIterator operator++(int) { SelectionIterator r(list,it); ++it; return r; }
		ListItem& operator*() { return list[*it]; }
		bool operator==(const SelectionIterator& s) const { return it==s.it; }
		bool operator!=(const SelectionIterator& s) const { return it!=s.it; }
	};
	class SelectionIterable {
		ItemList* list;
		public:
		SelectionIterable(ItemList* l) : list(l) {}
		SelectionIterator begin() { return SelectionIterator(*list->m_items, list->m_selected.cbegin()); }
		SelectionIterator end() { return SelectionIterator(*list->m_items, list->m_selected.cend()); }
		size_t size() const { return list->m_selected.size(); }
		bool empty() const { return list->m_selected.empty(); }
		ListItem& operator[](size_t i) { return list->m_items->at(list->m_selected[i]); }
	};
	template<class List, class Item, class Iterator>
	class IterableType {
		List& list;
		public:
		IterableType(List& list) : list(list) {};
		Iterator begin() { return list.begin(); }
		Iterator end() { return list.end(); }
		Item& operator[](uint i) { return list[i]; }
		size_t size() const { return list.size(); }
		bool empty() const { return list.empty(); }
	};
	using Iterable = IterableType<std::vector<ListItem>, ListItem, std::vector<ListItem>::iterator>;
	using ConstIterable = IterableType<const std::vector<ListItem>, const ListItem, std::vector<ListItem>::const_iterator>;


	// For iteration
	Iterable items() { return Iterable(*m_items); }
	ConstIterable items() const { return ConstIterable(*m_items); }
	SelectionIterable selectedItems() { return SelectionIterable(this); }
	ListItem* getSelectedItem() { return getSelectionSize()? &m_items->at(m_selected[0]): 0; }
	const ListItem* getSelectedItem() const { return getSelectionSize()? &m_items->at(m_selected[0]): 0; }
	int getSelectedIndex() const { return getSelectionSize()? m_selected[0]: -1; }
	const ListItem& getItem(uint index) const;
	ListItem& getItem(uint index);
	void addItem(const ListItem& item);
	void insertItem(uint index, const ListItem& item);

	protected:
	void initialise(const PropertyMap&);
	void updateItemIndices(uint from=0);
	virtual void itemCountChanged() {}
	virtual void itemSelectionChanged(ItemList* src) {}
	
	void addItemData(ListItem& item) {}
	template<class T, class ...R> void addItemData(ListItem& item, const T& value,  R...more) {
		item.setValue(item.size(), value);
		addItemData(item, more...);
	}

	private:
	std::vector<ListItem>* m_items;
	std::vector<ItemList*>* m_shared;
	std::vector<uint> m_selected;
	void dropList();
	void countChanged();
	void selectionChanged();
};


/** Listbox - setup from template. Needs _scroll, _client, itemSkin */
class Listbox : public Widget, public ItemList {
	WIDGET_TYPE(Listbox);
	Listbox();
	~Listbox();
	void copyData(const Widget*) override;
	void setMultiSelect(bool);
	void scrollToItem(uint index);
	void draw() const override;
	void updateBounds();
	void setSize(int w, int h) override;

	IconList* getIconList() const;
	void      setIconList(IconList*);
	int       getItemHeight() const;
	void      ensureVisible(int index);
	void      selectItem(uint index, bool events=false);
	ListItem* getItemAt(const Point& pos);

	Widget*  getItemWidget(); // This may be in use by an item, or just be a template. Can be null.
	void     setItemWidget(Widget*);
	void     refresh() { updateCache(true); }

	public:
	Delegate<void(Listbox*, ListItem&)> eventPressed;
	Delegate<void(Listbox*, ListItem&)> eventSelected;
	Delegate<void(Listbox*, ListItem&, Widget*)> eventCustom;
	Delegate<void(const ListItem&, Widget*)> eventCacheItem;

	private:
	void initialise(const Root*, const PropertyMap&) override;
	void scrollChanged(Scrollbar*, int);
	void itemCountChanged() override;
	void itemSelectionChanged(ItemList* src) override;
	void onMouseButton(const Point&, int, int) override;
	bool onMouseWheel(int) override;
	void bindEvents(Widget* itemWidget);
	void updateCache(bool full);
	void cacheItem(ListItem& item, Widget* w) const;
	template<class T> void fireCustomEventEvent(Widget* w, T data);
	int getItemState(uint, const Rect&, const Point& mouse) const;
	uint getItemIndexFromWidget(const Widget* w) const;
	Scrollbar* m_scrollbar;
	bool m_multiSelect;
	int m_itemHeight;
	int m_tileWidth = 0;
	Widget* m_itemWidget = nullptr;
	std::vector<Widget*> m_cache;
	uint m_cacheOffset = 0;
	uint m_cacheFirst = 0;
	uint m_cacheSize = 0;
};

/** Dropdown list */
class Combobox : public Widget, public ItemList {
	WIDGET_TYPE(Combobox);
	Combobox();
	~Combobox();

	void copyData(const Widget*) override;
	void setPosition(int x, int y) override;
	void setVisible(bool) override;
	void setText(const char* text);
	const char* getText() const;
	void selectItem(int index, bool fireEvents=false);
	void clearSelection();

	public:
	Delegate<void(Combobox*, ListItem&)>     eventSelected;
	Delegate<void(Combobox*, const char*)>   eventTextChanged;
	Delegate<void(Combobox*)>                eventSubmit;
	Delegate<void(const ListItem&, Widget*)> eventCacheItem;
	Delegate<void(Combobox*, Widget*)>       eventDropDown;

	protected:
	void initialise(const Root*, const PropertyMap&) override;
	void itemCountChanged() override;
	void buttonPress(Widget*, const Point&, int);
	void changeItem(Listbox*, ListItem&);
	void popupClicked(Widget*, const Point&, int);
	void popupLostFocus(Widget*);
	void textChanged(Textbox*, const char*);
	void textSubmit(Textbox*);
	void onLoseFocus() override;
	void showList();
	void hideList();

	protected:
	Listbox* m_list = nullptr;
	Widget*  m_text = nullptr;
	
};

/** Table - each element can be a custom widget 
 * Template needs _header, _client, scrollbars, _headerItem. all optional.
 * xml can define columns and data.
 * Columns can be sortable.
 * */
class Table : public Widget {
	WIDGET_TYPE(Table);
	Table();
	~Table();
	void initialise(const Root*, const PropertyMap&) override;
	void draw() const override;
	void setSize(int,int) override;
	void copyData(const Widget*) override;
	Point getPreferredSize(const Point& hint) const override;

	public:
	void showHeader(bool visible);
	void showScrollbars(bool always);
	void setRowHeight(int Height);
	void clearColumns();
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
	void columnPressed(Button*);
	void fireCustomEvent(Widget*);

	protected:
	Widget*     m_headerTemplate = nullptr;
	Widget*     m_header = nullptr;
	Widget*     m_dataPanel = nullptr;
	bool        m_sortable = false;	// Are columns sortable
	int         m_sorted = 0;   // column to sort by. negative for inverse order. 0=none;
	int         m_rowHeight = 16;

	typedef std::vector<Any> DefaultRowType;
	std::vector<Any> m_data;	// Table data

	struct Column { Widget* header; String name; String widgets; int pos; int width; };
	std::vector<Column> m_columns;

	// Cell widget cache
	uint m_cacheOffset = 0;
	uint m_cacheSize = 0;
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

