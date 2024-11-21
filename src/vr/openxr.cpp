#ifndef EMSCRIPTEN

#include "openxr.h"
#include <base/game.h>
#include <base/camera.h>
#include <base/input.h>
#include <base/renderer.h>
#include <base/compositor.h>
#include <base/framebuffer.h>
#include <base/opengl.h>
#include <cstdio>
#include <string>
#include <vector>

#include <base/scenecomponent.h>

#ifdef LINUX
#define XR_USE_PLATFORM_XLIB
#include <base/window_x11.h>
#include <GL/glx.h>
#endif

#ifdef WIN32
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11
#include <base/window_win32.h>
#include <unknwn.h> // openxr_platform.h depends on this
#include <d3d11.h>
#endif

#define XR_USE_GRAPHICS_API_OPENGL
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include "directx-interop.h"

PFN_xrCreateHandTrackerEXT xrCreateHandTrackerEXT = nullptr;
PFN_xrDestroyHandTrackerEXT xrDestroyHandTrackerEXT = nullptr;
PFN_xrLocateHandJointsEXT xrLocateHandJointsEXT = nullptr;

using namespace base;

namespace xr {
	static const XrPosef  identity     = { {0,0,0,1}, {0,0,0} };
	XrInstance     instance     = 0;
	XrSession      session      = 0;
	XrSessionState sessionState = XR_SESSION_STATE_UNKNOWN;
	XrSpace        appSpace     = 0;
	XrSystemId     systemId     = XR_NULL_SYSTEM_ID;
	XrEnvironmentBlendMode   blend;
	XrDebugUtilsMessengerEXT debug = 0;
	DirectXWGLInterop* interop = nullptr;
	FlipTarget* flipTarget = nullptr;

	std::vector<XrView>                  views;
	std::vector<XrViewConfigurationView> configViews;
	std::vector<Swapchain>               swapchains;

	XrSpace				headSpace;
	HandController      hand[2];
	HandController      tracker[3];
	XrActionSet         actionSet;
	struct Actions {
		XrAction aimPose;
		XrAction gripPose;
		XrAction trigger;
		XrAction grip;
		XrAction stickX;
		XrAction stickY;
		XrAction stickPress;
		XrAction padX;
		XrAction padY;
		XrAction padTouch;
		XrAction padPress;
		XrAction system;
		XrAction a;
		XrAction b;
		XrAction haptic;
	} actions;

	bool initialise(const char* appName);
	void setupActions();
	void setupHandTrackers();
	void destroy();

	inline void convertPose(vr::Transform& out, const XrPosef& pose) {
		out.position.set(pose.position.x, pose.position.y, pose.position.z);
		out.orientation.set(pose.orientation.w, pose.orientation.x, pose.orientation.y, pose.orientation.z);
	}

	inline int64_t selectFormat(const std::vector<int64_t>& availiable, std::initializer_list<int64_t> valid, bool interop) {
		for(int64_t f: valid) {
			if(interop) f = GLFormatToDXGIFormat(f);
			for(int64_t v: availiable) if(f==v) return f;
		}
		return 0;
	}
};

class XRController : public base::Joystick {
	public:
	XRController(vr::Side hand) : Joystick(6, 6), side(hand) {
		sprintf(m_name, "XR Controller %s", side? "Right": "Left");
		for(uint i=0; i<m_numAxes; ++i) { m_range[i].min=-100000; m_range[i].max=100000; }
	}
	bool update() override;
	void vibrate(uint duration, float amplitude, float frequency) override;
	vr::Side side;
};


bool xr::initialise(const char* appName) {
	// Check required extensions
	const char *requiredExtensions[] = { 
			XR_KHR_OPENGL_ENABLE_EXTENSION_NAME, // Use OpenGL for rendering
			XR_EXT_DEBUG_UTILS_EXTENSION_NAME,  // Debug utils for extra info
			XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME, // Depth buffer submission for reprojection
			XR_EXT_HAND_TRACKING_EXTENSION_NAME, // Hand skeleton
	};
	#ifdef WIN32
	interop = DirectXWGLInterop::create();
	if(interop) requiredExtensions[0] = XR_KHR_D3D11_ENABLE_EXTENSION_NAME; // D3D interop
	#endif


	uint count = 0;
	xrEnumerateInstanceExtensionProperties(nullptr, 0, &count, nullptr);
	std::vector<XrExtensionProperties> extensions(count, { XR_TYPE_EXTENSION_PROPERTIES });
	xrEnumerateInstanceExtensionProperties(nullptr, count, &count, extensions.data());


	uint found = 0;
	constexpr int len = sizeof(requiredExtensions) / sizeof(char*);
	printf("OpenXR extensions available:\n");
	for(auto& ext: extensions) {
		printf(" %s\n", ext.extensionName);
		for(uint i=0; i<len; ++i) {
			if(strcmp(requiredExtensions[i], ext.extensionName)==0) {
				found |= 1<<i;
				break;
			}
		}
	}
	// Are we missing any
	for(uint i=0; i<len; ++i) {
		if(~found & 1<<i) {
			printf("Error: Missing extension %s\n", requiredExtensions[i]);
			return false;
		}
	}

	// --------------------------
	int patchVersion = XR_VERSION_PATCH(XR_CURRENT_API_VERSION);
	if(FILE* fp = fopen("xrversion.txt", "r")) {
		fscanf(fp, "%d", &patchVersion);
		fclose(fp);
	}
	printf("OpenXR API Version: %d.%d.%d\n", XR_VERSION_MAJOR(XR_CURRENT_API_VERSION), XR_VERSION_MINOR(XR_CURRENT_API_VERSION), patchVersion);

	//int patchVersion = std::min(34u, XR_VERSION_PATCH(XR_CURRENT_API_VERSION));
	
	// Initialize OpenXR with the extensions we've found!
	XrInstanceCreateInfo createInfo = { XR_TYPE_INSTANCE_CREATE_INFO };
	createInfo.enabledExtensionCount      = len;
	createInfo.enabledExtensionNames      = requiredExtensions;
	createInfo.applicationInfo.apiVersion = XR_MAKE_VERSION(1, 0, patchVersion); //XR_CURRENT_API_VERSION;
	createInfo.applicationInfo.engineVersion = 1;
	createInfo.applicationInfo.applicationVersion = 1;
	strcpy(createInfo.applicationInfo.applicationName, appName? appName: "OpenXR Demo");
	strcpy(createInfo.applicationInfo.engineName, "baselib");
	XrResult r = xrCreateInstance(&createInfo, &xr::instance);

	if(r != XR_SUCCESS) {
		switch(r) {
		case XR_ERROR_VALIDATION_FAILURE:      printf("Error: OpenXR validation failure\n"); break;
		case XR_ERROR_RUNTIME_FAILURE:         printf("Error: OpenXR runtime failure\n"); break;
		case XR_ERROR_OUT_OF_MEMORY:           printf("Error: OpenXR out of memory\n"); break;
		case XR_ERROR_LIMIT_REACHED:           printf("Error: OpenXR instance limit reached\n"); break;
		case XR_ERROR_RUNTIME_UNAVAILABLE:     printf("Error: OpenXR uuntime unavailiable\n"); break;
		case XR_ERROR_NAME_INVALID:	           printf("Error: OpenXR name invalid\n"); break;
		case XR_ERROR_INITIALIZATION_FAILED:   printf("Error: OpenXR initialisation failed\n"); break;
		case XR_ERROR_EXTENSION_NOT_PRESENT:   printf("Error: OpenXR extension not present\n"); break;
		case XR_ERROR_API_VERSION_UNSUPPORTED: printf("Error: OpenXR version unsupported\n"); break;
		case XR_ERROR_API_LAYER_NOT_PRESENT:   printf("Error: OpenXR API layer not present\n"); break;
		default: printf("Error: OpenXR failed to create instance %#x\n", r); break;
		}
		return false;
	}
	
	// Request a form factor from the device (HMD, Handheld, etc.)
	XrSystemGetInfo systemInfo = { XR_TYPE_SYSTEM_GET_INFO };
	systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
	xrGetSystem(xr::instance, &systemInfo, &xr::systemId);

	XrSystemProperties systemProperties = { XR_TYPE_SYSTEM_PROPERTIES };
	XrSystemHandTrackingPropertiesEXT handProperties = { XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT };
	systemProperties.next = &handProperties;
	xrGetSystemProperties(xr::instance, xr::systemId, &systemProperties);
	if(handProperties.supportsHandTracking) {
		xrGetInstanceProcAddr(xr::instance, "xrCreateHandTrackerEXT",  (PFN_xrVoidFunction*)&xrCreateHandTrackerEXT);
		xrGetInstanceProcAddr(xr::instance, "xrDestroyHandTrackerEXT", (PFN_xrVoidFunction*)&xrDestroyHandTrackerEXT);
		xrGetInstanceProcAddr(xr::instance, "xrLocateHandJointsEXT",   (PFN_xrVoidFunction*)&xrLocateHandJointsEXT);
	}


	// Blend modes - we want opaque, but openXR supports AR stuff... just grab the first one.
	XrViewConfigurationType	viewConfig = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
	xrEnumerateEnvironmentBlendModes(xr::instance, xr::systemId, viewConfig, 1, &count, &xr::blend);


	if(!interop) {
		PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = nullptr;
		xrGetInstanceProcAddr(xr::instance, "xrGetOpenGLGraphicsRequirementsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetOpenGLGraphicsRequirementsKHR));
		XrGraphicsRequirementsOpenGLKHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR};
		pfnGetOpenGLGraphicsRequirementsKHR(xr::instance, xr::systemId, &graphicsRequirements);
	}
	#ifdef WIN32
	else {
		PFN_xrGetD3D11GraphicsRequirementsKHR pfnGetD3D11GraphicsRequirementsKHR = nullptr;
		xrGetInstanceProcAddr(xr::instance, "xrGetD3D11GraphicsRequirementsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetD3D11GraphicsRequirementsKHR));
		XrGraphicsRequirementsD3D11KHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR};
		pfnGetD3D11GraphicsRequirementsKHR(xr::instance, xr::systemId, &graphicsRequirements);
	}
	#endif


	// A session represents this application's desire to display things! This is where we hook up our graphics API.
	// This does not start the session, for that, you'll need a call to xrBeginSession, which we do in openxr_poll_events
	void* binding = nullptr;
	#ifdef LINUX
	X11Window* window = static_cast<X11Window*>(Game::window());
	XrGraphicsBindingOpenGLXlibKHR glBinding = { XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR };
	glBinding.xDisplay = window->getXDisplay();
	glBinding.glxFBConfig = window->getFBConfig();
	glBinding.glxDrawable = glXGetCurrentDrawable();
	glBinding.glxContext = glXGetCurrentContext();
	int iValue = 0;
	glXGetFBConfigAttrib(glBinding.xDisplay, glBinding.glxFBConfig, GLX_VISUAL_ID, &iValue);
	glBinding.visualid = iValue;
	binding = &glBinding;
	#elif defined(WIN32)
	Win32Window* window = static_cast<Win32Window*>(Game::window());
	XrGraphicsBindingOpenGLWin32KHR glBinding = { XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR };
	glBinding.hDC = window->getHDC();
	glBinding.hGLRC = window->getHGLRC();
	
	XrGraphicsBindingD3D11KHR dxBinding = { XR_TYPE_GRAPHICS_BINDING_D3D11_KHR };
	dxBinding.device = interop? interop->getD3DDeviceHandle(): nullptr;
	if(interop) binding = &dxBinding;
	else binding = &glBinding;
	#endif

	XrSessionCreateInfo sessionInfo = { XR_TYPE_SESSION_CREATE_INFO };
	sessionInfo.next     = binding;
	sessionInfo.systemId = xr::systemId;
	r = xrCreateSession(xr::instance, &sessionInfo, &xr::session);
	if(!xr::session) {
		xrDestroyInstance(xr::instance);
		xr::instance = 0;
		printf("Error: OpenXR failed to create session %d\n", r);
		return false;
	}


	// How to manage positioning
	XrReferenceSpaceCreateInfo refSpace = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
	refSpace.poseInReferenceSpace = xr::identity;
	refSpace.referenceSpaceType   = XR_REFERENCE_SPACE_TYPE_STAGE; // LOCAL is the other option
	xrCreateReferenceSpace(xr::session, &refSpace, &xr::appSpace);

	// Space for tracking haedset location
	XrReferenceSpaceCreateInfo viewSpace = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO, NULL, XR_REFERENCE_SPACE_TYPE_VIEW, {{0,0,0,1}, {0,0,0}} };
	xrCreateReferenceSpace(xr::session, &viewSpace, &xr::headSpace);

	// Create swapchains for vr displays
	xrEnumerateViewConfigurationViews(xr::instance, xr::systemId, viewConfig, 0, &count, nullptr);
	xr::configViews.resize(count, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
	xrEnumerateViewConfigurationViews(xr::instance, xr::systemId, viewConfig, count, &count, xr::configViews.data());
	xr::views.resize(count, {XR_TYPE_VIEW});

	xrEnumerateSwapchainFormats(xr::session, 0, &count, nullptr);
	std::vector<int64_t> formats(count);
	xrEnumerateSwapchainFormats(xr::session, formats.size(), &count, formats.data());
	int64_t colourFormat = selectFormat(formats, { GL_RGB10_A2, GL_RGBA16F, GL_RGB16F, GL_RGBA16, GL_RGBA8, GL_SRGB8_ALPHA8_EXT }, interop);
	int64_t depthFormat = selectFormat(formats, { GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT16 }, interop);

	if(!colourFormat || !depthFormat) {
		printf("Error: No compatible formats found\n");
		printf("Possible formats: ");
		for(int64_t f: formats) printf(" %lx ", (unsigned long)f);
		printf("\n");
		xrDestroyInstance(xr::instance);
		xr::instance = 0;
		return false;
	}

	printf("OpenXR Colour Format: %#x Depth Format: %#x\n", (int)colourFormat, (int)depthFormat);

	xr::swapchains.reserve(xr::configViews.size());
	for(XrViewConfigurationView& view : xr::configViews) {
		Swapchain swapchain = { view.recommendedImageRectWidth, view.recommendedImageRectHeight };

		XrSwapchainCreateInfo    swapchainInfo = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
		swapchainInfo.createFlags = 0;
		swapchainInfo.arraySize   = 1;
		swapchainInfo.mipCount    = 1;
		swapchainInfo.faceCount   = 1;
		swapchainInfo.format      = colourFormat;
		swapchainInfo.width       = view.recommendedImageRectWidth;
		swapchainInfo.height      = view.recommendedImageRectHeight;
		swapchainInfo.sampleCount = view.recommendedSwapchainSampleCount;
		swapchainInfo.usageFlags  = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
		xrCreateSwapchain(xr::session, &swapchainInfo, &swapchain.colourHandle);

		swapchainInfo.format      = depthFormat;
		swapchainInfo.usageFlags  = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		xrCreateSwapchain(xr::session, &swapchainInfo, &swapchain.depthHandle);

		auto setupSwapChainImages = [&](XrSwapchain handle, std::vector<SwapChainImage*>& images, uint64_t format) {
			xrEnumerateSwapchainImages(swapchain.colourHandle, 0, &count, nullptr);
			if(interop) {
				#ifdef WIN32
				std::vector<XrSwapchainImageD3D11KHR> list(count, {XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR});
				xrEnumerateSwapchainImages(handle, count, &count, (XrSwapchainImageBaseHeader*)list.data());
				for(auto& i: list) images.push_back(interop->createSwapChainImage(i.texture, format));
				#endif
			}
			else {
				std::vector<XrSwapchainImageOpenGLKHR> list(count, {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR});
				xrEnumerateSwapchainImages(handle, count, &count, (XrSwapchainImageBaseHeader*)list.data());
				for(auto& i: list) images.push_back(new SwapChainImage(i.image));
			}
		};

		setupSwapChainImages(swapchain.colourHandle, swapchain.images, colourFormat);
		setupSwapChainImages(swapchain.depthHandle, swapchain.depth, depthFormat);

		printf("Swap chain created: Resolution: %dx%d Samples: %d Count: %u\n", swapchainInfo.width, swapchainInfo.height, swapchainInfo.sampleCount, count);
		xr::swapchains.push_back(std::move(swapchain));
	}

	// Nasty render target flip action as directX is inverted
	if(interop) {
		flipTarget = new FlipTarget(xr::configViews[0].recommendedImageRectWidth, xr::configViews[0].recommendedImageRectHeight, colourFormat, depthFormat);
	}

	return true;
}

void xr::setupActions() {
	constexpr int Left = vr::LEFT;
	constexpr int Right = vr::RIGHT;

	XrActionSetCreateInfo actionSetInfo{XR_TYPE_ACTION_SET_CREATE_INFO};
	strcpy(actionSetInfo.actionSetName, "gameplay");
	strcpy(actionSetInfo.localizedActionSetName, "Gameplay");
	actionSetInfo.priority = 0;
	xrCreateActionSet(xr::instance, &actionSetInfo, &xr::actionSet);

	static auto path = [](const char* name) {
		XrPath path;
		xrStringToPath(xr::instance, name, &path);
		return path;
	};

	auto createAction = [](XrActionType type, const char* name, const char* text, bool perHand = true) {
		XrAction result = 0;
		XrActionCreateInfo actionInfo{XR_TYPE_ACTION_CREATE_INFO};
		XrPath handPaths[2] = { xr::hand[Left].subaction, xr::hand[Right].subaction};
		actionInfo.actionType = type;
		strcpy(actionInfo.actionName, name);
		strcpy(actionInfo.localizedActionName, text);
		actionInfo.countSubactionPaths = perHand? 2: 0;
		actionInfo.subactionPaths = handPaths;
		xrCreateAction(xr::actionSet, &actionInfo, &result);
		return result;
	};
	
	// Get the XrPath for the left and right hands - we will use them as subaction paths.
	xr::hand[Left].role = vr::HAND_LEFT; 
	xr::hand[Right].role = vr::HAND_RIGHT; 
	xr::hand[Left].subaction = path("/user/hand/left");
	xr::hand[Right].subaction = path("/user/hand/right");

	xr::actions.aimPose = createAction(XR_ACTION_TYPE_POSE_INPUT, "aim_pose", "Aim Pose");
	xr::actions.gripPose = createAction(XR_ACTION_TYPE_POSE_INPUT, "grip_pose", "Grip Pose");
	xr::actions.trigger = createAction(XR_ACTION_TYPE_FLOAT_INPUT, "trigger_value", "Trigger Value");
	xr::actions.grip = createAction(XR_ACTION_TYPE_FLOAT_INPUT, "grip_value", "Grip Value");
	xr::actions.stickX = createAction(XR_ACTION_TYPE_FLOAT_INPUT, "stick_x", "Thumbstick X");
	xr::actions.stickY = createAction(XR_ACTION_TYPE_FLOAT_INPUT, "stick_y", "Thumbstick Y");
	xr::actions.stickPress = createAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "stick_press", "Thumbstick Press");
	xr::actions.padX = createAction(XR_ACTION_TYPE_FLOAT_INPUT, "pad_x", "Trackpad X");
	xr::actions.padY = createAction(XR_ACTION_TYPE_FLOAT_INPUT, "pad_y", "Trackpad Y");
	xr::actions.padTouch = createAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "pad_touch", "Trackpad Touch");
	xr::actions.padPress = createAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "pad_press", "Trackpad Press");
	xr::actions.system = createAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "system", "System");
	xr::actions.a = createAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "a", "A");
	xr::actions.b = createAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "b", "B");
	xr::actions.haptic = createAction(XR_ACTION_TYPE_VIBRATION_OUTPUT, "haptics", "Buzz");


	// action spaces for pose actions
	XrActionSpaceCreateInfo actionSpaceInfo{XR_TYPE_ACTION_SPACE_CREATE_INFO};
	actionSpaceInfo.poseInActionSpace.orientation.w = 1.f;
	actionSpaceInfo.action = xr::actions.gripPose;
	actionSpaceInfo.subactionPath = xr::hand[Left].subaction;
	xrCreateActionSpace(xr::session, &actionSpaceInfo, &xr::hand[Left].space);
	actionSpaceInfo.subactionPath = xr::hand[Right].subaction;
	xrCreateActionSpace(xr::session, &actionSpaceInfo, &xr::hand[Right].space);


	// Additional trackers using XR_HTCX_vive_tracker_interaction extension
	// See https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#XR_HTCX_vive_tracker_interaction
	// TODO

	

	struct Binding { XrAction action; const char* pathSuffix; };
	auto suggestBindings = [](const char* target, std::vector<Binding> bindings) {
		std::vector<XrActionSuggestedBinding> bindingData;
		bindingData.reserve(bindings.size() * 2);
		char pathName[128];
		for(Binding& b: bindings) {
			sprintf(pathName, "/user/hand/left/%s", b.pathSuffix);
			bindingData.push_back({b.action, path(pathName)});
			sprintf(pathName, "/user/hand/right/%s", b.pathSuffix);
			bindingData.push_back({b.action, path(pathName)});
		}
		XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
		suggestedBindings.interactionProfile = path(target);
		suggestedBindings.suggestedBindings = bindingData.data();
		suggestedBindings.countSuggestedBindings = bindingData.size();
		if(xrSuggestInteractionProfileBindings(xr::instance, &suggestedBindings) != XR_SUCCESS) {
			printf("Failed profile for %s\n", strrchr(target, '/'));
		}
	};

	
	suggestBindings("/interaction_profiles/khr/simple_controller",
	{
		{ xr::actions.aimPose, "input/aim/pose" },
		{ xr::actions.gripPose, "input/grip/pose" },
		{ xr::actions.trigger, "input/select/click" },
		{ xr::actions.system, "input/menu/click" },
		{ xr::actions.haptic, "output/haptic" },
	});
	
	suggestBindings("/interaction_profiles/valve/index_controller",
	{
		{ xr::actions.aimPose, "input/aim/pose" },
		{ xr::actions.gripPose, "input/grip/pose" },
		{ xr::actions.trigger, "input/trigger/value" },
		{ xr::actions.grip, "input/squeeze/value" },
		{ xr::actions.stickX, "input/thumbstick/x" },
		{ xr::actions.stickY, "input/thumbstick/y" },
		{ xr::actions.stickPress, "input/thumbstick/click" },
		{ xr::actions.padX, "input/trackpad/x" },
		{ xr::actions.padY, "input/trackpad/y" },
		{ xr::actions.padTouch, "input/trackpad/touch" },
		{ xr::actions.padPress, "input/trackpad/force" },
		{ xr::actions.system, "input/system/click" },
		{ xr::actions.b, "input/b/click" },
		{ xr::actions.a, "input/a/click" },
		{ xr::actions.haptic, "output/haptic" },
	});
	suggestBindings("/interaction_profiles/oculus/touch_controller",
	{
		{ xr::actions.aimPose, "input/aim/pose" },
		{ xr::actions.gripPose, "input/grip/pose" },
		{ xr::actions.trigger, "input/trigger/value" },
		{ xr::actions.grip, "input/squeeze/value" },
		{ xr::actions.stickX, "input/thumbstick/x" },
		{ xr::actions.stickY, "input/thumbstick/y" },
		{ xr::actions.stickPress, "input/thumbstick/click" },
		{ xr::actions.system, "input/system/click" },
		{ xr::actions.b, "input/b/click" },
		{ xr::actions.a, "input/a/click" },
		{ xr::actions.haptic, "output/haptic" },
	});
	suggestBindings("/interaction_profiles/htc/vive_controller",
	{
		{ xr::actions.aimPose, "input/aim/pose" },
		{ xr::actions.gripPose, "input/grip/pose" },
		{ xr::actions.trigger, "input/trigger/value" },
		{ xr::actions.grip, "input/squeeze/value" },
		{ xr::actions.padX, "input/trackpad/x" },
		{ xr::actions.padY, "input/trackpad/y" },
		{ xr::actions.padTouch, "input/trackpad/touch" },
		{ xr::actions.padPress, "input/trackpad/click" },
		{ xr::actions.system, "input/menu/click" },
		{ xr::actions.haptic, "output/haptic" },
	});
	suggestBindings("/interaction_profiles/microsoft/motion_controller"	,
	{
		{ xr::actions.aimPose, "input/aim/pose" },
		{ xr::actions.gripPose, "input/grip/pose" },
		{ xr::actions.trigger, "input/trigger/value" },
		{ xr::actions.grip, "input/squeeze/value" },
		{ xr::actions.stickX, "input/thumbstick/x" },
		{ xr::actions.stickY, "input/thumbstick/y" },
		{ xr::actions.stickPress, "input/thumbstick/click" },
		{ xr::actions.padX, "input/trackpad/x" },
		{ xr::actions.padY, "input/trackpad/y" },
		{ xr::actions.padTouch, "input/trackpad/touch" },
		{ xr::actions.padPress, "input/trackpad/click" },
		{ xr::actions.system, "input/menu/click" },
		{ xr::actions.haptic, "output/haptic" },
	});


	// Attach action set to session
	XrSessionActionSetsAttachInfo attachInfo{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
	attachInfo.countActionSets = 1;
	attachInfo.actionSets = &xr::actionSet;
	xrAttachSessionActionSets(xr::session, &attachInfo);
}

void xr::setupHandTrackers() {
	if(!xrCreateHandTrackerEXT) return;
	for(int i=0; i<2; ++i) {
		XrHandTrackerCreateInfoEXT createInfo = { XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT };
		createInfo.hand = i==0? XR_HAND_LEFT_EXT: XR_HAND_RIGHT_EXT;
		createInfo.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;
		xrCreateHandTrackerEXT(session, &createInfo, &hand[i].handTracker);
	}
}

void xr::destroy() {
	if(xr::instance) {
		for(auto& swap: xr::swapchains) {
			for(auto img: swap.images) delete img;
			for(auto img: swap.depth) delete img;
			xrDestroySwapchain(swap.colourHandle);
			xrDestroySwapchain(swap.depthHandle);
		}
		for(HandController& h: xr::hand) {
			xrDestroySpace(h.space);
			if(xrDestroyHandTrackerEXT && h.handTracker) xrDestroyHandTrackerEXT(h.handTracker);
		}

		xrDestroyActionSet(xr::actionSet);
		xrDestroySpace(xr::headSpace);
		xrDestroySpace(xr::appSpace);
		xrDestroySession(xr::session);
		xrDestroyInstance(xr::instance);
		xr::instance = 0;
	}
}

// ================================================================ //


VRComponent* VRComponent::create(const char* appName, base::Scene* scene, const base::CompositorGraph* graph) {
	VRComponent* component = new VRComponent(appName, scene, graph);
	if(component->isEnabled()) return component;
	delete component;
	return nullptr;
}

VRComponent::VRComponent(const char* appName, base::Scene* scene, const base::CompositorGraph* graph)
	: GameStateComponent(-10, 10, PERSISTENT)
	, m_scene(scene)
{

	// -------------- Initialise OpenXR ------------ //
	if(!xr::initialise(appName)) {
		return;
	}

	xr::setupActions();
	xr::setupHandTrackers();
	m_camera = new Camera();
	m_camera->adjustDepth(0.1, 100);
	m_target = new FrameBuffer(16,16, Texture::NONE);

	if(xr::actions.trigger != XR_NULL_HANDLE) {
		m_controllerIndex[0] = Game::input()->addJoystick(new XRController(vr::RIGHT));
		m_controllerIndex[1] = Game::input()->addJoystick(new XRController(vr::LEFT));
	}

	// Assume these are the same for all views
	int width = xr::configViews[0].recommendedImageRectWidth;
	int height = xr::configViews[0].recommendedImageRectHeight;

	// Set up compositor - need to work with two targets somehow
	if(!graph) m_workspace = createDefaultWorkspace();
	else m_workspace = new Workspace(graph);
	m_workspace->compile(width, height);
	m_renderer = new Renderer();
}

VRComponent::~VRComponent() {
	fflush(stdout);
	delete m_camera;
	delete m_target;
	delete m_workspace;
	delete m_renderer;
	xr::destroy();
}

bool VRComponent::isEnabled() const {
	return xr::instance;
}

bool VRComponent::isActive() const {
	return xr::sessionState == XR_SESSION_STATE_FOCUSED;
}

BoundingBox2D VRComponent::getPlayArea() const {
	XrExtent2Df bounds;
	xrGetReferenceSpaceBoundsRect(xr::session, XR_REFERENCE_SPACE_TYPE_STAGE, &bounds);
	return BoundingBox2D(-bounds.width/2, -bounds.height/2, bounds.width, bounds.height);
}

void VRComponent::adjustDepth(float nearClip, float farClip) {
	m_camera->adjustDepth(nearClip, farClip);
}

void VRComponent::setScene(Scene* s) {
	m_scene = s;
}

bool VRComponent::setCompositor(const CompositorGraph* graph) {
	if(graph) {
		Workspace* workspace = new Workspace(graph);
		if(workspace->compile(FrameBuffer::Screen.width(), FrameBuffer::Screen.height())) {
			delete m_workspace;
			m_workspace = workspace;
			return true;
		}
		else printf("Error: Failed to compile compositor graph");
	}
	else {
		delete m_workspace;
		m_workspace = nullptr;
	}
	if(!m_workspace) {
		m_workspace = createDefaultWorkspace();
	}
	return !graph;
}

const Joystick& VRComponent::getController(vr::Side c) const {
	switch(c) {
	case vr::RIGHT: return Game::input()->joystick(m_controllerIndex[0]);
	case vr::LEFT: return Game::input()->joystick(m_controllerIndex[1]);
	default: return Game::input()->joystick(-1);
	}
}

bool VRComponent::hasHandJoints(vr::Side hand) const {
	return xr::hand[hand].handTracker;
}

vr::Transform VRComponent::getHandJoint(vr::Side hand, int index) const {
	vr::Transform result;
	xr::convertPose(result, xr::hand[hand].jointLocations[index].pose);
	// 180 degree rotation in x axis to get z+ as forward to match bones
	result.orientation.set(-result.orientation.x, result.orientation.w, result.orientation.z, -result.orientation.y);
	return result;
}

bool VRComponent::hasTrackedObject(vr::Role o) const {
	if(!isActive()) return false;
	switch(o) {
	case vr::HEAD: return true;
	case vr::HAND_LEFT: return xr::hand[vr::LEFT].active;
	case vr::HAND_RIGHT: return xr::hand[vr::RIGHT].active;
	case vr::WAIST: return xr::tracker[0].active;
	case vr::FOOT_RIGHT: return xr::tracker[1].active;
	case vr::FOOT_LEFT: return xr::tracker[2].active;
	}
	return false;
}


void VRComponent::update() {
	if(!xr::instance) return;

	XrEventDataBuffer event = { XR_TYPE_EVENT_DATA_BUFFER };
	while(xrPollEvent(xr::instance, &event) == XR_SUCCESS) {
		switch (event.type) {
		case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
			xr::sessionState = ((XrEventDataSessionStateChanged*)&event)->state;
			switch (xr::sessionState) {
			case XR_SESSION_STATE_READY:
				{
				printf("XR_SESSION_STATE_READY\n");
				XrSessionBeginInfo beginInfo = { XR_TYPE_SESSION_BEGIN_INFO };
				beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
				xrBeginSession(xr::session, &beginInfo);
				}
				break;
			case XR_SESSION_STATE_STOPPING:
				printf("XR_SESSION_STATE_STOPPING\n");
				xrEndSession(xr::session); 
				break;
			case XR_SESSION_STATE_EXITING:
			case XR_SESSION_STATE_LOSS_PENDING:
				printf("XR_SESSION_STATE_EXITING\n");
				getState()->changeState(0);
				break;
			default:
				break;
			}
			break;
		case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
			getState()->changeState(0);
			return;
		case XR_TYPE_EVENT_DATA_EVENTS_LOST:
			printf("%u events lost\n", ((XrEventDataEventsLost*)&event)->lostEventCount);
			break;
		case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
			printf("Interaction profile changed\n");
			break;
		default:
			break;
		}
		event = { XR_TYPE_EVENT_DATA_BUFFER };
	}

	if(xr::sessionState != XR_SESSION_STATE_FOCUSED) return;

	// Actions
	const XrActiveActionSet activeActionSet {xr::actionSet, XR_NULL_PATH};
	XrActionsSyncInfo syncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
	syncInfo.countActiveActionSets = 1;
	syncInfo.activeActionSets = &activeActionSet;
	xrSyncActions(xr::session, &syncInfo);

	// Debug
//	XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO, nullptr, xr::actions.a, 0};
//	XrActionStateBoolean grabValue{XR_TYPE_ACTION_STATE_BOOLEAN};
//	xrGetActionStateBoolean(xr::session, &getInfo, &grabValue);
//	printf("%d, %d\n", grabValue.isActive, grabValue.currentState);
}

bool XRController::update() {
	XrActionStateGetInfo info{XR_TYPE_ACTION_STATE_GET_INFO, nullptr, xr::actions.a, xr::hand[side].subaction};
	XrActionStateFloat floatValue{XR_TYPE_ACTION_STATE_FLOAT};
	XrActionStateBoolean boolValue{XR_TYPE_ACTION_STATE_BOOLEAN};
	
	auto getActionf = [&](XrAction action) {
		info.action = action;
		xrGetActionStateFloat(xr::session, &info, &floatValue);
		return floatValue.currentState;
	};
	auto getAction = [&](XrAction action) {
		info.action = action;
		xrGetActionStateBoolean(xr::session, &info, &boolValue);
		return boolValue.currentState;
	};

	const float scale = m_range[0].max;
	m_axis[0] = getActionf(xr::actions.stickX) * scale;
	m_axis[1] = getActionf(xr::actions.stickY) * scale;
	m_axis[2] = getActionf(xr::actions.trigger) * scale;
	m_axis[3] = getActionf(xr::actions.grip) * scale;
	m_axis[4] = getActionf(xr::actions.padX) * scale;
	m_axis[5] = getActionf(xr::actions.padY) * scale;

	uint buttons = 0;
	if(getAction(xr::actions.a)) buttons |= 1;
	if(getAction(xr::actions.b)) buttons |= 2;
	if(getAction(xr::actions.stickPress)) buttons |= 4;
	if(getAction(xr::actions.padTouch)) buttons |= 8;
	if(getAction(xr::actions.padPress)) buttons |= 16;
	if(getAction(xr::actions.system)) buttons |= 32;
	m_changed = m_buttons ^ buttons;
	m_buttons = buttons;
	return true;
}
void XRController::vibrate(uint duration, float amplitude, float frequency) {
	XrHapticVibration vibration{XR_TYPE_HAPTIC_VIBRATION};
	vibration.amplitude = amplitude;
	vibration.duration = duration<0? -1ll: (int64_t)(duration * 1e6f); // nanoseconds
	vibration.frequency = frequency;
	XrHapticActionInfo hapticActionInfo{XR_TYPE_HAPTIC_ACTION_INFO, nullptr, xr::actions.haptic, xr::hand[side].subaction};
	xrApplyHapticFeedback(xr::session, &hapticActionInfo, (XrHapticBaseHeader*)&vibration);
}


void VRComponent::updateTransforms(uint64 predictedTime) {
	if(xr::sessionState != XR_SESSION_STATE_FOCUSED) return;

	// Update hand position based on the predicted time of when the frame will be rendered! This
	// should result in a more accurate location, and reduce perceived lag.
	constexpr XrFlags64 requiredFlags = XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
	XrSpaceLocation spaceRelation = { XR_TYPE_SPACE_LOCATION };

	XrResult result = xrLocateSpace(xr::headSpace, xr::appSpace, predictedTime, &spaceRelation);
	if(XR_UNQUALIFIED_SUCCESS(result) && (spaceRelation.locationFlags&requiredFlags) == requiredFlags) {
		xr::convertPose(m_controllerPose[vr::HEAD], spaceRelation.pose);
	}

	for(auto& hand: xr::hand) {
		if(hand.active) {
			XrResult result = xrLocateSpace(hand.space, xr::appSpace, predictedTime, &spaceRelation);
			if(XR_UNQUALIFIED_SUCCESS(result) && (spaceRelation.locationFlags&requiredFlags) == requiredFlags) {
				xr::convertPose(m_controllerPose[hand.role], spaceRelation.pose);
			}

			if(hand.handTracker) {
				XrHandJointsMotionRangeInfoEXT motionRange = { XR_TYPE_HAND_JOINTS_MOTION_RANGE_INFO_EXT };
				motionRange.handJointsMotionRange = XR_HAND_JOINTS_MOTION_RANGE_UNOBSTRUCTED_EXT;

				XrHandJointsLocateInfoEXT locateInfo = { XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT, &motionRange };
				locateInfo.baseSpace = xr::appSpace;
				locateInfo.time = predictedTime;

				XrHandJointLocationsEXT locations = {XR_TYPE_HAND_JOINT_LOCATIONS_EXT};
				locations.jointCount = XR_HAND_JOINT_COUNT_EXT;
				locations.jointLocations = hand.jointLocations;
				xrLocateHandJointsEXT(hand.handTracker, &locateInfo, &locations);
			}
		}
	}

	// ToDo: Additional Trackers
	
}


void VRComponent::draw() {
	if(!xr::instance || !m_scene) return;
	if(xr::sessionState > XR_SESSION_STATE_FOCUSED) return;

	XrFrameState frameState = { XR_TYPE_FRAME_STATE };
	xrWaitFrame (xr::session, nullptr, &frameState);
	xrBeginFrame(xr::session, nullptr);

	// Execute any code that's dependant on the predicted time, such as updating tracked locations
	updateTransforms(frameState.predictedDisplayTime);

	// If the session is active, lets render our layer in the compositor!
	XrCompositionLayerBaseHeader *layer = nullptr;
	XrCompositionLayerProjection layerProj = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
	std::vector<XrCompositionLayerProjectionView> views;
	std::vector<XrCompositionLayerDepthInfoKHR> depth;
	if(xr::sessionState == XR_SESSION_STATE_VISIBLE || xr::sessionState == XR_SESSION_STATE_FOCUSED) {
		
		// Find the state and location of each viewpoint at the predicted time
		uint viewCount = 0;
		XrViewState viewState  = { XR_TYPE_VIEW_STATE };
		XrViewLocateInfo locateInfo = { XR_TYPE_VIEW_LOCATE_INFO };
		locateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
		locateInfo.displayTime           = frameState.predictedDisplayTime;
		locateInfo.space                 = xr::appSpace;
		xrLocateViews(xr::session, &locateInfo, &viewState, xr::views.size(), &viewCount, xr::views.data());
		views.resize(viewCount, {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW});
		depth.resize(viewCount, {XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR});

		// And now we'll iterate through each viewpoint, and render it!
		for(uint i=0; i<viewCount; ++i) {
			xr::Swapchain& swap = xr::swapchains[i];

			// We need to ask which swapchain image to use for rendering! Which one will we get?
			// Who knows! It's up to the runtime to decide.
			uint colourIndex, depthIndex;
			xrAcquireSwapchainImage(swap.colourHandle, nullptr, &colourIndex);
			xrAcquireSwapchainImage(swap.depthHandle, nullptr, &depthIndex);
			xr::SwapChainImage& colourImage = *swap.images[colourIndex];
			xr::SwapChainImage& depthImage = *swap.depth[depthIndex];

			// Wait until the image is available to render to. The compositor could still be
			// reading from it.
			XrSwapchainImageWaitInfo waitInfo = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
			waitInfo.timeout = XR_INFINITE_DURATION;
			xrWaitSwapchainImage(swap.colourHandle, &waitInfo);
			xrWaitSwapchainImage(swap.depthHandle, &waitInfo);

			// Set up our rendering information for the viewpoint we're using right now!
			views[i].pose = xr::views[i].pose;
			views[i].fov  = xr::views[i].fov;
			views[i].subImage.swapchain        = swap.colourHandle;
			views[i].subImage.imageRect.offset = { 0, 0 };
			views[i].subImage.imageRect.extent = { (int)swap.width, (int)swap.height };
			views[i].subImage.imageArrayIndex = 0;
			views[i].next = &depth[i];

			depth[i].subImage.swapchain = swap.depthHandle;
			depth[i].subImage.imageRect.offset = {0, 0};
			depth[i].subImage.imageRect.extent = { (int)swap.width, (int)swap.height };
			depth[i].minDepth = 0.f;
			depth[i].maxDepth = 1.f;
			depth[i].nearZ = m_camera->getNear();
			depth[i].farZ = m_camera->getFar();

			// Render the scene to this viewport
			vr::Transform pose;
			xr::convertPose(pose, views[i].pose);
			const XrFovf& fov = xr::views[i].fov;

			m_camera->setPerspective(fov.angleLeft, fov.angleRight, fov.angleDown, fov.angleUp, m_camera->getNear(), m_camera->getFar());
			m_camera->setTransform(m_rootTransform.position + m_rootTransform.orientation * pose.position, m_rootTransform.orientation * pose.orientation); 

			colourImage.beginFrame();
			depthImage.beginFrame();

			m_target->attachColour(0, colourImage.getTarget());
			m_target->attachDepth(depthImage.getTarget());
			m_workspace->setCamera(m_camera);
			if(xr::flipTarget) {
				m_workspace->execute(xr::flipTarget->getTarget(), m_scene, m_renderer);
				xr::flipTarget->execute(m_target, m_renderer->getState());
			}
			else m_workspace->execute(m_target, m_scene, m_renderer);

			colourImage.endFrame();
			depthImage.endFrame();

			// And tell OpenXR we are done with rendering to this one!
			XrSwapchainImageReleaseInfo releaseInfo = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
			xrReleaseSwapchainImage(swap.colourHandle, &releaseInfo);
			xrReleaseSwapchainImage(swap.depthHandle, &releaseInfo);
		}
		layerProj.space = xr::appSpace;
		layerProj.viewCount = viewCount;
		layerProj.views = views.data();
		layer = (XrCompositionLayerBaseHeader*)&layerProj;
	}

	// We are finished with rendering our layer, so send it off for display!
	XrFrameEndInfo endInfo{ XR_TYPE_FRAME_END_INFO };
	endInfo.displayTime          = frameState.predictedDisplayTime;
	endInfo.environmentBlendMode = xr::blend;
	endInfo.layerCount           = layer? 1: 0;
	endInfo.layers               = &layer;
	xrEndFrame(xr::session, &endInfo);

	// Switch back to monitor window
	if(!xr::interop) Game::window()->makeCurrent();
	m_renderer->getState().reset();
}

#endif

// Stub - TODO: Investigate webxr
#ifdef EMSCRIPTEN
#include <base/game.h>
#include <base/input.h>
#include <base/vrcomponent.h>
VRComponent* VRComponent::create(const char*, base::Scene*, const base::CompositorGraph*) { return nullptr; }
VRComponent::~VRComponent() {}
void VRComponent::setScene(base::Scene*) {}
bool VRComponent::setCompositor(const base::CompositorGraph*) { return false; }
void VRComponent::adjustDepth(float, float) {}
bool VRComponent::isActive() const { return false; }
bool VRComponent::isEnabled() const {return false; }
BoundingBox2D VRComponent::getPlayArea() const { return BoundingBox2D(); }
bool VRComponent::hasTrackedObject(vr::Role) const { return false; }
const base::Joystick& VRComponent::getController(vr::Side) const { return base::Game::input()->joystick(-1); }
bool VRComponent::hasHandJoints(vr::Side) const { return false; }
vr::Transform VRComponent::getHandJoint(vr::Side, int) const { return {}; }
void VRComponent::update() {}
void VRComponent::draw() {}
#endif

