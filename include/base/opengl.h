#ifndef _BASE_OPENGL_
#define _BASE_OPENGL_

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#define GL_GLEXT_PROTOTYPES
#endif

#include <GL/gl.h>
#include <GL/glext.h>


#ifndef EMSCRIPTEN
#define GL_CHECK_ERROR  { int e=glGetError(); if(e) printf("OpenGL Error 0x%x: %s:%d\n", e, __FILE__, __LINE__); fflush(stdout); }
#else
#define GL_CHECK_ERROR
#endif

#endif

