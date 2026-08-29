#pragma once

#include <base/editor/editor.h>

namespace editor {
class Performance : public EditorComponent {
	public:
	void initialise() override;
	void activate() override;
	void deactivate() override;
	void update() override;
	bool isActive() const override;

	void addGraphLine(uint id, uint colour);
	void removeGraphLine(uint id);
	void clearGraphLines();


	private:
	gui::Widget* m_blocks;	// Frame block diagram
	gui::Widget* m_graph;	// Performance graph over time
	gui::Widget* m_list;	// Flat list of all profile data for the frame

	struct GraphLine {
		uint id;
		uint colour;
		std::vector<Point> line;
	};
	std::vector<GraphLine> m_graphLines;
	friend class GraphWidget;
};
REGISTER_EDITOR_COMPONENT(Performance);
}


// Sneaky way to inject into SceneComponent
#ifdef BASE_SCENE_COMPONENT
#include <base/profiler.h>
namespace base {
	class ProfiledSceneComponent : public SceneComponent {
		public:
		using SceneComponent::SceneComponent;
		void update() override { PROFILE_BLOCK(SceneUpdate); SceneComponent::update(); }
		void draw() override { PROFILE_BLOCK(SceneDraw); SceneComponent::draw(); }
	};
}
#define SceneComponent ProfiledSceneComponent
#endif


