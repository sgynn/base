#ifdef WIN32
#include "directx-interop.h"
#include "openxr.h"

#include <base/opengl.h>
#include <base/shader.h>
#include <base/material.h>
#include <base/renderer.h>

#include <d3d11.h>
#include <dxgi1_2.h>
#include <stdexcept>

namespace xr {
	typedef BOOL(WINAPI* WGLDXSETRESOURCESHAREHANDLENV)(void* dxObject, HANDLE shareHandle);
	typedef HANDLE(WINAPI* WGLDXOPENDEVICENV)(void* dxDevice);
	typedef BOOL(WINAPI* WGLDXCLOSEDEVICENV)(HANDLE hDevice);
	typedef HANDLE(WINAPI* WGLDXREGISTEROBJECTNV)(HANDLE hDevice, void* dxObject, GLuint name, GLenum type, GLenum access);
	typedef BOOL(WINAPI* WGLDXUNREGISTEROBJECTNV)(HANDLE hDevice, HANDLE hObject);
	typedef BOOL(WINAPI* WGLDXOBJECTACCESSNV)(HANDLE hObject, GLenum access);
	typedef BOOL(WINAPI* WGLDXLOCKOBJECTSNV)(HANDLE hDevice, GLint count, HANDLE* hObjects);
	typedef BOOL(WINAPI* WGLDXUNLOCKOBJECTSNV)(HANDLE hDevice, GLint count, HANDLE* hObjects);

	struct DirectXWGLInteropData {
		ID3D11Device* device = nullptr;
		ID3D11DeviceContext* context = nullptr;

		HMODULE D3D11Dll = nullptr;
		PFN_D3D11_CREATE_DEVICE       D3D11CreateDevice = nullptr;
		WGLDXSETRESOURCESHAREHANDLENV wglDXSetResourceShareHandleNV = nullptr;
		WGLDXOPENDEVICENV             wglDXOpenDeviceNV = nullptr;
		WGLDXCLOSEDEVICENV            wglDXCloseDeviceNV = nullptr;
		WGLDXREGISTEROBJECTNV         wglDXRegisterObjectNV = nullptr;
		WGLDXUNREGISTEROBJECTNV       wglDXUnregisterObjectNV = nullptr;
		WGLDXOBJECTACCESSNV           wglDXObjectAccessNV = nullptr;
		WGLDXLOCKOBJECTSNV            wglDXLockObjectsNV = nullptr;
		WGLDXUNLOCKOBJECTSNV          wglDXUnlockObjectsNV = nullptr;
		HANDLE wglDXDevice = nullptr;
	};


	class DirectXSharedImage final : public SwapChainImage {
		public:
		DirectXSharedImage(DirectXWGLInteropData* d3d, ID3D11Texture2D* texure, uint64 format);
		~DirectXSharedImage();
		void beginFrame() override;
		void endFrame() override;
		protected:
		DirectXWGLInteropData* m_d3d;
        ID3D11Texture2D* m_targetTexture;
        ID3D11Texture2D* m_sharedTexture = nullptr;
        void* m_resourceShareHandle = nullptr;
	};

	DirectXSharedImage::DirectXSharedImage(DirectXWGLInteropData* d3d, ID3D11Texture2D* texture, uint64 format)
		: m_d3d(d3d)
		, m_targetTexture(texture)
	{
		uint32 glTexture = 0;
		ID3D11Device* targetDevice = nullptr;
		texture->GetDevice(&targetDevice);
		if(targetDevice != d3d->device) printf("Error: D3DDevice is different somehow\n");
		//m_device->GetImmediateContext(&m_context);
		glGenTextures(1, &glTexture);
		D3D11_TEXTURE2D_DESC desc{};
		texture->GetDesc(&desc);
		// Some OpenXR runtimes create textures that cannot be shared, so create a copy
		D3D11_TEXTURE2D_DESC sharedDesc{};
		sharedDesc.Width = desc.Width;
		sharedDesc.Height = desc.Height;
		sharedDesc.MipLevels = desc.MipLevels;
		sharedDesc.ArraySize = desc.ArraySize;
		sharedDesc.Format = (DXGI_FORMAT)format; // Different somehow - TYPELESS vs UNORM formats
		sharedDesc.SampleDesc = desc.SampleDesc;
		sharedDesc.Usage = D3D11_USAGE_DEFAULT;
		sharedDesc.BindFlags = 0;
		sharedDesc.CPUAccessFlags = 0;
		sharedDesc.MiscFlags = 0;
		d3d->device->CreateTexture2D(&sharedDesc, nullptr, &m_sharedTexture);
		m_resourceShareHandle = d3d->wglDXRegisterObjectNV(d3d->wglDXDevice, m_sharedTexture, glTexture, desc.ArraySize>1? GL_TEXTURE_2D_ARRAY: GL_TEXTURE_2D, 1);
		if(!m_resourceShareHandle) printf("Failed to connect shared resource (%#x) [%#lx]\n", (uint32)desc.Format, GetLastError());
		m_target = base::Texture(glTexture);
	}
	DirectXSharedImage::~DirectXSharedImage() {
		m_d3d->wglDXUnregisterObjectNV(m_d3d->wglDXDevice, m_resourceShareHandle);
	}
	void DirectXSharedImage::beginFrame() {
		if(!m_d3d->wglDXLockObjectsNV(m_d3d->wglDXDevice, 1, &m_resourceShareHandle))
			printf("DXLockObject failed [%#lx]\n", GetLastError());
	}
	void DirectXSharedImage::endFrame() {
		if(!m_d3d->wglDXUnlockObjectsNV(m_d3d->wglDXDevice, 1, &m_resourceShareHandle))
			printf("DXUnockObject failed [%#lx]\n", GetLastError());
		m_d3d->context->CopyResource(m_targetTexture, m_sharedTexture);
	}


	// =========================================================================================== //
	
	DirectXWGLInterop* DirectXWGLInterop::create() {
		typedef const char* (WINAPI* PFNWGLGETEXTENSIONSSTRINGARBPROC)(HDC hdc);
		PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = 0;
		wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)(wglGetProcAddress("wglGetExtensionsStringARB"));
		if(wglGetExtensionsStringARB) {
			const char* extensions = wglGetExtensionsStringARB(wglGetCurrentDC());
			bool enabled = strstr(extensions, "WGL_NV_DX_interop2");
			if(enabled) return new DirectXWGLInterop();
			else printf("OpenGL -> DirectX bridge unsupported\n");
		}
		else printf("Failed to query OpenGL extensions\n");
		return nullptr;
	}
		

	DirectXWGLInterop::DirectXWGLInterop() : m_d3d(new DirectXWGLInteropData) {
		DirectXWGLInteropData& dx = *m_d3d;
		dx.D3D11Dll = LoadLibrary("D3D11.dll");
		if (!dx.D3D11Dll)
			throw std::runtime_error("Current OpenXR runtime requires DirectX >= 11.0 but D3D11.dll was not found.");

		dx.D3D11CreateDevice = reinterpret_cast<PFN_D3D11_CREATE_DEVICE>(GetProcAddress(dx.D3D11Dll, "D3D11CreateDevice"));

		if (!dx.D3D11CreateDevice)
throw std::runtime_error("Symbol 'D3D11CreateDevice' not found in D3D11.dll");

		// Create the device and device context objects
		dx.D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			0,
			nullptr,
			0,
			D3D11_SDK_VERSION,
			&dx.device,
			nullptr,
			&dx.context);

		if(dx.context) printf("D3D11 Context created\n");

		#define LOAD_WGL(a) dx.a = reinterpret_cast<decltype(dx.a)>(wglGetProcAddress(#a)); if(!dx.a) throw std::runtime_error("Extension WGL_NV_DX_interop2 required to run via DirectX missing expected symbol '" #a "'.")
		LOAD_WGL(wglDXSetResourceShareHandleNV);
		LOAD_WGL(wglDXOpenDeviceNV);
		LOAD_WGL(wglDXCloseDeviceNV);
		LOAD_WGL(wglDXRegisterObjectNV);
		LOAD_WGL(wglDXUnregisterObjectNV);
		LOAD_WGL(wglDXObjectAccessNV);
		LOAD_WGL(wglDXLockObjectsNV);
		LOAD_WGL(wglDXUnlockObjectsNV);
		#undef LOAD_WGL

		dx.wglDXDevice = dx.wglDXOpenDeviceNV(dx.device);
		if(!dx.wglDXDevice)
			throw std::runtime_error("Failed to create OpenGL -> DirectX bridge");
	}

	DirectXWGLInterop::~DirectXWGLInterop() {
		delete m_d3d;
	}

	ID3D11Device* DirectXWGLInterop::getD3DDeviceHandle() {
		return m_d3d->device;
	}

	SwapChainImage* DirectXWGLInterop::createSwapChainImage(ID3D11Texture2D* texture, uint64 format) {
		return new DirectXSharedImage(m_d3d, texture, format);
	}




	// =========================================================================================== //


	int64 GLFormatToDXGIFormat(int64 format) {
		switch (format) {
		case GL_RGBA32F:            return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case GL_RGBA32UI:           return DXGI_FORMAT_R32G32B32A32_UINT;
		case GL_RGBA32I:            return DXGI_FORMAT_R32G32B32A32_SINT;
		case GL_RGB32F:             return DXGI_FORMAT_R32G32B32_FLOAT;
		case GL_RGB32UI:            return DXGI_FORMAT_R32G32B32_UINT;
		case GL_RGB32I:             return DXGI_FORMAT_R32G32B32_SINT;
		case GL_RGBA16F:            return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case GL_RGBA16:             return DXGI_FORMAT_R16G16B16A16_UNORM;
		case GL_RGBA16UI:           return DXGI_FORMAT_R16G16B16A16_UINT;
		case GL_RGBA16_SNORM:       return DXGI_FORMAT_R16G16B16A16_SNORM;
		case GL_RGBA16I:            return DXGI_FORMAT_R16G16B16A16_SINT;
		case GL_RGBA8:              return DXGI_FORMAT_R8G8B8A8_UNORM;
		case GL_SRGB8_ALPHA8:       return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case GL_RGBA8UI:            return DXGI_FORMAT_R8G8B8A8_UINT;
		case GL_RGBA8_SNORM:        return DXGI_FORMAT_R8G8B8A8_SNORM;
		case GL_RGBA8I:             return DXGI_FORMAT_R8G8B8A8_SINT;
		case GL_RGB10_A2:           return DXGI_FORMAT_R10G10B10A2_UNORM;
		case GL_RGB10_A2UI:         return DXGI_FORMAT_R10G10B10A2_UINT;
		case GL_R11F_G11F_B10F:     return DXGI_FORMAT_R11G11B10_FLOAT;
		case GL_DEPTH32F_STENCIL8:  return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		case GL_DEPTH_COMPONENT32F: return DXGI_FORMAT_D32_FLOAT;
		case GL_DEPTH24_STENCIL8:   return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case GL_DEPTH_COMPONENT16:  return DXGI_FORMAT_D16_UNORM;
		default: return 0;
		}
	}

	int64_t DXGIFormatToGLFormat(int64_t format) {
		switch (format) {
		case DXGI_FORMAT_R32G32B32A32_FLOAT:   return GL_RGBA32F;
		case DXGI_FORMAT_R32G32B32A32_UINT:    return GL_RGBA32UI;
		case DXGI_FORMAT_R32G32B32A32_SINT:    return GL_RGBA32I;
		case DXGI_FORMAT_R32G32B32_FLOAT:      return GL_RGB32F;
		case DXGI_FORMAT_R32G32B32_UINT:       return GL_RGB32UI;
		case DXGI_FORMAT_R32G32B32_SINT:       return GL_RGB32I;
		case DXGI_FORMAT_R16G16B16A16_FLOAT:   return GL_RGBA16F;
		case DXGI_FORMAT_R16G16B16A16_UNORM:   return GL_RGBA16;
		case DXGI_FORMAT_R16G16B16A16_UINT:    return GL_RGBA16UI;
		case DXGI_FORMAT_R16G16B16A16_SNORM:   return GL_RGBA16_SNORM;
		case DXGI_FORMAT_R16G16B16A16_SINT:    return GL_RGBA16I;
		case DXGI_FORMAT_R8G8B8A8_UNORM:       return GL_RGBA8;
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:  return GL_SRGB8_ALPHA8;
		case DXGI_FORMAT_R8G8B8A8_UINT:        return GL_RGBA8UI;
		case DXGI_FORMAT_R8G8B8A8_SNORM:       return GL_RGBA8_SNORM;
		case DXGI_FORMAT_R8G8B8A8_SINT:        return GL_RGBA8I;
		case DXGI_FORMAT_R10G10B10A2_UNORM:    return GL_RGB10_A2;
		case DXGI_FORMAT_R10G10B10A2_UINT:     return GL_RGB10_A2UI;
		case DXGI_FORMAT_R11G11B10_FLOAT:      return GL_R11F_G11F_B10F;
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: return GL_DEPTH32F_STENCIL8;
		case DXGI_FORMAT_D32_FLOAT:            return GL_DEPTH_COMPONENT32F;
		case DXGI_FORMAT_D24_UNORM_S8_UINT:    return GL_DEPTH24_STENCIL8;
		case DXGI_FORMAT_D16_UNORM:            return GL_DEPTH_COMPONENT16;
		default: return 0;
		}
	}
}

#endif

