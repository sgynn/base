#pragma once

#include <base/texture.h>
#include <base/framebuffer.h>
#include <base/hardwarebuffer.h>
#include <base/vrcomponent.h>

#define XR_USE_GRAPHICS_API_OPENGL
#include <openxr/openxr.h>

namespace base { class Material; class RenderState; }

namespace xr {
	class SwapChainImage {
		public:
		SwapChainImage(unsigned glTexture=0) : m_target(glTexture) {}
		virtual ~SwapChainImage() {}
		virtual void beginFrame() {}
		virtual void endFrame() {}
		const base::Texture& getTarget() const { return m_target; }
		protected:
		base::Texture m_target;
	};

	struct Swapchain {
		uint width, height;
		XrSwapchain colourHandle;
		XrSwapchain depthHandle;
		std::vector<SwapChainImage*> images;
		std::vector<SwapChainImage*> depth;
	};

	struct HandController {
		vr::Role role;
		bool active = false;
		XrSpace space = 0;
		XrPath subaction = XR_NULL_PATH;
		XrHandJointLocationEXT jointLocations[XR_HAND_JOINT_COUNT_EXT];
		XrHandTrackerEXT handTracker = 0;
	};

	/// OpenGL -> DirectX bridge needs an extra compositor pass to vertically flip the image
	class FlipTarget {
		public:
		FlipTarget(int width, int height, unsigned colour, unsigned depth);
		~FlipTarget();
		base::FrameBuffer* getTarget() { return &m_buffer; }
		void execute(base::FrameBuffer* out, base::RenderState& state);
		private:
		base::FrameBuffer m_buffer;
		base::HardwareVertexBuffer m_vx;
		base::Material* m_material;
		unsigned m_geometry;
	};
}

