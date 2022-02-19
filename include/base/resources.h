#pragma once

#include <base/resourcemanager.h>

namespace base { 
	class Model;
	class Texture;
	class Material;
	class Shader;
	class ShaderPart;
	class ShaderVars;
	class Compositor;
	class CompositorGraph;

	/** Singleton class global resource manager */
	class Resources {
		static Resources* s_instance;
		public:
		static Resources* getInstance() { return s_instance; }
		Resources();
		Resources(const Resources&) = delete;
		~Resources() { if(s_instance==this) s_instance=0; }

		ResourceManager<Model>           models;
		ResourceManager<Texture>         textures;
		ResourceManager<Material>        materials;
		ResourceManager<Shader>          shaders;
		ResourceManager<ShaderPart>      shaderParts;
		ResourceManager<ShaderVars>      shaderVars;	// Shared shader vars
		ResourceManager<Compositor>      compositors;
		ResourceManager<CompositorGraph> graphs;

		// Load xml file defining multiple resources ?
		bool loadFile(const char* file);

		// Add serach path to all managers
		void addPath(const char* path);
	};
}

