#ifndef _GUI_LISTS_
#define _GUI_LISTS_

#include "widgets.h"
#include "any.h"

namespace gui {

/** Basic list management for list based widgets */
class ItemList {
	public:
	ItemList();
	virtual ~ItemList();
	void shareList(ItemList*);									// Use the list data from another list

	void clearItems();														// Clear all items
	void addItem(const char* name, const Any& data=Any(), int icon=0);					// Add a item to the list
	void insertItem(uint index, const char* name, const Any& data=Any(), int icon=0);	// Insert an item at an index
	void setItemName(uint index, const char* name);							// Change existing item text
	void setItemData(uint index, const Any& data);							// Change existing item data
	void setItemChecked(uint index, bool check);							// Set check state
	void setItemIcon(uint index, int icon);									// set item icon index
	void removeItem(uint index);											// Remove an item
	uint getItemCount() const;												// Number of items

	enum SortFlags { INVERSE=1, IGNORE_CASE=2 };
	void sortItems(int sortFlags = 0);

	template<class T> void setItemData(uint index, const T& v) { setItemData(index, Any(v)); }
	template<class T> void addItem(const char* name, const T& data, int icon=0) { addItem(name, Any(data), icon); }
	template<class T> void insertItem(uint index, const char* name, const T& data, int icon=0) { insertItem(index, name, Any(data),icon); }

	const char* getItem(uint index) const;							// Get item text
	const Any&  getItemData(uint index) const;						// get item data
	bool        getItemChecked(uint index) const;					// is item checked
	int         getItemIcon(uint index) const;						// get item icon

	int         findItem(const char* name) const;					// Find the index of an item by name

	void clearSelection();										// Deselect all
	void selectItem(uint index, bool only=true);				// select an item. if !only, add to selection
	void deselectItem(uint index);								// deselect specific item
	bool isItemSelected(uint index) const;						// is an item selected
	uint getSelectionSize() const;								// Number of selected items
	const char* getSelectedItem(uint sIndex=0) const;			// get selected item text. Use index if multiple
	const Any&  getSelectedData(uint sIndex=0) const;			// Get selected item data
	int         getSelectedIcon(uint sIndex=0) const;			// Get selected item icon
	int         getSelectedIndex(uint sIndex=0) const;			// Get selected item index

	protected:
	void initialise(const PropertyMap&);
	virtual void itemCountChanged() {}
	virtual void itemSelectionChanged() {}
	struct Item {
		String name;
		Any data;
		int icon;
		bool selected;
		bool checked;
	};

	private:
	std::vector<Item>* m_items;
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
	void setMultiSelect(bool);
	void scrollToItem(uint index);
	void draw() const override;
	void updateBounds();
	void setSize(int w, int h) override;

	IconList* getIconList() const;
	void      setIconList(IconList*);
	int       getItemHeight() const;
	void      ensureVisible(int index);

	public:
	Delegate<void(Listbox*, int)> eventSelected;

	private:
	void initialise(const Root*, const PropertyMap&) override;
	void scrollChanged(Scrollbar*, int);
	void itemCountChanged() override;
	void onMouseButton(const Point&, int, int) override;
	bool onMouseWheel(int) override;
	int getItemState(uint, const Rect&, const Point& mouse) const;
	Scrollbar* m_scrollbar;
	bool m_multiSelect;
	int m_itemHeight;
	Widget* m_text;
	Widget* m_check;
	Icon*   m_icon;
};

/** Dropdown list */
class Combobox : public Widget, public ItemList {
	WIDGET_TYPE(Combobox);
	Combobox(const Rect&, Skin*);
	~Combobox();

	Widget* clone(const char*) const override;
	void setPosition(int x, int y) override;
	void setText(const char* text);
	const char* getText() const;
	void selectItem(int index);

	IconList* getIconList() const;
	void      setIconList(IconList*);

	void setVisible(bool) override;
	public:
	Delegate<void(Combobox*, int)>         eventSelected;
	Delegate<void(Combobox*, const char*)> eventTextChanged;
	Delegate<void(Combobox*)>              eventSubmit;

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
	Icon*    m_icon;
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


#endif

