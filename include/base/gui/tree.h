#pragma once

#include "gui.h"
#include "any.h"

namespace gui {

class TreeView;
class Scrollbar;

/** Data for treeview */
class TreeNode {
	private:
	friend class TreeView;
	TreeView* m_treeView;
	TreeNode* m_parent;
	bool      m_selected;
	bool      m_expanded;
	int       m_depth;			// tree depth
	int       m_displayed;		// derived height of this plus visible child nodes
	Widget*   m_cached;			// Cached widget
	std::vector<Any> m_data;
	std::vector<TreeNode*> m_children;
	void setParentNode(TreeNode* parent);
	void changeDisplayed(int newValue);

	public:
	TreeNode(const char* text=0);
	~TreeNode();

	public:
	size_t size() const;						// Get number of children
	TreeNode* operator[](uint);					// get child node
	TreeNode* at(uint);							// Get child node
	TreeNode* back();							// Get last child node
	TreeNode* add(TreeNode*);					// Add child node
	TreeNode* insert(uint index, TreeNode*);		// Insert child node at index
	TreeNode* remove(uint index);				// Remove child node
	TreeNode* remove(TreeNode*);				// Remove child node
	TreeNode* remove();							// Remove this node
	TreeNode* getParent() const;				// Get parent node
	int       getIndex() const;					// Get node index
	void      clear();							// Delete all child nodes


	// Fun templatey add() and insert() functions where you can specify arbitrary data
	template<class ...T>
	TreeNode* add(T...values) { return insert(size(), values...); }
	template<class ...T>
	TreeNode* insert(uint index, T...values) {
		TreeNode* node = new TreeNode();
		addNodeData(node, values...);
		return insert(index, node);
	}
	private:
	void addNodeData(TreeNode*) {}
	template<class T, class ...R> void addNodeData(TreeNode* node, const T& value,  R...more) {
		node->setValue(node->m_data.size(), value);
		addNodeData(node, more...);
	}

	public:
	void refresh();				// Update cached widgets from node data
	void select();				// Select this node
	void expand(bool);			// Set whether node is expanded
	void expandAll();			// Recursively expand all nodes
	void ensureVisible();		// Expand all ancestor nodes
	bool isExpanded() const;	// Is this node expanded
	bool isSelected() const;	// Is this node selected
	std::vector<TreeNode*>::const_iterator begin() const;			// Iterator begin
	std::vector<TreeNode*>::const_iterator end() const;				// Iterator end
	std::vector<TreeNode*>::iterator begin();
	std::vector<TreeNode*>::iterator end();

	// Data access - Same as Listbox
	const char* getText(uint index=0) const;
	const Any& getData(uint index=0) const;
	void setValue(uint index, const Any& value);
	void setValue(uint index, const char* value);
	void setValue(uint index, char* value) { setValue(index, (const char*)value); }
	template<class T> void setValue(uint index, const T& value) { setValue(index, Any(value)); }
	template<class T> void setValue(const T& value) { setValue(0, value); }
	template<class T> const T& getValue(int index, const T& fallback) const { return getData(index).getValue(fallback); }
	template<class T> const T& findValue(const T& fallback=0) const { for(const Any& i: m_data) if(i.isType<T>()) return i.getValue(fallback); return fallback; }
};

/** TreeView - Heirachical listbox */
class TreeView : public Widget {
	friend class TreeNode;
	WIDGET_TYPE(TreeView);
	TreeView();
	~TreeView();

	void scrollToItem(TreeNode*);
	void draw() const override;
	void setSize(int,int) override;
	void copyData(const Widget*) override;

	public:
	Delegate<void(TreeView*, TreeNode*)> eventSelected;
	Delegate<void(TreeView*, TreeNode*)> eventExpanded;
	Delegate<void(TreeView*, TreeNode*)> eventCollapsed;
	Delegate<void(TreeView*, TreeNode*, int)> eventPressed;
	Delegate<void(TreeView*, TreeNode*, Widget*)> eventCustom;
	Delegate<void(TreeNode*, Widget*)> eventCacheItem;

	public:
	TreeNode* getSelectedNode();
	TreeNode* getRootNode();
	TreeNode* getNodeAt(const Point& localPos);
	void      setRootNode(TreeNode* n);
	void      clearSelection();
	void      expandAll();
	void      showRootNode(bool s);
	void      refresh();

	protected:
	void initialise(const Root*, const PropertyMap&) override;
	void scrollChanged(Scrollbar*, int);
	void onMouseButton(const Point&, int, int) override;
	void onMouseMove(const Point&, const Point&, int) override;
	bool onMouseWheel(int w) override;
	int  getItemState(uint, const Rect&) const;
	void cacheItem(TreeNode*, Widget*) const;
	void removeCache(TreeNode*);
	Scrollbar* m_scrollbar;
	TreeNode*  m_over;
	int        m_itemHeight;
	int        m_indent;
	bool       m_hideRootNode;
	int        m_lineColour;
	char       m_zip = 0;

	Widget*    m_expand;
	TreeNode*  m_rootNode;
	TreeNode*  m_selectedNode;

	// render cache
	struct CacheItem { TreeNode* node; int up; };
	struct ItemWidget { Widget* widget; TreeNode* node; };
	mutable std::vector<ItemWidget> m_itemWidgets;
	mutable std::vector<CacheItem>  m_drawCache;
	mutable std::vector<Point>      m_additionalLines;
	mutable int                     m_cacheOffset;
	mutable bool                    m_needsUpdating;
	void                            updateCache() const;
	int                             buildCache(TreeNode* n, int y, int top, int bottom) const;

	protected:
	void bindEvents(Widget* item);
	TreeNode* findNode(Widget* w);
	template<class T> void fireCustomEventEvent(Widget* w, T data);
};

}


