#ifndef _GUI_TREE_
#define _GUI_TREE_

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
	TreeNode* operator[](int);					// get child node
	TreeNode* at(int);							// Get child node
	TreeNode* back();							// Get last child node
	TreeNode* add(TreeNode*);					// Add child node
	TreeNode* add(const char*, const Any& d=Any());	// Add child node
	TreeNode* insert(int index, TreeNode*);		// Insert child node at index
	TreeNode* insert(int index, const char*);	// Insert child node at index
	TreeNode* remove(int index);				// Remove child node
	TreeNode* remove(TreeNode*);				// Remove child node
	TreeNode* remove();							// Remove this node
	TreeNode* getParent() const;				// Get parent node
	int       getIndex() const;					// Get node index
	void      clear();							// Delete all child nodes

	public:
	void refresh();				// Update cached widgets from node data
	void select();				// Select this node
	void expand(bool);			// Set whether node is expanded
	void expandAll();			// Recursively expand all nodes
	void ensureVisible();		// Expand all ancestor nodes
	bool isExpanded() const;	// Is this node expanded
	bool isSelected() const;	// Is this node selected
	const Any&  getData(size_t sub=0) const;	// Get node data
	void setText(const char*);
	const char* getText(size_t index=0) const;
	void setData(const Any& data);			// Set node data
	void setData(size_t, const Any& data);	// Set node data
	std::vector<TreeNode*>::const_iterator begin()const;			// Iterator begin
	std::vector<TreeNode*>::const_iterator end() const;				// Iterator end

	// implicit Any interface
	template<class T> void setData(const T& v) { setData(Any(v)); }
	template<class T> void setData(size_t i, const T& v) { setData(i, Any(v)); }
	template<class T> TreeNode* add(const char* n, const T& d) { return add(n, Any(d)); }
};

/** TreeView - Heirachical listbox */
class TreeView : public Widget {
	friend class TreeNode;
	WIDGET_TYPE(TreeView);
	TreeView(const Rect& r, Skin* s);
	~TreeView();

	void scrollToItem(TreeNode*);
	void draw() const override;
	void setSize(int,int) override;
	Widget* clone(const char* t) const override;

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
	void buttonEvent(class Button*);
	void textEvent(class Textbox*);
	void bindEvents(Widget* item);
	TreeNode* findNode(Widget* w);
};

}


#endif

