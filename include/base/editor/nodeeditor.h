#pragma once

#include <base/gui/gui.h>
#include <base/gui/widgets.h>

namespace nodegraph {

class Node;
class LinkIterable;

enum ConnectorMode { INPUT, OUTPUT };
struct Link {
	Node* a;
	Node* b;
	unsigned ia:8;
	unsigned ib:8;
	unsigned type:8;	// From type
};

struct Connector {
	gui::Widget* widget;
	unsigned output:1;	// Connecor mode
	unsigned type:7;	// type masking
	unsigned single:1;	// Only allow a single link
	unsigned index:7;	// index in input or output list
};

/// Generic node editor class
class NodeEditor : public gui::Widget {
	WIDGET_TYPE(NodeEditor)
	friend class Node;
	public:
	NodeEditor();
	~NodeEditor();
	virtual void draw() const override;
	public:
	void setMousePanButtonMask(int mask) { m_mousePanButtonMask = mask; }
	void setConnectorIconSet(gui::IconList*);
	void setConnectorType(unsigned type, int icon=0, unsigned colour=0xffffffff, unsigned validMask=0u);
	static int areNodesConnected(const Node* a, const Node* b, unsigned connectorMask=~0u, unsigned limit=999);

	public:
	bool link(Node* a, int out, Node* b, int in);
	void unlink(Node* a, int out, Node* b, int in);
	void unlink(Node*);
	public:
	Delegate<void(Node*)> eventDelete;			// A node was deleted (called after it is unlinked)
	Delegate<void(NodeEditor*)> eventChanged;	// Called whenever the graph is modified
	Delegate<void(const Link&)> eventLinked;	
	Delegate<void(const Link&)> eventUnlinked;

	protected:
	void drawCurve(const Point* bezier, unsigned colour, float width=1, int n=32) const;
	bool validateConnection(const Connector* a, const Connector* b) const;
	bool isValid(unsigned fromType, unsigned toType) const;
	void getSplineControlPoints(const Link& link, Point points[4]) const;
	bool overLink(const Link& link, const Point& absolute, float distance) const;
	Point getConnectionPoint(const Connector& c) const;
	void unlink(Link*);
	private:
	void onMouseMove(const Point& last, const Point& pos, int b) override;
	void onMouseButton(const Point&, int, int) override;
	void onKey(int code, wchar_t chr, gui::KeyMask mask) override;
	private:
	const Connector* m_dragLink = 0;
	const Link* m_overLink = 0;
	gui::IconList* m_connectorIcons = 0;
	struct ConnectorType { int icon; unsigned colour; unsigned validMask; };
	std::vector<ConnectorType> m_connectorTypes;
	int m_mousePanButtonMask = 4; // Right mouse button
	Point m_boxStart;
	Point m_boxEnd;
};




class Node : public gui::Button {
	WIDGET_TYPE(Node)
	friend class LinkIterable;
	friend class NodeEditor;
	public:
	Node();
	void initialise(const gui::Root*, const gui::PropertyMap&) override;
	void onMouseButton(const Point& p, int,int) override;
	void onMouseMove(const Point&, const Point&, int) override;
	void onKey(int code, wchar_t chr, gui::KeyMask mask) override;
	void copyData(const Widget* from) override;
	public:
	NodeEditor* getEditor() const;
	void setConnectorTemplates(const gui::Widget* input, const gui::Widget* output);
	void addInput(const char* name, unsigned type=0, bool single=true);
	void addOutput(const char* name, unsigned type=0, bool single=false);
	int getInputCount() const { return m_inputs.size(); }
	int getOutputCount() const { return m_outputs.size(); }
	int getConnectorCount(ConnectorMode mode) const;
	const char* getConnectorName(ConnectorMode mode, unsigned index) const;
	unsigned getConnectorType(ConnectorMode mode, unsigned index) const;
	void forceStartDrag();
	void setPersistant(bool persist) { m_canBeDeleted = !persist; }
	bool editConnector(ConnectorMode, unsigned index, const char* name, unsigned type, bool single);
	bool editConnector(ConnectorMode, unsigned index, const char* name, unsigned type=0);
	bool removeConnector(ConnectorMode, unsigned);

	// Const accesors for iteration
	const std::vector<Connector>& getInputs() const { return m_inputs; }
	const std::vector<Connector>& getOutputs() const { return m_outputs; }

	LinkIterable getLinks(ConnectorMode m) const;	// Get all links
	LinkIterable getLinks(ConnectorMode m, unsigned index) const; // Get all links attached to a connector
	int isLinked(ConnectorMode m, int index) const;

	void setScale(float s);

	protected:
	Connector* createConnector(const char* name, unsigned mode, unsigned type, bool single);
	Widget* getConnectorWidget(ConnectorMode mode, unsigned index) const;
	const Connector* getConnector(gui::Widget*) const;
	const Connector* getTarget(Widget* src, const Point&) const;
	void connectorPress(Widget* w, const Point& p, int b);
	void connectorRelease(Widget* w, const Point& p, int b);
	void connectorDrag(Widget* w, const Point& p, int b);
	int disconnect(ConnectorMode mode, size_t index);
	static Node* getNodeFromWidget(Widget*);

	protected:
	Point m_heldPoint;
	std::vector<Connector> m_inputs, m_outputs;
	std::vector<Link*> m_links;
	const gui::Widget* m_connectorTemplate[2];
	gui::Widget* m_connectorClient[2] = {0,0};
	bool m_canBeDeleted = true;
};


class LinkIterable {
	friend class Node;
	typedef std::vector<Link*>::const_iterator Iter;
	const Node* m_node;
	unsigned m_index;
	bool m_outputs;
	LinkIterable(const Node* node, bool outputs, unsigned index=-1) : m_node(node), m_index(index), m_outputs(outputs) {}
	inline bool match(const Link& link) const {
		if(m_outputs != (link.a==m_node)) return false;
		if(m_index!=-1u && (m_outputs? link.ia: link.ib)!=m_index) return false;
		return true;
	}
	inline Iter findNext(const Iter& start) const {
		for(Iter i=start; i!=m_node->m_links.end(); ++i) if(match(*(*i))) return i;
		return m_node->m_links.end();
	}
	public:

	struct Iterator {
		Iter current;
		const LinkIterable* parent;
		const Link& operator*() const { return *(*current); }
		const Link* operator->() const { return *current; }
		Iterator operator++(int) { Iterator t(*this); ++*this; return t; }
		Iterator& operator++() { current=parent->findNext(++current); return *this; }
		bool operator==(const Iterator& i) const { return current==i.current; }
		bool operator!=(const Iterator& i) const { return current!=i.current; }
	};
	Iterator begin() const { return Iterator{findNext(m_node->m_links.begin()), this}; }
	Iterator end() const { return Iterator{m_node->m_links.end(), this}; }
};


}


