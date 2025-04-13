#pragma once

#include <base/resourcemanager.h>
#include <base/file.h>

namespace particle { class System; }
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
		float m_progress = 1.f;
		VirtualFileSystem* m_fileSystem;
		public:
		static Resources* getInstance() { return s_instance; }
		Resources();
		Resources(const Resources&) = delete;
		~Resources();

		ResourceManager<Model>            models;
		ResourceManager<Texture>          textures;
		ResourceManager<Material>         materials;
		ResourceManager<Shader>           shaders;
		ResourceManager<ShaderPart>       shaderParts;
		ResourceManager<ShaderVars>       shaderVars;	// Shared shader vars
		ResourceManager<Compositor>       compositors;
		ResourceManager<CompositorGraph>  graphs;
		ResourceManager<particle::System> particles;

		// Load xml file defining multiple resources. Use create=false to merely declare them
		bool loadFile(const char* file, bool create=true);

		// Add search paths to filesystem
		void addFolder(const char* path, bool recursive=true, const char* mount="");
		void addArchive(const char* archive, const char* mount="");
		const VirtualFileSystem& getFileSystem() const { return *m_fileSystem; }
		File openFile(const char* name) const;
		
		// Threaded or deferred loading. returns number of items finished this call
		int update();
		float getProgress() const { return m_progress; }
	};
}

