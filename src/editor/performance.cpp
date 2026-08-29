#include <base/editor/performance.h>
#include <base/profiler.h>
#include <base/gui/widgets.h>
#include <base/gui/lists.h>
#include <base/gui/renderer.h>

#include <base/game.h>
#include <base/input.h>

using namespace editor;
using namespace gui;
using base::Profiler;

namespace editor {
class GraphWidget : public Widget {
	WIDGET_TYPE(GraphWidget);
	const Performance* m_src;
	public:
	GraphWidget(const Performance* src) : m_src(src) { setTangible(Tangible::NONE); }
	void draw() const override {
		if(!isVisible()) return;
		drawSkin();
		for(const Performance::GraphLine& line : m_src->m_graphLines) {
			getRoot()->getRenderer()->drawLineStrip(line.line.size(), line.line.data(), 1, getPosition(), line.colour);
		}
	}
};
}

void Performance::initialise() {
	// Create widgets
	Widget* root = getEditor()->getGUI()->getRootWidget();
	m_blocks = new Widget();
	m_graph = new GraphWidget(this);
	m_list = new Listbox();
	m_list->setSkin(root->getRoot()->getSkin("listitem"));
	m_list->setPosition(200, 300);
	m_graph->setPosition(0, 300);
	m_graph->setSize(200, 100);
	addGraphLine(0, 0xffffffff);



	root->add(m_blocks);
	root->add(m_graph);
	root->add(m_list);
	deactivate();



	// Editor button
	Button* b = getEditor()->addButton("editors", "performance");
	b->eventPressed.bind([this](Button* b) {
		bool on = !isActive();
		if(m_blocks) m_blocks->setVisible(on);
		if(m_graph) m_graph->setVisible(on);
		if(m_list) m_list->setVisible(on);
		if(on) activate();
		else deactivate();
		b->setSelected(on);
	});
}

bool Performance::isActive() const {
	return m_blocks->isVisible();
}

void Performance::activate() {
	m_blocks->setVisible(true);
	m_graph->setVisible(true);
	m_list->setVisible(true);
}

void Performance::deactivate() {
	m_blocks->setVisible(false);
	m_graph->setVisible(false);
	m_list->setVisible(false);
}

void Performance::addGraphLine(uint id, uint colour) {
	constexpr int size = 64;
	m_graphLines.push_back({id, colour, {size, m_graph->getSize()}});
	int width = m_graph->getSize().x;
	for(int i=0; i<size; ++i) m_graphLines.back().line[i].x = i * width / (size-1);
}

void Performance::removeGraphLine(uint id) {
	for(size_t i=0; i<m_graphLines.size(); ++i) {
		if(m_graphLines[i].id == id) {
			m_graphLines[i] = std::move(m_graphLines.back());
			m_graphLines.pop_back();
		}
	}
}
void Performance::clearGraphLines() {
	m_graphLines.clear();
}

void Performance::update() {
	if(!isActive()) return;
	static bool refreshBlocks = true;
	if(base::Game::Key(base::KEY_F8)) refreshBlocks = true;

	// Display statistics
	const Profiler& profiler = Profiler::getInstance();
	Listbox* l = cast<Listbox>(m_list);
	l->clearItems();
	l->setMultiSelect(true);
	const float ms = 1000.f / base::Game::getTickFrequency();
	const std::vector<Profiler::Data>& data = profiler.getData();
	for(size_t i=0; i<data.size(); ++i) {
		if(data[i].count<=1) l->addItem(String::format("%s : %gms\n", profiler.getName(i), data[i].total * ms));
		else l->addItem(String::format("%s : %gms [x%d %g - %g ~%g]\n", profiler.getName(i), data[i].total * ms, data[i].count, data[i].min*ms, data[i].max*ms, data[i].total * ms / data[i].count));
	}
	l->setSize(200, l->getItemHeight() * l->getItemCount());
	for(GraphLine& g: m_graphLines) l->selectItem(g.id, false);

	// Graphs
	int height = m_graph->getSize().y;
	for(GraphLine& line: m_graphLines) {
		float value = data[line.id].count? (float)data[line.id].total / data[line.id].count: 0;
		for(size_t i=1; i<line.line.size(); ++i) line.line[i-1].y = line.line[i].y;
		line.line.back().y = height - value * ms * 10;
	}
	
	// Build block diagram
	if(refreshBlocks) {
		refreshBlocks = false;
		const Profiler::Frame& frame = profiler.getFrame();
		if(frame.children.empty()) return;
		Profiler::Frame root{0, frame.children[0].start, frame.children.back().end};
		Skin* skin = m_blocks->getRoot()->getSkin("panel");
		float scale = 800.f / (root.end-root.start);
		auto buildBlocks = [this, skin, &profiler, &root, scale](const Profiler::Frame& frame, int row, auto& recurse)->void {
			int x = (frame.start - root.start) * scale;
			int w = (frame.end - root.start) * scale - x;
			if(w < 2) return;
			Button* block = new Button();
			block->setSkin(skin);
			block->setPosition(x, row*20);
			block->setSize(w, 20);
			if(w > 20) block->setCaption(profiler.getName(frame.id));
			m_blocks->add(block);
			for(const Profiler::Frame& c: frame.children) recurse(c, row+1, recurse);
		};
		m_blocks->deleteChildWidgets();
		for(const Profiler::Frame& c: frame.children) buildBlocks(c, 0, buildBlocks);
		m_blocks->setAutosize(true);
	}
}






