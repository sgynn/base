#include <base/editor/renderdoc.h>
#include <base/gui/widgets.h>

#ifdef RENDERDOC

#include <dlfcn.h>
#include <assert.h>
#include "/opt/renderdoc_1.27/include/renderdoc_app.h"
#define APIPATH "/opt/renderdoc_1.27/"

void editor::RenderDoc::initialise() {
	static RENDERDOC_API_1_6_0 *api = NULL;

	#ifdef WIN32
	if(HMODULE mod = GetModuleHandleA("renderdoc.dll"))
	{
		pRENDERDOC_GetAPI RENDERDOC_GetAPI =
			(pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
		int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_6_0, (void **)&api);
		assert(ret == 1);
	}
	#endif

	#ifdef LINUX
	// requires -ldl linker flag
	const char* path = APIPATH "lib/" "librenderdoc.so";
	if(void *mod = dlopen(path, RTLD_NOW))
	{
		pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)dlsym(mod, "RENDERDOC_GetAPI");
		int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_6_0, (void **)&api);
		assert(ret == 1);
	}
	else printf("Failed to open %s\n%s\n", path, dlerror());
	#endif

	if(api) {
		printf("Renderdoc API Conenected\n");
		gui::Button* b = getEditor()->addButton("editors", "renderdoc");
		b->eventPressed.bind([this](gui::Button*) {
			//api->LaunchReplayUI(1, NULL);
			api->TriggerCapture();

			//api->StartFrameCapture(NULL, NULL);
			//(*m_workspace)->execute(m_scene, m_renderer);
			//api->EndFrameCapture(NULL, NULL);	
		});
	}
	else printf("Renderdoc not found\n");
}
#else
void editor::RenderDoc::initialise() {
	printf("Not compiled with RENDERDOC defined\n");
}

#endif

