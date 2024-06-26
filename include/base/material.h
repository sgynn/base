#pragma once

#include <base/hashmap.h>
#include <base/vec.h>
#include <vector>

namespace base {
	class Shader;
	class Texture;
	class Material;
	class AutoVariableSource;

	enum CullMode    { CULL_NONE, CULL_BACK, CULL_FRONT };
	enum BlendMode   { BLEND_NONE, BLEND_ALPHA, BLEND_ADD, BLEND_MULTIPLY };
	enum DepthTest   { DEPTH_ALWAYS, DEPTH_LESS, DEPTH_LEQUAL, DEPTH_GREATER, DEPTH_GEQUAL, DEPTH_EQUAL, DEPTH_DISABLED };
	enum CompileResult { COMPILE_FAILED, COMPILE_OK, COMPILE_WARN };
	enum ColourMask  { MASK_RED=1, MASK_GREEN=2, MASK_BLUE=4, MASK_ALPHA=8, MASK_ALL=0xf, MASK_NONE=0 };

	/** Scene blend mode */
	class Blend {
		public:
		Blend();
		Blend(BlendMode);
		Blend(BlendMode colour, BlendMode alpha);
		Blend(unsigned srcRGB, unsigned dstRGB, unsigned srcAlpha, unsigned dstAlpha);
		void set(int buffer, BlendMode);
		void set(int buffer, BlendMode colour, BlendMode alpha);
		void set(int buffer, unsigned srcRGB, unsigned dstRGB, unsigned srcAlpha, unsigned dstAlpha);
		bool operator==(const Blend&) const;
		bool operator!=(const Blend&) const;
		void bind() const;
		
		private:
		enum class State : char { Disabled, Single, Separate, Indexed, IndexedSeparate };
		struct Data { int buffer; unsigned short src, dst, srcAlpha, dstAlpha; };

		State m_state;
		std::vector<Data> m_data;

		private:
		static void setFromEnum(BlendMode, unsigned short& src, unsigned short& dst);
	};

	/** Material macro state */
	class MacroState {
		public:
		bool operator==(const MacroState&) const;
		void bind() const;

		public:
		CullMode  cullMode   = CULL_BACK;		// Polygon cull mode
		DepthTest depthTest  = DEPTH_LEQUAL;	// Depth test mode
		bool      depthWrite = true;			// Do we write to depth buffer
		char      colourMask = MASK_ALL;		// RGBA bitset for colour channels to write to
		bool      wireframe  = false;			// wireframe polygon mode
		float     depthOffset = 0;				// Polygon depth offset
	};

	/// Stencil buffer
	class StencilState {
		public:
		StencilState& operator==(const StencilState&) const;
		void bind();
		public:
		int mode; //?
		int value;
		int mask;
		int test;
	};

	enum ShaderVarType { VT_AUTO, VT_FLOAT, VT_INT, VT_SAMPLER, VT_MATRIX };

	/** Shader variables */
	class ShaderVars {
		public:
		ShaderVars();
		virtual ~ShaderVars();
		int  setFrom(const ShaderVars*);									// Copy variables from another set
		void set(const char* name, int i);									// set integer value
		void set(const char* name, float f);								// set float value
		void set(const char* name, float x, float y);						// set vec2 by element
		void set(const char* name, float x, float y, float z);				// set vec3 by element
		void set(const char* name, float x, float y, float z, float w);		// set vec4 by element
		void set(const char* name, const vec2& v);							// set vec2 value
		void set(const char* name, const vec3& v);							// set vec3 value
		void set(const char* name, const vec4& v);							// set vec3 value
		void set(const char* name, int elements, int array, const int* v);	// set int data from pointer
		void set(const char* name, int elements, int array, const float* v);// set float data from pointer
		void setSampler(const char* name, int index);						// Set texture (same as int)
		void setMatrix(const char* name, float* m);							// set matrix (4x4)
		void setAuto(const char* name, int key);							// set automatic variable
		void unset(const char* name);										// unset a variable

		int*   getIntPointer(const char* name);					// get int pointer for direct memory access
		int*   getSamplerPointer(const char* name);				// get int pointer for direct memory access
		float* getFloatPointer(const char* name);				// get float pointer for direct memory access
		const int*   getIntPointer(const char* name) const;		// get int pointer for reading
		const int*   getSamplerPointer(const char* name) const;	// get int pointer for direct memory access
		const float* getFloatPointer(const char* name) const;	// get float pointer for reading
		int    getElements(const char* name) const;				// get the number of values stored
		bool   contains(const char* name) const;				// is a variable defined
		int    getIndexCount() const;							// get the index count for mapping shader locations
		int    getAutoKey(const char* name) const;
		std::vector<const char*> getNames() const;

		int getLocationData(const Shader*, int* map, int size, bool complain=true) const;
		int bindVariables(const Shader*, int* map, int size, AutoVariableSource* =0, bool autosOnly=false) const;

		protected:
		// ToDo: change this to use GL_UNIFORM_BUFFER ?
		struct SVar {
			ShaderVarType type; // Data type
			char elements;	// Elements in variable (eg vec2 = 2)
			short array;	// Array size
			int index;		// Index for binding map
			union { float f; int i; float* fp; int* ip; };
		};
		void setType(SVar&, ShaderVarType, int e, int a);
		template<typename T> const T* getPointer(const SVar& var) const;
		template<typename T> const T* getPointer(const char* name, int typeMask) const;
		base::HashMap<SVar>  m_variables;	// shader variables
		int m_nextIndex;
	};

	/** Single render pass for a material */
	class Pass {
		friend class Material;
		Pass(Material*);
		Pass* clone(Material*) const;

		public:
		~Pass();

		void bind(AutoVariableSource* =0);			// Bind pass
		void bindTextures() const;
		void bindVariables(AutoVariableSource*, bool autoOnly=false) const;

		CompileResult compile();					// compile shader (if needed) and update uniform locations
		bool hasShader() const;						// Do we have a working shader?
		void setShader(Shader*);					// Set the shader
		void addShared(const ShaderVars* shared);	// Add shared variable list
		void removeShared(const ShaderVars* shared);// Remove shared variable list
		bool hasShared(const ShaderVars* shared) const;
		ShaderVars& getParameters() { return m_ownedVariables; }
		Shader* getShader() const { return m_shader; }
		
		Blend      blend;	// Blend state
		MacroState state;	// Macro state

		void                 setTexture(const char* name, const base::Texture*);
		void                 setTexture(size_t slot, const char* name, const base::Texture*);
		const base::Texture* getTexture(const char* name) const;
		const base::Texture* getTexture(size_t index) { return m_textures[index].texture; }
		size_t               getTextureCount() const { return m_textures.size(); }

		const char* getName() const;
		void setName(const char*);

		protected:
		size_t getTextureSlot(const char* name) const;

		protected:
		struct VariableData {
			const ShaderVars* vars;
			int*              map;
			int               size;
		};

		struct TextureSlot {
			const base::Texture* texture = nullptr;
			int uses = 0;
		};

		ShaderVars m_ownedVariables;				// variables owned by this pass
		std::vector<TextureSlot> m_textures;		// Textures
		std::vector<VariableData> m_variableData;	// List of shared shader variables
		Material* m_material;
		Shader*   m_shader;
		bool      m_changed;
		size_t    m_id;

		// Pass id lookup
		static base::HashMap<size_t> s_names;
		static std::vector<const char*> s_namesList;
	};


	/** Material container */
	class Material {
		public:
		Material();
		~Material();
		Material* clone() const;

		Pass* getPass(int index) const;
		Pass* getPass(const char* name) const;
		Pass* getPassByID(size_t id) const;

		Pass* addPass(const char* name=0);
		Pass* addPass(const Pass* pass, const char* name=0);
		void  deletePass(const Pass* pass);
		int   size() const;

		static size_t getPassID(const char*);

		std::vector<Pass*>::const_iterator begin() { return m_passes.begin(); }
		std::vector<Pass*>::const_iterator end() { return m_passes.end(); }


		// Will set a texture for all passes that have the variable
		bool setTexture(const char* name, Texture* texture);

		protected:
		std::vector<Pass*> m_passes;
	};

}

