cmake_minimum_required(VERSION 2.6)
project(basecode)

set(src
	camera.cpp
	file.cpp
	font.cpp
	framebuffer.cpp
	game.cpp
	inifile.cpp
	input.cpp
	keytest.cpp
	material.cpp
	model.cpp
	modelloaderx.cpp
	png.cpp
	scene.cpp
	shader.cpp
	state.cpp
	window.cpp
	xml.cpp
)

set( png
	png/libpng.cpp
	png/pngerror.cpp
	png/pnggccrd.cpp
	png/pngget.cpp
	png/pngmem.cpp
	png/pngpread.cpp
	png/pngread.cpp
	png/pngrio.cpp
	png/pngrtran.cpp
	png/pngrutil.cpp
	png/pngset.cpp
	png/pngtrans.cpp
)

set( zlib
	zlib/adler32.cpp
	zlib/compress.cpp
	zlib/crc32.cpp
	zlib/deflate.cpp
	zlib/infback.cpp
	zlib/inffast.cpp
	zlib/inflate.cpp
	zlib/inftrees.cpp
	zlib/trees.cpp
	zlib/uncompr.cpp
	zlib/zutil.cpp
)


add_library(base ${src} ${png} ${zlib} )


set(headers
	camera.h
	draw.h
	file.h
	font.h
	framebuffer.h
	game.h
	hashmap.h
	inifile.h
	input.h
	material.h
	math.h
	model.h
	modelloaderx.h
	opengl.h
	png.h
	scene.h
	shader.h
	state.h
	threads.h
	window.h
	xml.h
)

install(TARGETS base DESTINATION lib)
install(FILES ${headers} DESTINATION include/base)
