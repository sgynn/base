cmake_minimum_required(VERSION 2.6)
project(basecode)

set(src
	src/camera.cpp
	src/collision.cpp
	src/file.cpp
	src/font.cpp
	src/framebuffer.cpp
	src/game.cpp
	src/glhprojection.cpp
	src/gui.cpp
	src/inifile.cpp
	src/input.cpp
	src/keytest.cpp
	src/material.cpp
	src/model.cpp
	src/modelloaderx.cpp
	src/png.cpp
	src/scene.cpp
	src/shader.cpp
	src/state.cpp
	src/window.cpp
	src/xml.cpp
)

set( png
	src/png/libpng.cpp
	src/png/pngerror.cpp
	src/png/pnggccrd.cpp
	src/png/pngget.cpp
	src/png/pngmem.cpp
	src/png/pngpread.cpp
	src/png/pngread.cpp
	src/png/pngrio.cpp
	src/png/pngrtran.cpp
	src/png/pngrutil.cpp
	src/png/pngset.cpp
	src/png/pngtrans.cpp
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
	include/base/camera.h
	include/base/collision.h
	include/base/draw.h
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
	include/base/math.h
	include/base/model.h
	include/base/modelloaderx.h
	include/base/opengl.h
	include/base/png.h
	include/base/scene.h
	include/base/shader.h
	include/base/state.h
	include/base/threads.h
	include/base/window.h
	include/base/xml.h
)


set(CMAKE_CXX_FLAGS "-g -Wall -DLINUX")
include_directories( include )
add_library(base STATIC ${src} ${png} ${zlib} ${headers} )
target_link_libraries(base GL X11 Xxf86vm)
install(TARGETS base DESTINATION lib64)
install(FILES ${headers} DESTINATION include/base)

