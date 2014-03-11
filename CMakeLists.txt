cmake_minimum_required(VERSION 2.6)
project(basecode)

set(src
	src/camera.cpp
	src/cameras.cpp
	src/collision.cpp
	src/directory.cpp
	src/file.cpp
	src/font.cpp
	src/framebuffer.cpp
	src/game.cpp
	src/glhprojection.cpp
	src/gui.cpp
	src/guifile.cpp
	src/inifile.cpp
	src/input.cpp
	src/material.cpp
	src/png.cpp
	src/scene.cpp
	src/shader.cpp
	src/state.cpp
	src/texture.cpp
	src/window.cpp
	src/xml.cpp

	src/model/model.cpp
	src/model/mesh.cpp
	src/model/morph.cpp
	src/model/skeleton.cpp
	src/model/animation.cpp
	src/model/drawmodel.cpp
	src/model/xmodel.cpp
)

set( png
	src/png/png.c
	src/png/pngerror.c
	src/png/pngget.c
	src/png/pngmem.c
	src/png/pngread.c
	src/png/pngrio.c
	src/png/pngrtran.c
	src/png/pngrutil.c
	src/png/pngset.c
	src/png/pngtrans.c
	src/png/pngwio.c
	src/png/pngwrite.c
	src/png/pngwtran.c
	src/png/pngwutil.c
)

set( zlib
	src/zlib/adler32.cpp
	src/zlib/compress.cpp
	src/zlib/crc32.cpp
	src/zlib/deflate.cpp
	src/zlib/infback.cpp
	src/zlib/inffast.cpp
	src/zlib/inflate.cpp
	src/zlib/inftrees.cpp
	src/zlib/trees.cpp
	src/zlib/uncompr.cpp
	src/zlib/zutil.cpp
)

set(headers
	include/base/animation.h
	include/base/camera.h
	include/base/colour.h
	include/base/fpscamera.h
	include/base/orbitcamera.h
	include/base/collision.h
	include/base/draw.h
	include/base/directory.h
	include/base/file.h
	include/base/font.h
	include/base/framebuffer.h
	include/base/game.h
	include/base/graph.h
	include/base/gui.h
	include/base/hashmap.h
	include/base/inifile.h
	include/base/input.h
	include/base/material.h
	include/base/mesh.h
	include/base/morph.h
	include/base/model.h
	include/base/noise.h
	include/base/opengl.h
	include/base/png.h
	include/base/scene.h
	include/base/shader.h
	include/base/skeleton.h
	include/base/state.h
	include/base/texture.h
	include/base/threads.h
	include/base/window.h
	include/base/xml.h
	include/base/xmodel.h

	include/base/math.h
	include/base/bounds.h
	include/base/matrix.h
	include/base/point.h
	include/base/quaternion.h
	include/base/vec.h
)

add_library(base STATIC ${src} ${png} ${zlib} ${headers} )
include_directories( include )
install(FILES ${headers} DESTINATION include/base)

if(${UNIX})
	set(CMAKE_CXX_FLAGS "-g -Wall -DLINUX")
	install(TARGETS base DESTINATION lib64)
elseif(${WIN32})
	set(CMAKE_CXX_FLAGS "-g -Wall -DWIN32")
	install(TARGETS base DESTINATION lib)
endif(${WIN32})

