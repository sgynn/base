#pragma once

#include <base/editor/editor.h>

namespace base { class Scene; class Model; class OrbitCamera; class Workspace; class FrameBuffer; class Material; class Renderer; class Animation; }

namespace editor {
class ModelViewer : public EditorComponent {
	public:
	struct View {
		gui::Widget* window;
		base::Scene* scene;
		base::Model* model;
		base::OrbitCamera* camera;
		base::Material* material;
		base::Renderer* renderer;
		base::Workspace* workspace;
		base::FrameBuffer* target;
		base::Animation* animation;
		float frame;
		Point last;
	};
	void initialise() override {}
	gui::Widget* openAsset(const Asset&) override;
	void update() override;
	protected:
	base::Scene* createScene(base::Model* model, base::Material*);
	static void createSkeleton(View&);
	static void autosizeView(const View&);
	static base::Material* createPreviewMaterial();
	void applyAnimationPose(View&, float frame=0);

	private:
	std::vector<View*> m_views;
};
}

