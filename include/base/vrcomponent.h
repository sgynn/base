#pragma once

#include <base/gamestate.h>
#include <base/math.h>

namespace base { class Scene; class SceneNode; class CompositorGraph; class Renderer; class FrameBuffer; class Camera; class Workspace; class Joystick; }

namespace vr {
	enum Role {
		HEAD,
		HAND_LEFT,
		HAND_RIGHT,
		WAIST,
		FOOT_LEFT,
		FOOT_RIGHT,
	};
	enum Side {
		LEFT = 0,
		RIGHT = 1,
	};
	enum ControlAxis {
		AXIS_X,
		AXIS_Y,
		TRIGGER,
		GRIP,
		PAD_X,
		PAD_Y,
	};
	enum ControllerButton {
		BUTTON_A,
		BUTTON_B,
		BUTTON_MENU,
		STICK_PRESS,
		PAD_TOUCH,
		PAD_PRESS
	};
	
	struct Transform {
		Quaternion orientation;
		vec3 position;
	};
};


class VRComponent : public base::GameStateComponent {
	public:
	static VRComponent* create(const char* name, base::Scene* scene=nullptr, const base::CompositorGraph* graph=nullptr);
	VRComponent(const char* name, base::Scene* scene=nullptr, const base::CompositorGraph* graph=nullptr);
	~VRComponent();
	void setScene(base::Scene* s);
	bool setCompositor(const base::CompositorGraph*);
	void adjustDepth(float nearClip, float farClip);
	bool isActive() const;
	bool isEnabled() const;
	BoundingBox2D getPlayArea() const;
	bool hasTrackedObject(vr::Role) const;
	const vr::Transform& getTransform(vr::Role c = vr::HEAD) const { return m_controllerPose[c]; }
	const base::Joystick& getController(vr::Side c) const;
	const base::Workspace* getWorkspace() { return m_workspace; }

	vr::Transform& getRootTransform() { return m_rootTransform; }
	const vr::Transform& getRootTransform() const { return m_rootTransform; }
	void setRootTransform(const vec3& pos, const Quaternion& rot) { m_rootTransform.position = pos; m_rootTransform.orientation = rot; }
	vr::Transform getDerivedTransform(vr::Role c = vr::HEAD) const {
		const vr::Transform& t = getTransform(c);
		return { m_rootTransform.orientation * t.orientation, m_rootTransform.position + m_rootTransform.orientation * t.position };
	}

	bool hasHandJoints(vr::Side) const;
	vr::Transform getHandJoint(vr::Side hand, int index) const;

	void update() override;
	void draw() override;
	private:
	void updateTransforms(uint64);

	protected:
	base::Scene* m_scene = nullptr;
	base::Camera* m_camera = nullptr;
	base::Workspace* m_workspace = nullptr;
	base::Renderer* m_renderer = nullptr;
	base::FrameBuffer* m_target = nullptr;
	uint m_controllerIndex[2] = { -1u, -1u };
	vr::Transform m_controllerPose[6];
	vr::Transform m_rootTransform;
};


