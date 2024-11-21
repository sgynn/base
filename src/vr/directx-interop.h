#pragma once


// Attempt at using opengl -> directx surface sharing to convince openxr to play nice
// Copied from OpenMW VR

#ifdef WIN32
class ID3D11Device;
class ID3D11Texture2D;
namespace xr {
	typedef long long int64;
	typedef unsigned long long uint64;
	typedef unsigned int uint32;
	class SwapChainImage;

	int64 GLFormatToDXGIFormat(int64);
	int64 DXGIFormatToGLFormat(int64);

	class DirectXWGLInterop {
		public:
		static DirectXWGLInterop* create();
		~DirectXWGLInterop();
		ID3D11Device* getD3DDeviceHandle();
		SwapChainImage* createSwapChainImage(ID3D11Texture2D*, uint64 format);
		private:
		DirectXWGLInterop();
		struct DirectXWGLInteropData* m_d3d;
	};
}

#else

// Dummy implementation to avoid some ifdefs
namespace xr {
	typedef long long int64;
	class DirectXWGLInterop { };
	inline int64 GLFormatToDXGIFormat(int64) { return 0; }
	inline int64 DXGIFormatToGLFormat(int64) { return 0; }
}

#endif

