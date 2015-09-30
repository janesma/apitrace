// Copyright (C) Intel Corp.  2015.  All Rights Reserved.

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice (including the
// next paragraph) shall be included in all copies or substantial
// portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

//  **********************************************************************/
//  * Authors:
//  *   Mark Janes <mark.a.janes@intel.com>
//  **********************************************************************/

#include "glframe_glhelper.hpp"

#include <assert.h>
#include <dlfcn.h>

using glretrace::GlFunctions;

bool GlFunctions::m_is_initialized = false;

namespace {
static void *pCreateProgram = NULL;
static void *pCreateShader = NULL;
static void *pShaderSource = NULL;
static void *pCompileShader = NULL;
static void *pAttachShader = NULL;
static void *pLinkProgram = NULL;
static void *pUseProgram = NULL;
static void *pGetError = NULL;
static void *pGetShaderiv = NULL;
static void *pGetShaderInfoLog = NULL;
static void *pGenBuffers = NULL;
static void *pBindBuffer = NULL;
static void *pGetAttribLocation = NULL;
static void *pGetUniformLocation = NULL;
static void *pClearColor = NULL;
static void *pClear = NULL;
static void *pUniform1f = NULL;
static void *pUniform4f = NULL;
static void *pBufferData = NULL;
static void *pEnableVertexAttribArray = NULL;
static void *pVertexAttribPointer = NULL;
static void *pDrawArrays = NULL;
static void *pDisableVertexAttribArray = NULL;
static void *pEnable = NULL;
static void *pReadPixels = NULL;
static void *pDrawElements = NULL;
}  // namespace

void
GlFunctions::Init(void *lookup_fn) {
  if (m_is_initialized)
    return;

  if (!lookup_fn) {
    void *lib_handle = NULL;
    lib_handle = dlopen("libGL.so", RTLD_LAZY | RTLD_GLOBAL);
    lookup_fn = dlsym(lib_handle, "glXGetProcAddress");
    assert(lookup_fn);
  }
  typedef void* (* GETPROCADDRESS) (const char *procName);
  GETPROCADDRESS GetProcAddress = (GETPROCADDRESS)lookup_fn;
  pCreateProgram = GetProcAddress("glCreateProgram");
  assert(pCreateProgram);

  pCreateShader = GetProcAddress("glCreateShader");
  assert(pCreateShader);
  pShaderSource = GetProcAddress("glShaderSource");
  assert(pShaderSource);
  pCompileShader = GetProcAddress("glCompileShader");
  assert(pCompileShader);
  pAttachShader = GetProcAddress("glAttachShader");
  assert(pAttachShader);
  pLinkProgram = GetProcAddress("glLinkProgram");
  assert(pLinkProgram);
  pUseProgram = GetProcAddress("glUseProgram");
  assert(pUseProgram);
  pGetError = GetProcAddress("glGetError");
  assert(pGetError);
  pGetShaderiv = GetProcAddress("glGetShaderiv");
  assert(pGetShaderiv);
  pGetShaderInfoLog = GetProcAddress("glGetShaderInfoLog");
  assert(pGetShaderInfoLog);
  pGenBuffers = GetProcAddress("glGenBuffers");
  assert(pGenBuffers);
  pBindBuffer = GetProcAddress("glBindBuffer");
  assert(pBindBuffer);
  pGetAttribLocation = GetProcAddress("glGetAttribLocation");
  assert(pGetAttribLocation);
  pGetUniformLocation = GetProcAddress("glGetUniformLocation");
  assert(pGetUniformLocation);
  pClearColor = GetProcAddress("glClearColor");
  assert(pClearColor);
  pClear = GetProcAddress("glClear");
  assert(pClear);
  pUniform1f = GetProcAddress("glUniform1f");
  assert(pUniform1f);
  pUniform4f = GetProcAddress("glUniform4f");
  assert(pUniform4f);
  pBufferData = GetProcAddress("glBufferData");
  assert(pBufferData);
  pEnableVertexAttribArray = GetProcAddress("glEnableVertexAttribArray");
  assert(pEnableVertexAttribArray);
  pVertexAttribPointer = GetProcAddress("glVertexAttribPointer");
  assert(pVertexAttribPointer);
  pDrawArrays = GetProcAddress("glDrawArrays");
  assert(pDrawArrays);
  pDisableVertexAttribArray = GetProcAddress("glDisableVertexAttribArray");
  assert(pDisableVertexAttribArray);
  pEnable = GetProcAddress("glEnable");
  assert(pEnable);
  pReadPixels = GetProcAddress("glReadPixels");
  assert(pReadPixels);
  pDrawElements = GetProcAddress("glDrawElements");
  assert(pDrawElements);
}

GLuint
GlFunctions::CreateProgram(void) {
  typedef GLuint(*CREATEPROGRAM)();
  return ((CREATEPROGRAM)pCreateProgram)();
}

GLuint
GlFunctions::CreateShader(GLenum type) {
  typedef GLuint (*CREATESHADER)(GLenum type);
  return ((CREATESHADER)pCreateShader)(type);
}

void
GlFunctions::ShaderSource(GLuint shader, GLsizei count,
                          const GLchar *const*str,
                          const GLint *length) {
  typedef void (*SHADERSOURCE)(GLuint shader, GLsizei count,
                               const GLchar *const*str,
                               const GLint *length);
  ((SHADERSOURCE)pShaderSource)(shader, count, str, length);
}

void
GlFunctions::CompileShader(GLuint shader) {
  typedef void (*COMPILESHADER)(GLuint shader);
  ((COMPILESHADER)pCompileShader)(shader);
}

void
GlFunctions::AttachShader(GLuint program, GLuint shader) {
  typedef void (*ATTACHSHADER)(GLuint program, GLuint shader);
  ((ATTACHSHADER)pAttachShader)(program, shader);
}

void
GlFunctions::LinkProgram(GLuint program) {
  typedef void (*LINKPROGRAM)(GLuint program);
  ((LINKPROGRAM)pLinkProgram)(program);
}

void
GlFunctions::UseProgram(GLuint program) {
  typedef void (*USEPROGRAM)(GLuint program);
  ((USEPROGRAM)pUseProgram)(program);
}

GLenum
GlFunctions::GetError() {
  typedef GLenum (*GETERROR)();
  return ((GETERROR)pGetError)();
}

void
GlFunctions::GetShaderiv(GLuint shader, GLenum pname, GLint *params) {
  typedef void (*GETSHADERIV)(GLuint shader, GLenum pname, GLint *params);
  ((GETSHADERIV)pGetShaderiv)(shader, pname, params);
}

void
GlFunctions::GetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length,
                              GLchar *infoLog) {
  typedef void (*GETSHADERINFOLOG)(GLuint shader, GLsizei bufSize,
                                   GLsizei *length, GLchar *infoLog);
  ((GETSHADERINFOLOG)pGetShaderInfoLog)(shader, bufSize,
                                        length, infoLog);
}

void
GlFunctions::GenBuffers(GLsizei n, GLuint *buffers) {
  typedef void (*GENBUFFERS)(GLsizei n, GLuint *buffers);
  ((GENBUFFERS) pGenBuffers)(n, buffers);
}

void
GlFunctions::BindBuffer(GLenum target, GLuint buffer) {
  typedef void (*BINDBUFFER)(GLenum target, GLuint buffer);
  ((BINDBUFFER)pBindBuffer)(target, buffer);
}

GLint
GlFunctions::GetAttribLocation(GLuint program, const GLchar *name) {
  typedef GLint (*GETATTRIBLOCATION)(GLuint program, const GLchar *name);
  return ((GETATTRIBLOCATION)pGetAttribLocation)(program, name);
}

GLint
GlFunctions::GetUniformLocation(GLuint program, const GLchar *name) {
  typedef GLint (*GETUNIFORMLOCATION)(GLuint program, const GLchar *name);
  return ((GETUNIFORMLOCATION)pGetUniformLocation)(program, name);
}

void
GlFunctions::ClearColor(GLclampf red, GLclampf green, GLclampf blue,
                        GLclampf alpha) {
  typedef void (*CLEARCOLOR)(GLclampf red, GLclampf green, GLclampf blue,
                             GLclampf alpha);
  ((CLEARCOLOR)pClearColor)(red, green, blue, alpha);
}

void
GlFunctions::Clear(GLbitfield mask) {
  typedef void(*CLEAR)(GLbitfield mask);
  ((CLEAR)pClear)(mask);
}


void
GlFunctions::Uniform1f(GLint location, GLfloat v0) {
  typedef void (*UNIFORM1F)(GLint location, GLfloat v0);
  ((UNIFORM1F)pUniform1f)(location, v0);
}

void
GlFunctions::Uniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2,
                       GLfloat v3) {
  typedef void (*UNIFORM4F)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2,
                            GLfloat v3);
  ((UNIFORM4F)pUniform4f)(location, v0, v1, v2, v3);
}

void
GlFunctions::BufferData(GLenum target, GLsizeiptr size, const void *data,
                         GLenum usage) {
  typedef void (*BUFFERDATA) (GLenum target, GLsizeiptr size, const void *data,
                              GLenum usage);
  ((BUFFERDATA)pBufferData) (target, size, data, usage);
}

void
GlFunctions::EnableVertexAttribArray(GLuint index) {
  typedef void (*ENABLEVERTEXATTRIBARRAY)(GLuint index);
  ((ENABLEVERTEXATTRIBARRAY)pEnableVertexAttribArray)(index);
}

void
GlFunctions::VertexAttribPointer(GLuint index, GLint size, GLenum type,
                                 GLboolean normalized, GLsizei stride,
                                 const void *pointer) {
  typedef void (*VERTEXATTRIBPOINTER)(GLuint index, GLint size, GLenum type,
                                      GLboolean normalized, GLsizei stride,
                                      const void *pointer);
  ((VERTEXATTRIBPOINTER)pVertexAttribPointer)(index, size,
                                             type, normalized,
                                             stride, pointer);
}

void
GlFunctions::DrawArrays(GLenum mode, GLint first, GLsizei count) {
  typedef void (*DRAWARRAYS)(GLenum mode, GLint first, GLsizei count);
  ((DRAWARRAYS)pDrawArrays)(mode, first, count);
}

void
GlFunctions::DisableVertexAttribArray(GLuint index) {
  typedef void (*DISABLEVERTEXATTRIBARRAY)(GLuint index);
  ((DISABLEVERTEXATTRIBARRAY)pDisableVertexAttribArray)(index);
}

void
GlFunctions::Enable(GLenum cap) {
  typedef void (*ENABLE)(GLenum cap);
  ((ENABLE)pEnable)(cap);
}


void
GlFunctions::ReadPixels(GLint x, GLint y,
                          GLsizei width, GLsizei height,
                          GLenum format, GLenum type,
                          GLvoid *pixels) {
  typedef void (*READPIXELS)(GLint x, GLint y,
                             GLsizei width, GLsizei height,
                             GLenum format, GLenum type,
                             GLvoid *pixels);
  ((READPIXELS)pReadPixels)(x, y, width, height, format, type, pixels);
}

void
GlFunctions::DrawElements(GLenum mode, GLsizei count,
                          GLenum type, const GLvoid *indices) {
  typedef void (*DRAWELEMENTS)(GLenum mode, GLsizei count,
                               GLenum type, const GLvoid *indices);
  ((DRAWELEMENTS) pDrawElements)(mode, count, type, indices);
}
