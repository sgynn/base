cmake_minimum_required(VERSION 2.8.12)
project(basecode)

set(src
	src/archive.cpp
	src/binaryfile.cpp
	src/camera.cpp
	src/cameras.cpp
	src/collision.cpp
	src/directory.cpp
	src/file.cpp
	src/game.cpp
	src/gamestate.cpp
	src/glhprojection.cpp
	src/hardwarebuffer.cpp
	src/image.cpp
	src/inifile.cpp
	src/input.cpp
	src/joystick.cpp
	src/opengl.cpp
	src/png.cpp
	src/resources.cpp
	src/scenecomponent.cpp
	src/tiff.cpp
	src/dds.cpp
	src/virtualfilesystem.cpp
	src/window.cpp
	src/xml.cpp

	src/miniz.c

	src/scene/autovariables.cpp
	src/scene/compositor.cpp
	src/scene/debuggeometry.cpp
	src/scene/drawable.cpp
	src/scene/framebuffer.cpp
	src/scene/material.cpp
	src/scene/renderer.cpp
	src/scene/scene.cpp
	src/scene/shader.cpp
	src/scene/texture.cpp

	src/model/animationcontroller.cpp
	src/model/animation.cpp
	src/model/animationstate.cpp
	src/model/batcher.cpp
	src/model/bmloader.cpp
	src/model/mesh.cpp
	src/model/model.cpp
	src/model/primitives.cpp
	src/model/skeleton.cpp
	src/model/wavefront.cpp

	src/vr/directx-interop.cpp
	src/vr/fliptarget.cpp
	src/vr/openxr.cpp

	src/navmesh/navdrawable.cpp
	src/navmesh/navedit.cpp
	src/navmesh/navmesh.cpp
	src/navmesh/navpath.cpp

	src/world/foliage.cpp
	src/world/landscape.cpp
	src/world/object.cpp
	src/world/objectworld.cpp
	src/world/particleinstance.cpp
	src/world/skydome.cpp
	src/world/terraindrawable.cpp
	src/world/terrainfoliage.cpp
	src/world/infoplate.cpp
	src/world/floatingtext.cpp
)

set(gui
	src/gui/font.cpp
	src/gui/gui.cpp
	src/gui/imagegen.cpp
	src/gui/layouts.cpp
	src/gui/lists.cpp
	src/gui/loader.cpp
	src/gui/renderer.cpp
	src/gui/table.cpp
	src/gui/textbox.cpp
	src/gui/tree.cpp
	src/gui/widgets.cpp
	src/gui/colourpicker.cpp
	src/gui/filedialog.cpp

	src/gui/systemfont.cpp
	src/gui/bitmapfont.cpp
	src/gui/freetypefont.cpp
)

set(script
	src/script/console.cpp
	src/script/script.cpp
	src/script/variable.cpp
)

set(audio
	src/audio/audio.cpp
	src/audio/loader.cpp
	src/audio/object.cpp
	src/audio/source.cpp
	src/audio/log.cpp
)

set(particles
	src/particles/affectors.h
	src/particles/definition.cpp
	src/particles/emitters.h
	src/particles/particles.cpp
	src/particles/renderers.cpp
	src/particles/renderers.h
)

set(editor
	src/editor/assetbrowser.cpp
	src/editor/audioeditor.cpp
	src/editor/compositoreditor.cpp
	src/editor/editor.cpp
	src/editor/gizmo.cpp
	src/editor/layout.cpp
	src/editor/materialeditor.cpp
	src/editor/modelviewer.cpp
	src/editor/nodeeditor.cpp
	src/editor/particleeditor.cpp
	src/editor/renderdoc.cpp
)

set(headers
	include/base/assert.h
	include/base/archive.h
	include/base/animationcontroller.h
	include/base/animation.h
	include/base/animationstate.h
	include/base/audio.h
	include/base/autovariables.h
	include/base/batcher.h
	include/base/binaryfile.h
	include/base/bmloader.h
	include/base/bounds.h
	include/base/bresenham.h
	include/base/camera.h
	include/base/collision.h
	include/base/colour.h
	include/base/compositor.h
	include/base/dds.h
	include/base/debuggeometry.h
	include/base/directory.h
	include/base/drawable.h
	include/base/drawablemesh.h
	include/base/file.h
	include/base/fpscamera.h
	include/base/framebuffer.h
	include/base/game.h
	include/base/gamestate.h
	include/base/hardwarebuffer.h
	include/base/hashmap.h
	include/base/image.h
	include/base/inifile.h
	include/base/input.h
	include/base/material.h
	include/base/math.h
	include/base/matrix.h
	include/base/mesh.h
	include/base/model.h
	include/base/modellayout.h
	include/base/noise.h
	include/base/opengl.h
	include/base/orbitcamera.h
	include/base/parse.h
	include/base/particledef.h
	include/base/particles.h
	include/base/png.h
	include/base/point.h
	include/base/primitives.h
	include/base/quaternion.h
	include/base/random.h
	include/base/renderer.h
	include/base/resources.h
	include/base/resourcemanager.h
	include/base/rtti.h
	include/base/scenecomponent.h
	include/base/scene.h
	include/base/shader.h
	include/base/skeleton.h
	include/base/string.h
	include/base/texture.h
	include/base/thread.h
	include/base/tiff.h
	include/base/vec.h
	include/base/virtualfilesystem.h
	include/base/vrcomponent.h
	include/base/wavefront.h
	include/base/window.h
	include/base/xml.h

	include/base/variable.h
	include/base/script.h
	include/base/console.h

	include/base/navmesh.h
	include/base/navpath.h
	include/base/navdrawable.h

	include/base/consolecomponent.h
	include/base/guicomponent.h
)

set(worldheaders
	include/base/world/foliage.h
	include/base/world/landscape.h
	include/base/world/object.h
	include/base/world/objectworld.h
	include/base/world/particleinstance.h
	include/base/world/skydome.h
	include/base/world/terraindrawable.h
	include/base/world/terrainfoliage.h
	include/base/world/infoplate.h
	include/base/world/floatingtext.h
)

set(guiheaders
	include/base/gui/any.h
	include/base/gui/animator.h
	include/base/gui/delegate.h
	include/base/gui/font.h
	include/base/gui/freetype.h
	include/base/gui/gui.h
	include/base/gui/layouts.h
	include/base/gui/lists.h
	include/base/gui/renderer.h
	include/base/gui/skin.h
	include/base/gui/tree.h
	include/base/gui/transform.h
	include/base/gui/widgets.h
	include/base/gui/colourpicker.h
	include/base/gui/filedialog.h
	include/base/gui/menubuilder.h
)

set(editorheaders
	include/base/editor/assetbrowser.h
	include/base/editor/audioeditor.h
	include/base/editor/compositoreditor.h
	include/base/editor/editor.h
	include/base/editor/embed.h
	include/base/editor/gizmo.h
	include/base/editor/gradientbox.h
	include/base/editor/graphbox.h
	include/base/editor/layout.h
	include/base/editor/nodeeditor.h
	include/base/editor/particleeditor.h
	include/base/editor/renderdoc.h
	include/base/editor/resources.h
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
	set(WEB 1)
endif()


if(WEB)
	list(APPEND src src/window_em.cpp)
	list(APPEND headers include/base/window_em.h)
elseif(MINGW)
	list(APPEND src src/window_win32.cpp)
	list(APPEND headers include/base/window_win32.h)
elseif(UNIX)
	list(APPEND src src/window_x11.cpp)
	list(APPEND headers include/base/window_x11.h)
endif()

set(CMAKE_CXX_STANDARD 14)	# C++14 - needed for auto return type in Delegate class

find_package(Freetype)
if(${Freetype_FOUND})
	set(FREETYPE "-DFREETYPE")
endif()

add_library(base STATIC ${src} ${gui} ${script} ${audio} ${particles} ${headers} ${guiheaders} ${worldheaders} ${editor} ${editorheaders} )
include_directories(include)
include_directories(${FREETYPE_INCLUDE_DIRS})

install(FILES ${headers} DESTINATION include/base)
install(FILES ${guiheaders} DESTINATION include/base/gui)
install(FILES ${worldheaders} DESTINATION include/base/world)
install(FILES ${editorheaders} DESTINATION include/base/editor)

get_filename_component(ROOT_PATH . ABSOLUTE)
set(EditorPath -DEDITOR_DATA='\"${ROOT_PATH}/src/editor/data\"')

if(WEB)
	set(EditorPath -DEDITOR_DATA='\"./data/editor\"')
	#set(CMAKE_CXX_FLAGS "-g -Wall -DEMSCRIPTEN -DFREETYPE -pthread -s USE_PTHREADS=1 -s USE_FREETYPE=1")
	set(CMAKE_CXX_FLAGS "-g -Wall -DEMSCRIPTEN -DFREETYPE ${EditorPath} -s USE_FREETYPE=1")
	install(TARGETS base DESTINATION lib/wasm32-emscripten)
elseif(MINGW)
	set(CMAKE_CXX_FLAGS "-g -Wall -DWIN32 ${FREETYPE} ${EditorPath}")
	install(TARGETS base DESTINATION lib)
elseif(UNIX)
	set(CMAKE_CXX_FLAGS "-g -Wall -DLINUX ${FREETYPE} ${EditorPath}")
	set(CMAKE_C_FLAGS "-g -Wall")
	message(STATUS ${CMAKE_CXX_FLAGS})
	install(TARGETS base DESTINATION lib64)
else()
	message(WARNING "Invalid target")
endif()

