#pragma once

#include <base/editor/editor.h>
#include <base/resources.h>
#include <base/gui/gui.h>

#include <base/shader.h>
#include <base/material.h>
#include <base/autovariables.h>
#include <base/framebuffer.h>
#include <base/drawablemesh.h>
#include <base/model.h>
#include <base/scene.h>
#include <base/orbitcamera.h>
#include <base/scenecomponent.h>
#include <base/compositor.h>

namespace editor {

class ModelViewer : public EditorComponent {
	public:
	void initialise() override {}
	gui::Widget* openAsset(const char* asset) override {
		using namespace base;
		using namespace gui;

		const char* ext = strrchr(asset, '.');
		if(ext && (strcmp(ext, ".bm")==0 || strcmp(ext, ".obj")==0)) {
			Model* model = Resources::getInstance()->models.get(asset);
			if(!model) return nullptr;

			Widget* previewTemplate = getEditor()->getGUI()->getWidget("filepreview");
			if(!previewTemplate) return nullptr;
			gui::Renderer& r = *getEditor()->getGUI()->getRenderer();

			// Create a scene TODO: cleanup
			Scene* scene = new Scene();
			SceneNode* node = scene->add("mesh");
			Material* mat = Resources::getInstance()->materials.getIfExists("editorPreview");
			if(!mat) {
				mat = createPreviewMaterial();
				Resources::getInstance()->materials.add("editorPreview", mat);
			}
			BoundingBox bounds; bounds.setInvalid();
			for(const auto& m : model->meshes()) {
				bounds.include(m.mesh->calculateBounds());
				node->attach(new DrawableMesh(m.mesh, mat));
			}
			float max = fmax(fmax(bounds.size().x, bounds.size().y), bounds.size().z);
			base::Renderer* renderer = getEditor()->getState()->getComponent<SceneComponent>()->getRenderer();
			FrameBuffer* target = new FrameBuffer(512, 512, Texture::RGBA8, Texture::D24S8);
			OrbitCamera* camera = new OrbitCamera(90, 1, 0.1, 100);
			camera->setPosition(2.5,2.5,max * 0.6);
			camera->setTarget(bounds.centre());
			Workspace* ws = createDefaultWorkspace();
			ws->setCamera(camera);
			ws->execute(target, scene, renderer);

			int img = r.addImage(asset, 512, 512, target->texture().unit());
			Widget* preview = previewTemplate->clone();
			preview->setSize(Point(512,512) + preview->getRect().size() - preview->getClientRect().size());
			Image* view = cast<Image>(preview->getWidget(0));
			view->setImage(img);
			view->eventMouseMove.bind([camera, ws, target, scene, renderer](Widget* w, const Point& p, int b) {
				if(b) {
					camera->update(CU_MOUSE);
					ws->execute(target, scene, renderer);
				}
			});
			view->eventMouseWheel.bind([camera, ws, target, scene, renderer](Widget* w, int delta) {
				camera->update(CU_WHEEL);
				ws->execute(target, scene, renderer);
			});
			cast<gui::Window>(preview)->eventResized.bind([camera, ws, target, scene, renderer](gui::Window* w) {
				Point s = w->getSize();
				camera->setAspect((float)s.x / (float)s.y);
				ws->execute(target, scene, renderer);
			});

			return preview;
		}
		return nullptr;
	}

	static base::Material* createPreviewMaterial() {
		static const char* shaderVS = R"-(#version 330
			in vec4 vertex;
			in vec3 normal;
			out vec3 norm;
			uniform mat4 transform;
			uniform mat4 modelView;
			void main() { gl_Position = transform * vertex; norm = mat3(modelView) * normal;})-";
		static const char* shaderFS = R"-(#version 330
			in vec3 norm;
			out vec4 fragment;
			void main() { fragment = vec4(norm.zzz, 1); })-";
		
		using namespace base;
		Material* mat = new Material();
		Pass* pass = mat->addPass();
		pass->setShader(Shader::create(shaderVS, shaderFS));
		pass->getParameters().setAuto("transform", AUTO_MODEL_VIEW_PROJECTION_MATRIX);
		pass->getParameters().setAuto("modelView", AUTO_MODEL_VIEW_MATRIX);
		pass->compile();
		return mat;
	}
};

}

