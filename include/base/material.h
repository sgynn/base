#ifndef _SCENE_MATERIAL_
#define _SCENE_MATERIAL_

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
		bool operator==(const Blend&) const;
		void bind() const;

		// flags: src/dst, colour/alpha, invert, constant
		bool enabled;
		bool separate;
		int src, dst;
		int srcAlpha, dstAlpha;

		private:
		void setFromEnum(BlendMode, int& src, int& dst);
	};

	/** Material macro state */
	class MacroState {
		public:
		MacroState();
		bool operator==(const MacroState&) const;
		void bind() const;

		public:
		CullMode  cullMode;		// Polygon cull mode
		DepthTest depthTest;	// Depth test mode
		bool      depthWrite;	// Do we write to depth buffer
		char      colourMask;	// RGBA bitset for colour channels to write to
		bool      wireframe;	// wireframe polygon mode
	};

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
		void setMatrix(const char* name, float* m);							// set matrix (4x4)
		void setAuto(const char* name, int key);							// set automatic variable
		void unset(const char* name);										// unset a variable

		int*   getIntPointer(const char* name);					// get int pointer for direct memory access
		float* getFloatPointer(const char* name);				// get float pointer for direct memory access
		const int*   getIntPointer(const char* name) const;		// get int pointer for reading
		const float* getFloatPointer(const char* name) const;	// get float pointer for reading
		int    getElements(const char* name) const;				// get the number of values stored
		bool   contains(const char* name) const;				// is a variable defined
		int    getIndexCount() const;							// get the index count for mapping shader locations

		int getLocationData(const Shader*, int* map, int size, bool complain=true) const;
		int bindVariables(const Shader*, int* map, int size, AutoVariableSource* =0, bool autosOnly=false) const;

		protected:
		// ToDo: change this to use GL_UNIFORM_BUFFER ?
		struct SVar {
			char type;		// Variable base type
			char elements;	// Elements in variable (eg vec2 = 2)
			short array;	// Array size
			int index;		// Index for binding map
			union { float f; int i; float* fp; int* ip; };
		};
		void setType(SVar&, int, int, int);
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
		const base::Texture* getTexture(size_t index) { return m_textures[index]; }
		size_t               getTextureCount() const { return m_textures.size(); }

		const char* getName() const;
		void setName(const char*);

		protected:
		struct VariableData {
			const ShaderVars* vars;
			int*              map;
			int               size;
		};

		ShaderVars m_ownedVariables;					// variables owned by this pass
		std::vector<const base::Texture*> m_textures;	// Textures
		std::vector<VariableData> m_variableData;		// List of shared shader variables
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

		protected:
		std::vector<Pass*> m_passes;
	};

}

#endif

