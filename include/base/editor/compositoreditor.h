#pragma once

#include "editor.h"
#include <base/gui/widgets.h>
#include <base/gui/lists.h>

namespace nodegraph { class NodeEditor; class Node; }
namespace base { class Compositor; class CompositorGraph; class CompositorPass; class Workspace; }

namespace editor {

class CompositorNode;


class CompositorEditor : public EditorComponent {
	public:
	struct GraphInfo {
		base::CompositorGraph* graph;
		std::vector<Point> nodes;
		bool operator==(const GraphInfo& o) const { return o.graph==graph; }
	};

	public:
	~CompositorEditor();
	void initialise() override;
	bool isActive() const override;
	void activate() override;
	void deactivate() override;
	void refresh() override;

	void refreshCompositorList();
	void addCompositor(const char* name, base::Compositor*);
	void setGraph(base::CompositorGraph*);
	void setGraph(const GraphInfo&);
	void setCompositor(base::Compositor*);

	void exportGraph(base::CompositorGraph*) const;

	protected:
	gui::Widget* addDataWidget(const char* name, gui::Widget* parent);
	gui::Widget* addConnectorWidget(gui::Widget* parent, const char*);
	gui::Widget* addBufferWidget(gui::Widget* parent);
	gui::Widget* addPassWidget(gui::Widget* parent, const char* type);

	CompositorNode* createGraphNode(base::Compositor*, int alias);
	CompositorNode* getGraphNode(base::Compositor*, int index=0);
	CompositorNode* getGraphNode(int alias);
	const char* getCompositorName(base::Compositor*, const char* outputName="Output") const;
	base::CompositorGraph* buildGraph() const;
	bool applyGraph(base::CompositorGraph*);
	void layoutGraph();
	

	protected:
	void dragCompositor(gui::Widget*, const Point&, int);
	void selectCompositor(gui::Listbox*, gui::ListItem&);
	void selectGraph(gui::Listbox*, gui::ListItem&);
	void newCompositor(gui::Button*);
	void newGraph(gui::Button*);

	void addPass(gui::Button*);
	void addScenePass(gui::Button*);
	void addQuadPass(gui::Button*);
	void addCopyPass(gui::Button*);
	void addClearPass(gui::Button*);
	void addPass(const char* widget, base::CompositorPass*);
	void removePass(gui::Button*);

	void addConnector(int mode, gui::Widget* list, const char* name);
	void renameConnector(gui::Textbox*);
	void removeItem(gui::Button*);

	void addBuffer(gui::Button*);
	void removeBuffer(gui::Button*);

	void selectNode(gui::Button*);
	void graphChanged(nodegraph::NodeEditor*);


	void buildInputConnectors(gui::Widget* list);
	void buildOutputConnectors(gui::Widget* list);
	void refreshTargetsList();

	template<class F> void eachActiveNode(F&& func);

	protected:
	gui::Widget* m_panel = 0;
	base::CompositorGraph* m_graph = 0;
	base::Compositor* m_compositor = 0;
	gui::Listbox* m_graphList = 0;
	gui::Listbox* m_nodeList = 0;
	int m_compositorIndex = 0;
	nodegraph::NodeEditor* m_graphEditor = 0;
	gui::Popup* m_passTypePopup = 0;
	gui::ItemList* m_targets = 0;
	gui::ItemList* m_formats = 0;

	gui::Widget* m_sceneTemplate = 0;
	gui::Widget* m_clearTemplate = 0;
	gui::Widget* m_quadTemplate = 0;
	gui::Widget* m_copyTemplate = 0;
	gui::Widget* m_bufferTemplate = 0;
};

}

