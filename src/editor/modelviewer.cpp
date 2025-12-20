#include "modelviewer.h"
#include <base/editor/editor.h>
#include <base/resources.h>
#include <base/gui/widgets.h>
#include <base/gui/lists.h>
#include <base/gui/renderer.h>
#include <base/gui/menubuilder.h>

#include <base/shader.h>
#include <base/material.h>
#include <base/autovariables.h>
#include <base/framebuffer.h>
#include <base/drawablemesh.h>
#include <base/hardwarebuffer.h>
#include <base/model.h>
#include <base/scene.h>
#include <base/orbitcamera.h>
#include <base/scenecomponent.h>
#include <base/compositor.h>

using namespace editor;
using namespace base;
using namespace gui;

Material* ModelViewer::createPreviewMaterial() {
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
		void main() { fragment = vec4(normalize(norm).zzz, 1); })-";
	
	static Shader* shader = Shader::create(shaderVS, shaderFS);
	Material* mat = new Material();
	Pass* pass = mat->addPass();
	pass->setShader(shader);
	pass->getParameters().setAuto("transform", AUTO_MODEL_VIEW_PROJECTION_MATRIX);
	pass->getParameters().setAuto("modelView", AUTO_MODEL_VIEW_MATRIX);
	pass->compile();
	return mat;
}

Widget* ModelViewer::openAsset(const Asset& asset) {
	if(!asset.file) return nullptr;
	StringView ext = strrchr(asset.file.name, '.');
	if(ext==".bm" || ext==".obj") {
		String name = asset.resource? asset.resource: asset.file.getLocalPath();
		Model* model = Resources::getInstance()->models.get(name);
		if(!model) return nullptr;

		Widget* previewTemplate = getEditor()->getGUI()->getWidget("filepreview");
		if(!previewTemplate) return nullptr;
		Material* mat = createPreviewMaterial();

		View* viewPtr = new View();
		View& view = *viewPtr;
		view.scene = createScene(model, mat);
		view.model = model;
		view.camera = new OrbitCamera(90, 1, 0.1, 100);
		view.material = mat;
		view.renderer = getEditor()->getState()->getComponent<SceneComponent>()->getRenderer();
		view.target = new FrameBuffer(512, 512, Texture::RGBA8, Texture::D24S8);
		view.workspace = createDefaultWorkspace();
		view.workspace->setCamera(view.camera);
		view.animation = nullptr;
		view.frame = 0;

		gui::Renderer& r = *getEditor()->getGUI()->getRenderer();
		int img = r.addImage(name, 512, 512, view.target->texture().unit());
		Widget* preview = previewTemplate->clone();
		preview->setSize(Point(512,512) + preview->getRect().size() - preview->getClientRect().size());
		preview->setName(name);
		view.window = preview;

		enum ViewMode { LAYOUT, MESH, ANIMATION };
		Combobox* parts = cast<Combobox>(preview->getWidget(0));
		if(model->getMeshCount() > 1) {
			parts->addItem(model->getLayout()? "Layout": "Combined", LAYOUT);
			for(auto& mesh: model->meshes()) {
				if(!parts->findItem(mesh.name)) parts->addItem(mesh.name, MESH);
			}
		}
		if(model->getAnimationCount() && model->getSkeleton()) {
			createSkeleton(view);
			if(model->getMeshCount() == 1) parts->addItem("Mesh", LAYOUT);
			for(size_t i=0; i<model->getAnimationCount(); ++i) {
				parts->addItem(model->getAnimation(i)->getName(), ANIMATION, model->getAnimation(i));
			}
		}

		gui::Image* viewport = cast<gui::Image>(preview->getWidget(1));
		viewport->setImage(img);
		viewport->eventMouseMove.bind([&view](Widget* w, const Point& p, int b) {
			if(b==4) { // Rotate
				view.camera->update(CU_MOUSE);
				view.workspace->execute(view.target, view.scene, view.renderer);
			}
			else if(b==1) { // Pan
				vec3 t = view.camera->getTarget();
				float scale = t.distance(view.camera->getPosition()) * 0.005;
				t -= view.camera->getUp() * (p.y - view.last.y) * scale;
				t -= view.camera->getLeft() * (p.x - view.last.x) * scale;
				view.camera->setTarget(t);
				view.workspace->execute(view.target, view.scene, view.renderer);
			}
			view.last = p;
		});

		viewport->eventMouseWheel.bind([&view](Widget* w, int delta) {
			view.camera->update(CU_WHEEL);
			view.workspace->execute(view.target, view.scene, view.renderer);
		});

		cast<gui::Window>(preview)->eventResized.bind([&view](gui::Window* w) {
			Point s = w->getSize();
			view.camera->setAspect((float)s.x / (float)s.y);
			view.workspace->execute(view.target, view.scene, view.renderer);
		});

		parts->eventSelected.bind([this, &view](Combobox*, ListItem& item) {
			ViewMode mode = item.getValue(1, LAYOUT);
			view.animation = nullptr;
			for(SceneNode* n: view.scene->getRootNode()->children()) {
				if(strcmp(n->getName(), "meshes")==0) {
					n->setVisible(mode==MESH || (mode==LAYOUT && !view.model->getLayout()));
					for(Drawable* d: n->attachments()) {
						if(mode == LAYOUT) d->setVisible(true);
						else {
							Mesh* mesh = static_cast<DrawableMesh*>(d)->getMesh();
							for(auto& m: view.model->meshes()) {
								if(m.mesh == mesh) {
									d->setVisible(strcmp(m.name, item)==0);
									break;
								}
							}
						}
					}
				}
				else if(strcmp(n->getName(), "layout")==0) {
					n->setVisible(mode == LAYOUT);
				}
				else if(strcmp(n->getName(), "animation")==0) {
					view.frame = 0;
					n->setVisible(mode == ANIMATION);
					view.animation = item.getValue<Animation*>(2, nullptr);
					applyAnimationPose(view);
				}
			}
			autosizeView(view);
			view.workspace->execute(view.target, view.scene, view.renderer);
		});

		if(parts->getItemCount()) parts->selectItem(0, true);
		else {
			parts->setVisible(false);
			autosizeView(view);
			view.workspace->execute(view.target, view.scene, view.renderer);
		}

		m_views.push_back(viewPtr);
		return preview;
	}
	return nullptr;
}

Scene* ModelViewer::createScene(Model* model, Material* mat) {
	Scene* scene = new Scene();
	if(model->getMeshCount()) {
		SceneNode* meshNode = scene->add("meshes");
		for(const auto& m : model->meshes()) {
			meshNode->attach(new DrawableMesh(m.mesh, mat));
		}
	}

	if(const ModelLayout* layout = model->getLayout()) {
		SceneNode* layoutNode = scene->add("layout");
		for(const ModelLayout::Node& node: *layout) {
			if(node.type == ModelLayout::MESH) {
				SceneNode* n = layoutNode;
				if(node.position!=vec3() || node.orientation!=Quaternion() || node.scale!=vec3(1,1,1)) {
					n = layoutNode->createChild();
					n->setTransform(node.position, node.orientation, node.scale);
				}
				for(auto& m: model->meshes()) {
					if(strcmp(m.name, node.object)==0) n->attach(new DrawableMesh(m.mesh, mat));
				}
			}
			// ToDo: Markers
		}
	}
	return scene;
}

void ModelViewer::createSkeleton(View& view) {
	// Make a static bone octohedron mesh
	static Mesh* boneMesh = nullptr;
	if(!boneMesh) {
		boneMesh = new Mesh();
		const float w = 0.06, k=0.1;
		vec3 points[6] = {{0,0,0}, {w,w,k}, {w,-w,k}, {-w,-w,k}, {-w,w,k}, {0,0,1} };
		unsigned char ix[24] = {0,1,2, 0,2,3, 0,3,4, 0,4,1, 1,5,2, 2,5,3, 3,5,4, 4,5,1, };
		vec3* vx = new vec3[24*2];
		for(int i=0; i<24; ++i) vx[i*2] = points[ix[i]];
		for(int i=0; i<24; i+=3) {
			vec3 n = (vx[i*2+2]-vx[i*2]).cross(vx[i*2+4]-vx[i*2]).normalise();
			vx[i*2+1] = vx[i*2+3] = vx[i*2+5] = n;
		}
		HardwareVertexBuffer* vb = new HardwareVertexBuffer();
		vb->attributes.add(VA_VERTEX, VA_FLOAT3);
		vb->attributes.add(VA_NORMAL, VA_FLOAT3);
		vb->setData(vx, 24, 6*sizeof(float), true);
		boneMesh->setVertexBuffer(vb);
	}

	// Create a drawable per bone, share transform of bone matrix
	SceneNode* node = view.scene->add("animation");
	for(Bone* bone: *view.model->getSkeleton()) {
		DrawableMesh* d = new DrawableMesh(boneMesh, view.material);
		node->attach(d);
		d->shareTransform(const_cast<Matrix*>(&bone->getAbsoluteTransformation()));
	}
}

void ModelViewer::autosizeView(const View& view) {
	BoundingBox bounds;
	bounds.setInvalid();

	// Calculate bounds
	view.scene->updateSceneGraph();
	std::vector<SceneNode*> stack = { view.scene->getRootNode() };
	for(size_t i=0; i<stack.size(); ++i) {
		for(Drawable* d: stack[i]->attachments()) {
			if(d->isVisible()) {
				d->updateBounds();
				bounds.include(d->getBounds());
			}
		}
		for(SceneNode* n: stack[i]->children()) {
			if(n->isVisible()) stack.push_back(n);
		}
	}

	float zoom = bounds.size().length() * 0.6;
	view.camera->setTarget(bounds.centre());
	view.camera->setPosition(2.5,2.5,zoom);
}

void ModelViewer::applyAnimationPose(View& view, float frame) {
	Skeleton* skeleton = view.model->getSkeleton();
	skeleton->resetPose();
	if(view.animation) skeleton->applyPose(view.animation, frame);
	skeleton->update();
	for(SceneNode* n: view.scene->getRootNode()->children()) {
		if(n->getName()[0]=='a') { // animation
			for(Bone* bone : *skeleton) {
				Matrix matrix = bone->getAbsoluteTransformation();
				matrix.scale(bone->getLength());
				n->getAttachment(bone->getIndex())->setTransform(matrix);
			}
			break;
		}
	}
	view.workspace->execute(view.target, view.scene, view.renderer);
}

void ModelViewer::update() {
	for(View* v: m_views) {
		if(v->window->isVisible() && v->animation && v->animation->getLength() > 1) {
			v->frame += 1/60.f * v->animation->getSpeed();
			if(v->frame > v->animation->getLength()) v->frame -= v->animation->getLength();
			applyAnimationPose(*v, v->frame);
		}
	}
}

