#pragma once

#include <base/hashmap.h>
#include <base/texture.h>
#include <base/material.h>
#include <vector>

namespace base {
	typedef unsigned char uint8;
	class FrameBuffer;
	class RenderState;
	class Texture;
	class Renderer;
	class Material;
	class Camera;
	class Scene;
	class Pass;

	// Name indexer thing
	class NameLookup {
		public:
		size_t operator()(const char* s) { return get(s); }
		const char* name(size_t id) const { return id<m_names.size()? m_names[id]: 0; };
		size_t get(const char* s) { 
			size_t id = m_lookup.get(s, m_names.size());
			if(id==m_names.size()) m_lookup.insert(s,id), m_names.push_back(m_lookup.find(s)->key);
			return id;
		}
		private:
		base::HashMap<size_t> m_lookup;
		std::vector<const char*> m_names;
	};
	// ---------------------------------- //
	
	class MaterialResolver {
		public:
		virtual Material* resolveMaterial(const char* name) = 0;
		virtual base::Texture* resolveTexture(const char* name) = 0;
	};


	enum ClearBits { CLEAR_COLOUR=1, CLEAR_DEPTH=2, CLEAR_STENCIL=4 };

	/// Compositor pass virtual base class
	class CompositorPass {
		public:
		CompositorPass() {}
		virtual ~CompositorPass() {}
		virtual void execute(const base::FrameBuffer*, const Rect& view, Renderer*, base::Camera*, Scene*) const = 0;
		virtual void resolveExternals(MaterialResolver*) {}
		virtual const char* getCamera() const { return 0; }
	};
	/// Clear target buffer
	class CompositorPassClear final : public CompositorPass {
		public:
		CompositorPassClear(char clearBits=3, uint colour=0, float depth=1);
		CompositorPassClear(char clearBits, const float* colour, float depth=1);
		void execute(const base::FrameBuffer*, const Rect& view, Renderer*, base::Camera*, Scene*) const override;
		void setColour(float r, float g, float b, float a=1) { mColour[0]=r; mColour[1]=g; mColour[2]=b; mColour[3]=a; }
		void setDepth(float value) { mDepth=value; }
		void setBits(unsigned bits) { mBits=bits; }
		const float* getColour() const { return mColour; }
		float getDepth() const { return mDepth; }
		unsigned getBits() const { return mBits; }
		protected:
		float    mColour[4];
		float    mDepth;
		unsigned mBits;
	};
	/// Draw a fullscreen quad
	class CompositorPassQuad final : public CompositorPass {
		public:
		CompositorPassQuad(const char* material, const char* tech=0);
		CompositorPassQuad(Material* material=0, const char* tech=0);
		~CompositorPassQuad();
		void execute(const base::FrameBuffer*, const Rect& view, Renderer*, base::Camera*, Scene*) const override;
		void resolveExternals(MaterialResolver*) override;			// Resolve texture and material  classes from resource names
		void setMaterial(Material*, const char* tech=0, bool isInstance=false);	// Set material. isInstance specifies it is a material instance owned by this pass
		void setTexture(const char* name, const base::Texture*, bool compile=true);	// Set texture
		void setTexture(const char* name, const char* source);		// Set texture by name to be resolved later
		protected:
		void setupInstancedMaterial();
		Pass* getMaterialPass();
		protected:
		char*     mMaterialName;
		size_t    mTechnique;
		Material* mMaterial;
		uint      mGeometry; // vaobj
		bool      mInstancedMaterial;
		struct TextureOverride { char* source; const base::Texture* texture; };
		base::HashMap<TextureOverride> mTextures;
		protected:
		static uint sGeometry;
		static void createGeometry();
	};
	/// Draw scene items
	class CompositorPassScene final : public CompositorPass {
		public:
		CompositorPassScene(uint8 queue, const char* technique=0);
		CompositorPassScene(uint8 first, uint8 last, const char* technique=0);
		CompositorPassScene(uint8 first, uint8 last, const char* materialName, const char* technique);
		CompositorPassScene(uint8 first, uint8 last, Material* material, const char* technique=0);
		~CompositorPassScene();

		void setMaterial(Material* m, const char* technique);
		void setCamera(const char* camera);
		void setMaterial(Material* m) { mMaterial=m; }
		void setBlend(const Blend& blend);
		void setStencil(const StencilState&);
		void setState(const MacroState&);
		void clearMaterialOverides();

		void execute(const base::FrameBuffer*, const Rect& view, Renderer*, base::Camera*, Scene*) const override;
		const char* getCamera() const override { return mCameraName; }
		void resolveExternals(MaterialResolver*) override;

		protected:
		uint8         mFirst, mLast;      // Render queues to draw
		char*         mMaterialName;      // Override material name
		size_t        mTechnique;         // Material technique to use
		char*         mCameraName;        // Camera name
		Material*     mMaterial;          // Resolved material - how to set this ?? 
		Blend         mBlend;             // Set blend state override
		MacroState    mState;             // Material state override
		StencilState  mStencil;           // Stencil state override
		int           mOverrideFlags = 0; // What to override
	};
	/// Copy a texture
	class CompositorPassCopy final : public CompositorPass {
		public:
		CompositorPassCopy(const char* source, int index=0);
		void execute(const base::FrameBuffer*, const Rect& view, Renderer*, base::Camera*, Scene*) const override;
		protected:
		size_t mSource;
		int    mSourceIndex;
	};

	// ===================================================================================================== //

	// Compositor data
	class Compositor {
		public:
		Compositor(const char* name=0);
		~Compositor();

		using Format = Texture::Format;
		static constexpr char DepthIndex = -1;

		// Buffer attachments can be a format, or a texture from graph input
		struct BufferAttachment {
			Format format = Format::NONE;
			char* input = nullptr;
			char part = 0;
		};

		// Frame buffer definition
		struct Buffer {
			char  name[64];
			int   width, height;
			float relativeWidth, relativeHeight; // use these if nonzero
			BufferAttachment colour[4];
			BufferAttachment depth;
			bool  unique; // Non-unique buffers are re-used where possible
			base::Texture* texture; // use this instead - read only
		};



		// Check compositor for errors
		bool validate();

		/// Add pass to this compositor
		void addPass(const char* target, CompositorPass*);

		Buffer* addBuffer(const char* name, int w, int h, Format format, Format depth=Format::NONE, bool unique=false);
		Buffer* addBuffer(const char* name, int w, int h, Format f1, Format f2, Format depth, bool unique=false);
		Buffer* addBuffer(const char* name, int w, int h, Format f1, Format f2, Format f3, Format depth, bool unique=false);
		Buffer* addBuffer(const char* name, int w, int h, Format f1, Format f2, Format f3, Format f4, Format depth, bool unique=false);

		Buffer* addBuffer(const char* name, float w, float h, Format format, Format depth=Format::NONE, bool unique=false);
		Buffer* addBuffer(const char* name, float w, float h, Format f1, Format f2, Format depth, bool unique=false);
		Buffer* addBuffer(const char* name, float w, float h, Format f1, Format f2, Format f3, Format depth, bool unique=false);
		Buffer* addBuffer(const char* name, float w, float h, Format f1, Format f2, Format f3, Format f4, Format depth, bool unique=false);

		Buffer* addTexture(const char* name, base::Texture*);

		void addInput(const char* name, const char* buffer=0);
		void addOutput(const char* name, const char* buffer=0);

		// Special output compositor defines screen (or whatever target)
		static Compositor* Output;

		public:
		int     getInput(const char*) const;
		int     getOutput(const char* name) const;
		Buffer* getBuffer(const char* name) const;

		const char* getInputName(uint index) const;
		const char* getOutputName(uint index) const;
		const char* getPassTarget(uint index) const;
		CompositorPass* getPass(uint index);
		Buffer* getBuffer(uint index);
		void removeInput(const char*);
		void removeOutput(const char*);
		void removeBuffer(const char*);
		void removePass(uint index);

		private:
		friend class Workspace;
		friend class CompositorGraph;
		struct Pass { char* target; CompositorPass* pass; };
		struct Connector { char* name; char* buffer; char part; };
		void addConnector(std::vector<Connector>& list, const char* name, const char* buffer);
		void removeConnector(std::vector<Connector>& list, int index);
		std::vector<Pass>      m_passes;
		std::vector<Buffer*>   m_buffers;
		std::vector<Connector> m_inputs;
		std::vector<Connector> m_outputs;
		char m_name[64];
	};

	// ===================================================================================================== //

	// Stores the graph connections which link compositors
	class CompositorGraph {
		public:
		struct Link { size_t a,b; int out,in; };

		public:
		CompositorGraph();
		~CompositorGraph();
		void link(Compositor*, Compositor*);
		void link(Compositor*, Compositor*, const char*);
		void link(Compositor*, Compositor*, const char*, const char*);
		void unlink(Compositor*, const char* key);
		void unlink(Compositor*);

		Compositor* getCompositor(size_t key) const;
		size_t      add(Compositor*);
		size_t      find(Compositor*, int n=0) const;
		void        link(size_t a, size_t b);
		void        link(size_t a, size_t b, const char*);
		void        link(size_t a, size_t b, const char*, const char*);

		size_t      size() const;	// Get number of compositor nodes in graph
		const std::vector<Link>& links() const { return m_links; }

		bool        requiresTargetSize() const; // Does we need to recreate the buffers if the target size changes

		void resolveExternals(MaterialResolver*);	// Resolve any material or textures referenced by name
		protected:
		friend class Workspace;
		std::vector<Compositor*> m_compositors;
		std::vector<Link> m_links;

	};

	// Compiled compositor graph. Simple list of all passes with target framebuffers
	class Workspace {
		public:
		Workspace(const CompositorGraph*);
		~Workspace();
		void destroy();
		bool compile(int w, int h);										// Compile compositor instance from graph - creates buffers
		bool compile(const Point& size) { return compile(size.x, size.y); }
		void execute(Scene*, Renderer*);								// Execute compositor
		void execute(const base::FrameBuffer* target, Scene*, Renderer*);	// Execute compositor
		void execute(const base::FrameBuffer* target, const Rect& view, Scene*, Renderer*);	// Execute compositor
		const CompositorGraph* getGraph() const { return m_graph; }
		bool isCompiled() const;
		int getWidth() const { return m_width; }
		int getHeight() const { return m_height; }

		void setCamera(base::Camera*);
		void setCamera(const char* name, base::Camera* cam);
		void copyCameras(const Workspace* from);
		base::Camera* getCamera(const char* name="default") const;

		const base::FrameBuffer* getBuffer(const char* compositor, const char* name);	// get framebuffer

		protected:
		size_t getCameraIndex(const char* name);
		static base::FrameBuffer* createBuffer(Compositor::Buffer*, int sw, int sh);

		protected:
		struct Expose { size_t id=0; base::FrameBuffer* buffer=0; const base::Texture* texture=0; };
		struct Pass {
			const CompositorPass* pass;		    // Pass data
			base::FrameBuffer*    target;		// Pass target
			size_t                camera;		// Camera index
			std::vector<Expose>   expose;
			std::vector<size_t>   conceal;
		};
		struct CameraRef { base::Camera* camera; char name[64]; };
		std::vector<base::FrameBuffer*> m_buffers;		// List of framebuffers
		std::vector<CameraRef>          m_cameras;		// List of cameras
		std::vector<Pass>               m_passes;		// List of passes to execute in order
		const CompositorGraph*          m_graph;		// Parent graph we are an instance of
		bool                            m_compiled;		// Compiled successfully
		int                             m_width=0, m_height=0;	// Size passed to compile function
	};

	// Create basic compositor instance
	CompositorGraph* getDefaultCompositor();
	Workspace* createDefaultWorkspace();

	// ===================================================================================================== //

	/// Interface for attaching compositor targets as material inputs
	class CompositorTextures {
		public:
		static const int DEPTH = -1;
		static CompositorTextures* getInstance();
		~CompositorTextures();
		public:
		const base::Texture* get(const char* name, int index=0);// Get texture to attach to a material. use index -1 for depth
		const base::Texture* get(size_t id, int index=0);		// Get texture to attach to a material. use index -1 for depth
		size_t getId(const char* name);							// Get compositor id from name
		void   expose(size_t cid, base::FrameBuffer* buffer);	// Expose a compositor
		void   expose(size_t cid, const base::Texture* texture);		// Expose a texture thrugh alias
		void   conceal(size_t cid);								// Conceal a compositor
		void   reset();											// clear active compositors
		protected:
		struct CTexture { base::Texture* texture; int index; CTexture* next; };
		std::vector<CTexture*> m_textures;
		NameLookup m_names;
	};

}

