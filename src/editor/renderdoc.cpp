#include <base/editor/renderdoc.h>
#include <base/gui/widgets.h>
#include <base/window.h>
#include <base/game.h>

#if defined(LINUX) || defined(WIN32)

#ifdef LINUX
#include <base/window_x11.h>
#include <dlfcn.h>
#endif

#ifdef WIN32
#include <base/window_win32.h>
#endif

#include <assert.h>
#include "/opt/renderdoc_1.39/include/renderdoc_app.h"
#define APIPATH "/opt/renderdoc_1.39/"

static RENDERDOC_API_1_6_0 *api = NULL;
static int state = 0;

/*
struct AutoInit {
	AutoInit() {
		const char* path = APIPATH "lib/" "librenderdoc.so";
		if(void *mod = dlopen(path, RTLD_NOW))
		{
			pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)dlsym(mod, "RENDERDOC_GetAPI");
			int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_6_0, (void **)&api);
			assert(ret == 1);
			printf("Renderdoc Connected\n");
			//api->SetCaptureFilePathTemplate("./");
			api->SetFocusToggleKeys(NULL, 0);
			api->SetCaptureKeys(NULL, 0);
			api->SetCaptureOptionU32(eRENDERDOC_Option_CaptureCallstacks, true);
			api->SetCaptureOptionU32(eRENDERDOC_Option_CaptureAllCmdLists, true);
			api->SetCaptureOptionU32(eRENDERDOC_Option_APIValidation, true);
			api->SetCaptureOptionU32(eRENDERDOC_Option_DebugOutputMute, false);
			//api->MaskOverlayBits(eRENDERDOC_Overlay_None, eRENDERDOC_Overlay_None);
		}
		else printf("Failed to open %s\n%s\n", path, dlerror());
	}
	
} autoInit;
*/
void editor::RenderDoc::initialise() {
	//static RENDERDOC_API_1_6_0 *api = NULL;

	#ifdef WIN32
	if(HMODULE mod = GetModuleHandleA("renderdoc.dll"))
	{
		pRENDERDOC_GetAPI RENDERDOC_GetAPI =
			(pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
		int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_6_0, (void **)&api);
		assert(ret == 1);
		(void)ret;
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
		
		base::Window* w = base::Game::window();
		//base::X11Window* x = static_cast<base::X11Window*>(w);
		//::Window wnd = x->getXWindow();

		api->SetActiveWindow(w->getDevice(), w->getHandle());

		printf("Renderdoc API Conenected\n");
		gui::Button* b = getEditor()->addButton("editors", "renderdoc");
		b->eventPressed.bind([this, w](gui::Button*) {
//			api->TriggerCapture();

			if(!api->IsTargetControlConnected()) {
				uint pid = api->LaunchReplayUI(1, NULL); // arg should be "capture.log"
				if(pid==0) printf("Failed to launch RenderDoc\n");
			}

			//api->StartFrameCapture(NULL, NULL);
			//(*m_workspace)->execute(m_scene, m_renderer);
			//api->EndFrameCapture(NULL, NULL);	

			state = 1;
		});
	}
	else printf("Renderdoc not found\n");
}

void editor::RenderDoc::update() {
	//base::Window* w = base::Game::window();
	if(state == 1)  {
		printf("Start capture\n");
		//api->StartFrameCapture(w->getDevice(), w->getHandle());
		api->StartFrameCapture(NULL, NULL);
		state = 2;
	}
	else if(state == 2) {
		if(!api->IsFrameCapturing()) printf("Not capturing\n");
		printf("End capture\n");
		//api->EndFrameCapture(w->getDevice(), w->getHandle());
		int r = api->EndFrameCapture(NULL, NULL);
		printf("%d\n", r);
		state = 0;
	}
}

#endif

