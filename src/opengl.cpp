#include <base/opengl.h>
#include <cstdio>

#ifdef WIN32
#ifndef APIENTRYP
#define APIENTRYP __stdcall *
#endif

/* Seems these are not needed. Move the to opengl.h if something does need them.
typedef char GLchar;
typedef GLuint (APIENTRYP PFNGLCREATESHADERPROC) (GLenum type);
typedef void (APIENTRYP PFNGLSHADERSOURCEPROC) (GLuint shader, GLsizei count, const GLchar* *string, const GLint *length);
typedef void (APIENTRYP PFNGLCOMPILESHADERPROC) (GLuint shader);
typedef GLuint (APIENTRYP PFNGLCREATEPROGRAMPROC) (void);
typedef void (APIENTRYP PFNGLATTACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (APIENTRYP PFNGLLINKPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLDELETESHADERPROC) (GLuint shader);
typedef void (APIENTRYP PFNGLDELETEPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLUSEPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLGETPROGRAMIVPROC) (GLuint program, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETSHADERIVPROC) (GLuint shader, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETPROGRAMINFOLOGPROC) (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRYP PFNGLGETSHADERINFOLOGPROC) (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRYP PFNGLGETATTACHEDSHADERSPROC) (GLuint shader, GLsizei maxCount, GLsizei *count, GLuint* shaders);

typedef GLint (APIENTRYP PFNGLGETUNIFORMLOCATIONPROC) (GLuint program, const GLchar *name);
typedef GLint (APIENTRYP PFNGLGETATTRIBLOCATIONPROC) (GLuint program, const GLchar *name);
typedef GLint (APIENTRYP PFNGLBINDATTRIBLOCATIONPROC) (GLuint program, GLuint loc, const GLchar *name);
typedef GLint (APIENTRYP PFNGLBINDFRAGDATALOCATIONPROC) (GLuint program, GLuint loc, const GLchar *name);

typedef void (APIENTRYP PFNGLUNIFORM1IVPROC) (GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRYP PFNGLUNIFORM2IVPROC) (GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRYP PFNGLUNIFORM3IVPROC) (GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRYP PFNGLUNIFORM4IVPROC) (GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRYP PFNGLUNIFORM1FVPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRYP PFNGLUNIFORM2FVPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRYP PFNGLUNIFORM3FVPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRYP PFNGLUNIFORM4FVPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRYP PFNGLUNIFORMMATRIX2FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRYP PFNGLUNIFORMMATRIX3FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRYP PFNGLUNIFORMMATRIX4FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRYP PFNGLUNIFORMMATRIX3x4FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);

typedef void (APIENTRYP PFNGLENABLEVERTEXATTRIBARRAYPROC) (GLuint index);
typedef void (APIENTRYP PFNGLDISABLEVERTEXATTRIBARRAYPROC) (GLuint index);
typedef void (APIENTRYP PFNGLVERTEXATTRIBPOINTERPROC) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);

typedef void (APIENTRYP PFNGLBINDRENDERBUFFERPROC) (GLenum target, GLuint renderbuffer);
typedef void (APIENTRYP PFNGLDELETERENDERBUFFERSPROC) (GLsizei n, const GLuint *renderbuffers);
typedef void (APIENTRYP PFNGLGENRENDERBUFFERSPROC) (GLsizei n, GLuint *renderbuffers);
typedef void (APIENTRYP PFNGLRENDERBUFFERSTORAGEPROC) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (APIENTRYP PFNGLBINDFRAMEBUFFERPROC) (GLenum target, GLuint framebuffer);
typedef void (APIENTRYP PFNGLDELETEFRAMEBUFFERSPROC) (GLsizei n, const GLuint *framebuffers);
typedef void (APIENTRYP PFNGLGENFRAMEBUFFERSPROC) (GLsizei n, GLuint *framebuffers);
typedef GLenum (APIENTRYP PFNGLCHECKFRAMEBUFFERSTATUSPROC) (GLenum target);
typedef void (APIENTRYP PFNGLFRAMEBUFFERRENDERBUFFERPROC) (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
typedef void (APIENTRYP PFNGLDRAWBUFFERSPROC)(GLsizei n, const GLenum* bufs);

// #ifndef GL_VERSION_3_0
typedef ptrdiff_t GLsizeiptr;
typedef void (APIENTRYP PFNGLFRAMEBUFFERTEXTURE2DPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (APIENTRYP PFNGLGENERATEMIPMAPPROC) (GLenum target);
typedef void (*PFNGLBINDBUFFERPROC) (GLenum target, GLuint buffer);
typedef void (*PFNGLDELETEBUFFERSPROC) (GLsizei n, const GLuint *buffers);
typedef void (*PFNGLGENBUFFERSPROC) (GLsizei n, GLuint *buffers);
typedef void (*PFNGLBUFFERDATAPROC) (GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);

*/

PFNGLBINDBUFFERPROC     glBindBuffer    = 0;
PFNGLDELETEBUFFERSPROC  glDeleteBuffers = 0;
PFNGLGENBUFFERSPROC     glGenBuffers    = 0;
PFNGLBUFFERDATAPROC     glBufferData    = 0;

PFNGLCREATESHADERPROC   glCreateShader  = 0;
PFNGLSHADERSOURCEPROC   glShaderSource  = 0;
PFNGLCOMPILESHADERPROC  glCompileShader = 0;
PFNGLCREATEPROGRAMPROC  glCreateProgram = 0;
PFNGLATTACHSHADERPROC   glAttachShader  = 0;
PFNGLLINKPROGRAMPROC    glLinkProgram   = 0;
PFNGLDELETESHADERPROC   glDeleteShader  = 0;
PFNGLDELETEPROGRAMPROC  glDeleteProgram = 0;
PFNGLUSEPROGRAMPROC     glUseProgram    = 0;
PFNGLGETPROGRAMIVPROC   glGetProgramiv  = 0;
PFNGLGETSHADERIVPROC    glGetShaderiv   = 0;
PFNGLISPROGRAMPROC      glIsProgram     = 0;

PFNGLGETPROGRAMINFOLOGPROC    glGetProgramInfoLog    = 0;
PFNGLGETSHADERINFOLOGPROC     glGetShaderInfoLog     = 0;
PFNGLGETATTACHEDSHADERSPROC   glGetAttachedShaders   = 0;
PFNGLGETUNIFORMLOCATIONPROC   glGetUniformLocation   = 0;
PFNGLGETATTRIBLOCATIONPROC    glGetAttribLocation    = 0;
PFNGLBINDFRAGDATALOCATIONPROC glBindFragDataLocation = 0;
PFNGLBINDATTRIBLOCATIONPROC   glBindAttribLocation   = 0;

PFNGLUNIFORM1IVPROC         glUniform1iv = 0;
PFNGLUNIFORM2IVPROC         glUniform2iv = 0;
PFNGLUNIFORM3IVPROC         glUniform3iv = 0;
PFNGLUNIFORM4IVPROC         glUniform4iv = 0;
PFNGLUNIFORM1FVPROC         glUniform1fv = 0;
PFNGLUNIFORM2FVPROC         glUniform2fv = 0;
PFNGLUNIFORM3FVPROC         glUniform3fv = 0;
PFNGLUNIFORM4FVPROC         glUniform4fv = 0;
PFNGLUNIFORMMATRIX2FVPROC   glUniformMatrix2fv   = 0;
PFNGLUNIFORMMATRIX3FVPROC   glUniformMatrix3fv   = 0;
PFNGLUNIFORMMATRIX4FVPROC   glUniformMatrix4fv   = 0;
PFNGLUNIFORMMATRIX3X4FVPROC glUniformMatrix3x4fv = 0;
PFNGLGETACTIVEUNIFORMPROC   glGetActiveUniform   = 0;

PFNGLENABLEVERTEXATTRIBARRAYPROC  glEnableVertexAttribArray  = 0;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray = 0;
PFNGLVERTEXATTRIBPOINTERPROC      glVertexAttribPointer      = 0;
PFNGLVERTEXATTRIBIPOINTERPROC     glVertexAttribIPointer     = 0;
PFNGLVERTEXATTRIBDIVISORPROC      glVertexAttribDivisor      = 0;

PFNGLGENVERTEXARRAYSPROC       glGenVertexArrays       = 0;
PFNGLBINDVERTEXARRAYPROC       glBindVertexArray       = 0;
PFNGLDELETEVERTEXARRAYSPROC    glDeleteVertexArrays    = 0;
PFNGLDRAWELEMENTSINSTANCEDPROC glDrawElementsInstanced = 0;
PFNGLDRAWARRAYSINSTANCEDPROC   glDrawArraysInstanced   = 0;

PFNGLBINDRENDERBUFFERPROC        glBindRenderbuffer        = 0;
PFNGLDELETERENDERBUFFERSPROC     glDeleteRenderbuffers     = 0;
PFNGLGENRENDERBUFFERSPROC        glGenRenderbuffers        = 0;
PFNGLRENDERBUFFERSTORAGEPROC     glRenderbufferStorage     = 0;
PFNGLBINDFRAMEBUFFERPROC         glBindFramebuffer         = 0;
PFNGLDELETEFRAMEBUFFERSPROC      glDeleteFramebuffers      = 0;
PFNGLGENFRAMEBUFFERSPROC         glGenFramebuffers         = 0;
PFNGLCHECKFRAMEBUFFERSTATUSPROC  glCheckFramebufferStatus  = 0;
PFNGLFRAMEBUFFERTEXTURE2DPROC    glFramebufferTexture2D   = 0;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer = 0;
PFNGLDRAWBUFFERSPROC             glDrawBuffers             = 0;


int initialiseOpenGLExtensions() {
	if(glCreateShader) return 1;
	printf("SETUP OPENGL EXTENSIONS\n");

	glBindBuffer    = (PFNGLBINDBUFFERPROC)    wglGetProcAddress("glBindBuffer");
	glDeleteBuffers = (PFNGLDELETEBUFFERSPROC) wglGetProcAddress("glDeleteBuffers");
	glGenBuffers    = (PFNGLGENBUFFERSPROC)    wglGetProcAddress("glGenBuffers");
	glBufferData    = (PFNGLBUFFERDATAPROC)    wglGetProcAddress("glBufferData");

	glActiveTexture        = (PFNGLACTIVETEXTUREARBPROC)wglGetProcAddress("glActiveTextureARB");
	glTexImage3D           = (PFNGLTEXIMAGE3DPROC)wglGetProcAddress("glTexImage3D");
	glTexSubImage3D        = (PFNGLTEXSUBIMAGE3DPROC)wglGetProcAddress("glTexSubImage3D");
	glCompressedTexImage1D = (PFNGLCOMPRESSEDTEXIMAGE1DPROC)wglGetProcAddress("glCompressedTexImage1D");
	glCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)wglGetProcAddress("glCompressedTexImage2D");
	glCompressedTexImage3D = (PFNGLCOMPRESSEDTEXIMAGE3DPROC)wglGetProcAddress("glCompressedTexImage3D");
	glCompressedTexSubImage1D = (PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC)wglGetProcAddress("glCompressedTexSubImage1D");
	glCompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)wglGetProcAddress("glCompressedTexSubImage2D");
	glCompressedTexSubImage3D = (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)wglGetProcAddress("glCompressedTexSubImage3D");
	glGenerateMipmap       = (PFNGLGENERATEMIPMAPPROC)wglGetProcAddress("glGenerateMipmap");
	glBlendFuncSeparate    = (PFNGLBLENDFUNCSEPARATEPROC)wglGetProcAddress("glBlendFuncSeparate");


	glCreateShader       = (PFNGLCREATESHADERPROC)   wglGetProcAddress("glCreateShader");
	glShaderSource       = (PFNGLSHADERSOURCEPROC)   wglGetProcAddress("glShaderSource");
	glCompileShader      = (PFNGLCOMPILESHADERPROC)  wglGetProcAddress("glCompileShader");
	glCreateProgram      = (PFNGLCREATEPROGRAMPROC)  wglGetProcAddress("glCreateProgram");
	glAttachShader       = (PFNGLATTACHSHADERPROC)   wglGetProcAddress("glAttachShader");
	glLinkProgram        = (PFNGLLINKPROGRAMPROC)    wglGetProcAddress("glLinkProgram");
	glDeleteShader       = (PFNGLDELETESHADERPROC)   wglGetProcAddress("glDeleteShader");
	glDeleteProgram      = (PFNGLDELETEPROGRAMPROC)  wglGetProcAddress("glDeleteProgram");
	glUseProgram         = (PFNGLUSEPROGRAMPROC)     wglGetProcAddress("glUseProgram");
	glGetProgramiv       = (PFNGLGETPROGRAMIVPROC)   wglGetProcAddress("glGetProgramiv");
	glGetShaderiv        = (PFNGLGETSHADERIVPROC)    wglGetProcAddress("glGetShaderiv");
	glIsProgram          = (PFNGLISPROGRAMPROC)      wglGetProcAddress("glIsProgram");
	glGetProgramInfoLog  = (PFNGLGETPROGRAMINFOLOGPROC)  wglGetProcAddress("glGetProgramInfoLog");
	glGetShaderInfoLog   = (PFNGLGETSHADERINFOLOGPROC)   wglGetProcAddress("glGetShaderInfoLog");
	glGetAttachedShaders = (PFNGLGETATTACHEDSHADERSPROC) wglGetProcAddress("glGetAttachedShaders");
	glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC) wglGetProcAddress("glGetUniformLocation");
	glGetAttribLocation  = (PFNGLGETATTRIBLOCATIONPROC)  wglGetProcAddress("glGetAttribLocation");
	glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC) wglGetProcAddress("glBindAttribLocation");
	glBindFragDataLocation = (PFNGLBINDFRAGDATALOCATIONPROC) wglGetProcAddress("glBindFragDataLocation");
	
	glUniform1iv = (PFNGLUNIFORM1IVPROC) wglGetProcAddress("glUniform1iv");
	glUniform2iv = (PFNGLUNIFORM2IVPROC) wglGetProcAddress("glUniform2iv");
	glUniform3iv = (PFNGLUNIFORM3IVPROC) wglGetProcAddress("glUniform3iv");
	glUniform4iv = (PFNGLUNIFORM4IVPROC) wglGetProcAddress("glUniform4iv");
	glUniform1fv = (PFNGLUNIFORM1FVPROC) wglGetProcAddress("glUniform1fv");
	glUniform2fv = (PFNGLUNIFORM2FVPROC) wglGetProcAddress("glUniform2fv");
	glUniform3fv = (PFNGLUNIFORM3FVPROC) wglGetProcAddress("glUniform3fv");
	glUniform4fv = (PFNGLUNIFORM4FVPROC) wglGetProcAddress("glUniform4fv");

	glUniformMatrix2fv   = (PFNGLUNIFORMMATRIX2FVPROC)   wglGetProcAddress("glUniformMatrix2fv");
	glUniformMatrix3fv   = (PFNGLUNIFORMMATRIX3FVPROC)   wglGetProcAddress("glUniformMatrix3fv");
	glUniformMatrix4fv   = (PFNGLUNIFORMMATRIX4FVPROC)   wglGetProcAddress("glUniformMatrix4fv");
	glUniformMatrix3x4fv = (PFNGLUNIFORMMATRIX3X4FVPROC) wglGetProcAddress("glUniformMatrix3x4fv");
	glGetActiveUniform   = (PFNGLGETACTIVEUNIFORMPROC)   wglGetProcAddress("glGetActiveUniform");
	
	glEnableVertexAttribArray  = (PFNGLENABLEVERTEXATTRIBARRAYPROC)  wglGetProcAddress("glEnableVertexAttribArray");
	glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC) wglGetProcAddress("glDisableVertexAttribArray");
	glVertexAttribPointer      = (PFNGLVERTEXATTRIBPOINTERPROC)      wglGetProcAddress("glVertexAttribPointer");
	glVertexAttribIPointer     = (PFNGLVERTEXATTRIBIPOINTERPROC)     wglGetProcAddress("glVertexAttribIPointer");
	glVertexAttribDivisor      = (PFNGLVERTEXATTRIBDIVISORPROC)      wglGetProcAddress("glVertexAttribDivisor");

	glGenVertexArrays       = (PFNGLGENVERTEXARRAYSPROC)       wglGetProcAddress("glGenVertexArrays");
	glBindVertexArray       = (PFNGLBINDVERTEXARRAYPROC)       wglGetProcAddress("glBindVertexArray");
	glDeleteVertexArrays    = (PFNGLDELETEVERTEXARRAYSPROC)    wglGetProcAddress("glDeleteVertexArrays");
	glDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDPROC) wglGetProcAddress("glDrawElementsInstanced");
	glDrawArraysInstanced   = (PFNGLDRAWARRAYSINSTANCEDPROC)   wglGetProcAddress("glDrawArraysInstanced");


	glBindRenderbuffer        = (PFNGLBINDRENDERBUFFERPROC)        wglGetProcAddress("glBindRenderbuffer");
	glDeleteRenderbuffers     = (PFNGLDELETERENDERBUFFERSPROC)     wglGetProcAddress("glDeleteRenderbuffers");
	glGenRenderbuffers        = (PFNGLGENRENDERBUFFERSPROC)        wglGetProcAddress("glGenRenderbuffers");
	glRenderbufferStorage     = (PFNGLRENDERBUFFERSTORAGEPROC)     wglGetProcAddress("glRenderbufferStorage");
	glBindFramebuffer         = (PFNGLBINDFRAMEBUFFERPROC)         wglGetProcAddress("glBindFramebuffer");
	glDeleteFramebuffers      = (PFNGLDELETEFRAMEBUFFERSPROC)      wglGetProcAddress("glDeleteFramebuffers");
	glGenFramebuffers         = (PFNGLGENFRAMEBUFFERSPROC)         wglGetProcAddress("glGenFramebuffers");
	glCheckFramebufferStatus  = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)  wglGetProcAddress("glCheckFramebufferStatus");
	glFramebufferTexture2D    = (PFNGLFRAMEBUFFERTEXTURE2DPROC)    wglGetProcAddress("glFramebufferTexture2D");
	glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC) wglGetProcAddress("glFramebufferRenderbuffer");
	glDrawBuffers             = (PFNGLDRAWBUFFERSPROC)             wglGetProcAddress("glDrawBuffers");


	return glCreateShader? 1: 0;
}
#else
int initialiseShaderExtensions() {
	return 1;
}
#endif

