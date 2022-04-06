#pragma once

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#define GL_GLEXT_PROTOTYPES
#endif

#include <GL/gl.h>
#include <GL/glext.h>


#ifndef EMSCRIPTEN
#define GL_CHECK_ERROR  { for(int e=glGetError(); e; e=glGetError()) printf("OpenGL Error 0x%x: %s:%d\n", e, __FILE__, __LINE__); fflush(stdout); }
#else
#define GL_CHECK_ERROR
#endif


extern int initialiseOpenGLExtensions();

#ifdef WIN32

#define GL_ARRAY_BUFFER             0x8892
#define GL_ELEMENT_ARRAY_BUFFER     0x8893
#define GL_STATIC_DRAW              0x88E4

#define GL_FRAGMENT_SHADER          0x8B30
#define GL_VERTEX_SHADER            0x8B31
#define GL_GEOMETRY_SHADER          0x8DD9
#define GL_COMPILE_STATUS           0x8B81
#define GL_LINK_STATUS              0x8B82
#define GL_INFO_LOG_LENGTH          0x8B84
#define GL_SHADING_LANGUAGE_VERSION 0x8B8C

#define GL_TEXTURE_1D_ARRAY	        0x8C18
#define GL_TEXTURE_2D_ARRAY         0x8C1A
#define GL_R32F                     0x822E
#define GL_RG32F                    0x8230
#define GL_RGB32F                   0x8815
#define GL_RGBA32F                  0x8814
#define GL_R16F                     0x822D
#define GL_RG16F                    0x822F
#define GL_RGB16F                   0x881B
#define GL_RGBA16F                  0x881A
#define GL_RGB565                   0x8D62
#define GL_R11F_G11F_B10F           0x8C3A
#define GL_DEPTH_COMPONENT16        0x81A5
#define GL_DEPTH_COMPONENT24        0x81A6
#define GL_DEPTH_COMPONENT32        0x81A7
#define GL_DEPTH24_STENCIL8         0x88F0
#define GL_HALF_FLOAT               0x140B

#define GL_FRAMEBUFFER              0x8D40
#define GL_RENDERBUFFER             0x8D41
#define GL_COLOR_ATTACHMENT0        0x8CE0
#define GL_DEPTH_ATTACHMENT         0x8D00
#define GL_DEPTH_COMPONENT16        0x81A5
#define GL_DEPTH_COMPONENT24        0x81A6
#define GL_DEPTH_COMPONENT32        0x81A7
#define GL_FRAMEBUFFER_COMPLETE     0x8CD5
#define GL_DEPTH_STENCIL_ATTACHMENT 0x821A
#define GL_DEPTH24_STENCIL8         0x88F0

extern PFNGLTEXIMAGE3DPROC              glTexImage3D;
extern PFNGLTEXSUBIMAGE3DPROC           glTexSubImage3D;
extern PFNGLGENERATEMIPMAPPROC          glGenerateMipmap;
extern PFNGLACTIVETEXTUREARBPROC        glActiveTexture;
extern PFNGLCOMPRESSEDTEXIMAGE1DPROC    glCompressedTexImage1D;
extern PFNGLCOMPRESSEDTEXIMAGE2DPROC    glCompressedTexImage2D;
extern PFNGLCOMPRESSEDTEXIMAGE3DPROC    glCompressedTexImage3D;
extern PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC glCompressedTexSubImage1D;
extern PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC glCompressedTexSubImage2D;
extern PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC glCompressedTexSubImage3D;
extern PFNGLBLENDFUNCSEPARATEPROC       glBlendFuncSeparate;

extern PFNGLCREATESHADERPROC   glCreateShader;
extern PFNGLSHADERSOURCEPROC   glShaderSource;
extern PFNGLCOMPILESHADERPROC  glCompileShader;
extern PFNGLCREATEPROGRAMPROC  glCreateProgram;
extern PFNGLATTACHSHADERPROC   glAttachShader;
extern PFNGLLINKPROGRAMPROC    glLinkProgram;
extern PFNGLDELETESHADERPROC   glDeleteShader;
extern PFNGLDELETEPROGRAMPROC  glDeleteProgram;
extern PFNGLUSEPROGRAMPROC     glUseProgram;
extern PFNGLGETPROGRAMIVPROC   glGetProgramiv;
extern PFNGLGETSHADERIVPROC    glGetShaderiv;
extern PFNGLGETPROGRAMINFOLOGPROC    glGetProgramInfoLog;
extern PFNGLGETSHADERINFOLOGPROC     glGetShaderInfoLog;
extern PFNGLGETATTACHEDSHADERSPROC   glGetAttachedShader;
extern PFNGLGETUNIFORMLOCATIONPROC   glGetUniformLocation;
extern PFNGLGETATTRIBLOCATIONPROC    glGetAttribLocation;
extern PFNGLBINDFRAGDATALOCATIONPROC glBindFragDataLocation;
extern PFNGLBINDATTRIBLOCATIONPROC   glBindAttribLocation;

extern PFNGLUNIFORM1IVPROC         glUniform1iv;
extern PFNGLUNIFORM2IVPROC         glUniform2iv;
extern PFNGLUNIFORM3IVPROC         glUniform3iv;
extern PFNGLUNIFORM4IVPROC         glUniform4iv;
extern PFNGLUNIFORM1FVPROC         glUniform1fv;
extern PFNGLUNIFORM2FVPROC         glUniform2fv;
extern PFNGLUNIFORM3FVPROC         glUniform3fv;
extern PFNGLUNIFORM4FVPROC         glUniform4fv;
extern PFNGLUNIFORMMATRIX2FVPROC   glUniformMatrix2fv;
extern PFNGLUNIFORMMATRIX3FVPROC   glUniformMatrix3fv;
extern PFNGLUNIFORMMATRIX4FVPROC   glUniformMatrix4fv;
extern PFNGLUNIFORMMATRIX3X4FVPROC glUniformMatrix3x4fv;
extern PFNGLGETACTIVEUNIFORMPROC   glGetActiveUniform;

extern PFNGLENABLEVERTEXATTRIBARRAYPROC   glEnableVertexAttribArray;
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC  glDisableVertexAttribArray;
extern PFNGLVERTEXATTRIBPOINTERPROC       glVertexAttribPointer;
extern PFNGLVERTEXATTRIBIPOINTERPROC      glVertexAttribIPointer;
extern PFNGLVERTEXATTRIBDIVISORPROC       glVertexAttribDivisor;

extern PFNGLGENVERTEXARRAYSPROC       glGenVertexArray;
extern PFNGLBINDVERTEXARRAYPROC       glBindVertexArray;
extern PFNGLDELETEVERTEXARRAYSPROC    glDeleteVertexArray;
extern PFNGLDRAWELEMENTSINSTANCEDPROC glDrawElementsInstanced;
extern PFNGLDRAWARRAYSINSTANCEDPROC   glDrawArraysInstanced;
#endif


