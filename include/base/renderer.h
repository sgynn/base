#pragma once

#include <vector>
#include <base/point.h>

namespace base { 
	class Camera;
	class FrameBuffer;
	class HardwareIndexBuffer;
	class HardwareVertexBuffer;

	class Pass;
	class Material;
	class AutoVariableSource;
	class Drawable;
	class Blend;
	class MacroState;
	class StencilState;

	enum class RenderQueueMode : char { Normal, Sorted, SortedInverse, Disabled };

	// Render state holds the active state of the renderer
	class RenderState {
		public:
		RenderState();
		~RenderState();

		void setCamera(base::Camera*);
		void setTarget(const base::FrameBuffer*);
		void setTarget(const base::FrameBuffer*, const Rect& viewport);
		void setMaterialPass(Pass*);
		void setMaterial(Material*);
		void setMaterialOverride(Material*);
		void setMaterialTechnique(size_t);
		void setMaterialTechnique(const char*);
		void setMaterialBlendOverride(const Blend& blend);
		void setMaterialStateOverride(const MacroState&);
		void setStencilOverride(const StencilState&);
		void unbindVertexBuffers();
		void reset();	// Try to put the renderer back to its default state

		base::Camera* getCamera() const { return m_camera; }
		AutoVariableSource* getVariableSource() const { return m_auto; }
		
		private:
		AutoVariableSource*  m_auto;
		base::Camera*        m_camera = nullptr;
		Pass*                m_activePass = nullptr;
		Material*            m_materialOverride = nullptr;
		size_t               m_materialTechnique = 0;
		Blend*               m_blendOverride = nullptr;
		MacroState*          m_stateOverride = nullptr;
		StencilState*        m_stencilOverride = nullptr;
		Rect                 m_viewport;
	};

	
	/// Renderer class is the current render state and an ordered list of drawables
	class Renderer {
		public:
		Renderer();
		~Renderer();

		void clear();
		void add(Drawable*, unsigned char queue=0);
		void remove(Drawable*, unsigned char queue=0); /// @deprecated
		void setQueueMode(unsigned char queue, RenderQueueMode mode);

		void render(unsigned char first=0, unsigned char last=255);
		void clearScreen();

		RenderState& getState() { return m_state; }
		RenderQueueMode getQueueMode(unsigned char queue) const { return m_queueMode[queue]; }

		protected:
		std::vector<Drawable*> m_drawables[256];
		RenderQueueMode        m_queueMode[256];
		RenderState            m_state;

	};

}

