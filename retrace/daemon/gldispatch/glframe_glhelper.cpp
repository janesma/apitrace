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
#ifdef WIN32
#include "retrace.hpp"
#include "glretrace.hpp"
#include "glws.hpp"
#else
#include <dlfcn.h>
#endif

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
static void *pBlendFunc = NULL;
static void *pGetFirstPerfQueryIdINTEL = NULL;
static void *pGetNextPerfQueryIdINTEL = NULL;
static void *pGetIntegerv = NULL;
static void *pGetStringi = NULL;
static void *pGetPerfQueryInfoINTEL = NULL;
static void *pGetPerfCounterInfoINTEL = NULL;
static void *pCreatePerfQueryINTEL = NULL;
static void *pDeletePerfQueryINTEL = NULL;
static void *pBeginPerfQueryINTEL = NULL;
static void *pEndPerfQueryINTEL = NULL;
static void *pGetPerfQueryDataINTEL = NULL;
static void *pGetProgramiv = NULL;
static void *pGetProgramInfoLog = NULL;
static void *pGetBooleanv = NULL;
static void *pGetFloatv = NULL;
static void *pBlendColor = NULL;
static void *pBlendFuncSeparate = NULL;
static void *pDisable = NULL;
static void *pBlendEquation = NULL;
static void *pBlendEquationSeparate = NULL;
static void *pBindAttribLocation = NULL;
static void *pValidateProgram = NULL;
static void *pIsEnabled = NULL;
static void *pGetUniformBlockIndex = NULL;
static void *pUniformBlockBinding = NULL;
static void *pBindFragDataLocation = NULL;
static void *pGetActiveUniform = NULL;
static void *pGetUniformfv = NULL;
static void *pGetUniformiv = NULL;
static void *pUniform1fv = NULL;
static void *pUniform1iv = NULL;
static void *pUniform1uiv = NULL;
static void *pUniform2fv = NULL;
static void *pUniform2iv = NULL;
static void *pUniform2uiv = NULL;
static void *pUniform3fv = NULL;
static void *pUniform3iv = NULL;
static void *pUniform3uiv = NULL;
static void *pUniform4fv = NULL;
static void *pUniform4iv = NULL;
static void *pUniform4uiv = NULL;
static void *pUniformMatrix2fv = NULL;
static void *pUniformMatrix2x3fv = NULL;
static void *pUniformMatrix2x4fv = NULL;
static void *pUniformMatrix3fv = NULL;
static void *pUniformMatrix3x2fv = NULL;
static void *pUniformMatrix3x4fv = NULL;
static void *pUniformMatrix4fv = NULL;
static void *pUniformMatrix4x2fv = NULL;
static void *pUniformMatrix4x3fv = NULL;
static void *pFinish = NULL;

}  // namespace

static void * _GetProcAddress(const char *name) {
  typedef void* (* GETPROCADDRESS) (const char *procName);
  static GETPROCADDRESS lookup_fn = NULL;
#ifdef WIN32
  static HMODULE lib_handle = NULL;
  if (!lib_handle) {
    char szDll[MAX_PATH] = {0};
    if (!GetSystemDirectoryA(szDll, MAX_PATH)) {
      return NULL;
    }
    strncat(szDll, "\\opengl32.dll", MAX_PATH - 1);
    lib_handle = LoadLibraryA(szDll);
  }
  if (!lookup_fn) {
    lookup_fn = (GETPROCADDRESS) GetProcAddress(lib_handle,
                                                "wglGetProcAddress");
  }
#else
  static void *lib_handle = NULL;
  lib_handle = dlopen("libGL.so", RTLD_LAZY | RTLD_GLOBAL);
  lookup_fn = reinterpret_cast<GETPROCADDRESS>(dlsym(lib_handle,
                                                     "glXGetProcAddress"));
#endif
  assert(lookup_fn);
  void* ret = lookup_fn(name);
#ifdef WIN32
  if (ret == NULL)
    ret = GetProcAddress(lib_handle, name);
  if (ret == NULL)
    ret = wglGetProcAddress(name);
#endif
  assert(ret);
  return ret;
}
void
GlFunctions::Init(void *lookup_fn) {
  if (m_is_initialized)
    return;

#ifdef WIN32
  // GetProcAddress will return null on windows unless there is an
  // active GL context.
  retrace::setUp();
  glws::Drawable *d = glretrace::createDrawable();
  glretrace::Context *c = glretrace::createContext();
  glws::makeCurrent(d, c->wsContext);
#endif

  pCreateProgram = _GetProcAddress("glCreateProgram");
  assert(pCreateProgram);

  pCreateShader = _GetProcAddress("glCreateShader");
  assert(pCreateShader);
  pShaderSource = _GetProcAddress("glShaderSource");
  assert(pShaderSource);
  pCompileShader = _GetProcAddress("glCompileShader");
  assert(pCompileShader);
  pAttachShader = _GetProcAddress("glAttachShader");
  assert(pAttachShader);
  pLinkProgram = _GetProcAddress("glLinkProgram");
  assert(pLinkProgram);
  pUseProgram = _GetProcAddress("glUseProgram");
  assert(pUseProgram);
  pGetError = _GetProcAddress("glGetError");
  assert(pGetError);
  pGetShaderiv = _GetProcAddress("glGetShaderiv");
  assert(pGetShaderiv);
  pGetShaderInfoLog = _GetProcAddress("glGetShaderInfoLog");
  assert(pGetShaderInfoLog);
  pGenBuffers = _GetProcAddress("glGenBuffers");
  assert(pGenBuffers);
  pBindBuffer = _GetProcAddress("glBindBuffer");
  assert(pBindBuffer);
  pGetAttribLocation = _GetProcAddress("glGetAttribLocation");
  assert(pGetAttribLocation);
  pGetUniformLocation = _GetProcAddress("glGetUniformLocation");
  assert(pGetUniformLocation);
  pClearColor = _GetProcAddress("glClearColor");
  assert(pClearColor);
  pClear = _GetProcAddress("glClear");
  assert(pClear);
  pUniform1f = _GetProcAddress("glUniform1f");
  assert(pUniform1f);
  pUniform4f = _GetProcAddress("glUniform4f");
  assert(pUniform4f);
  pBufferData = _GetProcAddress("glBufferData");
  assert(pBufferData);
  pEnableVertexAttribArray = _GetProcAddress("glEnableVertexAttribArray");
  assert(pEnableVertexAttribArray);
  pVertexAttribPointer = _GetProcAddress("glVertexAttribPointer");
  assert(pVertexAttribPointer);
  pDrawArrays = _GetProcAddress("glDrawArrays");
  assert(pDrawArrays);
  pDisableVertexAttribArray = _GetProcAddress("glDisableVertexAttribArray");
  assert(pDisableVertexAttribArray);
  pEnable = _GetProcAddress("glEnable");
  assert(pEnable);
  pReadPixels = _GetProcAddress("glReadPixels");
  assert(pReadPixels);
  pDrawElements = _GetProcAddress("glDrawElements");
  assert(pDrawElements);
  pBlendFunc = _GetProcAddress("glBlendFunc");
  assert(pBlendFunc);
  pGetFirstPerfQueryIdINTEL = _GetProcAddress("glGetFirstPerfQueryIdINTEL");
  assert(pGetFirstPerfQueryIdINTEL);
  pGetNextPerfQueryIdINTEL = _GetProcAddress("glGetNextPerfQueryIdINTEL");
  assert(pGetNextPerfQueryIdINTEL);
  pGetIntegerv = _GetProcAddress("glGetIntegerv");
  assert(pGetIntegerv);
  pGetStringi = _GetProcAddress("glGetStringi");
  assert(pGetStringi);
  pGetPerfQueryInfoINTEL = _GetProcAddress("glGetPerfQueryInfoINTEL");
  assert(pGetPerfQueryInfoINTEL);
  pGetPerfCounterInfoINTEL = _GetProcAddress("glGetPerfCounterInfoINTEL");
  assert(pGetPerfCounterInfoINTEL);
  pCreatePerfQueryINTEL = _GetProcAddress("glCreatePerfQueryINTEL");
  assert(pCreatePerfQueryINTEL);
  pDeletePerfQueryINTEL = _GetProcAddress("glDeletePerfQueryINTEL");
  assert(pDeletePerfQueryINTEL);
  pBeginPerfQueryINTEL = _GetProcAddress("glBeginPerfQueryINTEL");
  assert(pBeginPerfQueryINTEL);
  pEndPerfQueryINTEL = _GetProcAddress("glEndPerfQueryINTEL");
  assert(pEndPerfQueryINTEL);
  pGetPerfQueryDataINTEL = _GetProcAddress("glGetPerfQueryDataINTEL");
  assert(pGetPerfQueryDataINTEL);
  pGetProgramiv = _GetProcAddress("glGetProgramiv");
  assert(pGetProgramiv);
  pGetProgramInfoLog = _GetProcAddress("glGetProgramInfoLog");
  assert(pGetProgramInfoLog);
  pGetBooleanv = _GetProcAddress("glGetBooleanv");
  assert(pGetBooleanv);
  pGetFloatv = _GetProcAddress("glGetFloatv");
  assert(pGetFloatv);
  pBlendColor = _GetProcAddress("glBlendColor");
  assert(pBlendColor);
  pBlendFuncSeparate = _GetProcAddress("glBlendFuncSeparate");
  assert(pBlendFuncSeparate);
  pDisable = _GetProcAddress("glDisable");
  assert(pDisable);
  pBlendEquation = _GetProcAddress("glBlendEquation");
  assert(pBlendEquation);
  pBlendEquationSeparate = _GetProcAddress("glBlendEquationSeparate");
  assert(pBlendEquationSeparate);
  pBindAttribLocation = _GetProcAddress("glBindAttribLocation");
  assert(pBindAttribLocation);
  pValidateProgram = _GetProcAddress("glValidateProgram");
  assert(pValidateProgram);
  pIsEnabled = _GetProcAddress("glIsEnabled");
  assert(pIsEnabled);
  pGetUniformBlockIndex = _GetProcAddress("glGetUniformBlockIndex");
  assert(pGetUniformBlockIndex);
  pUniformBlockBinding = _GetProcAddress("glUniformBlockBinding");
  assert(pUniformBlockBinding);
  pBindFragDataLocation = _GetProcAddress("glBindFragDataLocation");
  assert(pBindFragDataLocation);
  pGetActiveUniform = _GetProcAddress("glGetActiveUniform");
  assert(pGetActiveUniform);
  pGetUniformiv = _GetProcAddress("glGetUniformiv");
  assert(pGetUniformiv);
  pGetUniformfv = _GetProcAddress("glGetUniformfv");
  assert(pGetUniformfv);
  pUniform1fv = _GetProcAddress("glUniform1fv");
  assert(pUniform1fv);
  pUniform1iv = _GetProcAddress("glUniform1iv");
  assert(pUniform1iv);
  pUniform1uiv = _GetProcAddress("glUniform1uiv");
  assert(pUniform1uiv);
  pUniform2fv = _GetProcAddress("glUniform2fv");
  assert(pUniform2fv);
  pUniform2iv = _GetProcAddress("glUniform2iv");
  assert(pUniform2iv);
  pUniform2uiv = _GetProcAddress("glUniform2uiv");
  assert(pUniform2uiv);
  pUniform3fv = _GetProcAddress("glUniform3fv");
  assert(pUniform3fv);
  pUniform3iv = _GetProcAddress("glUniform3iv");
  assert(pUniform3iv);
  pUniform3uiv = _GetProcAddress("glUniform3uiv");
  assert(pUniform3uiv);
  pUniform4fv = _GetProcAddress("glUniform4fv");
  assert(pUniform4fv);
  pUniform4iv = _GetProcAddress("glUniform4iv");
  assert(pUniform4iv);
  pUniform4uiv = _GetProcAddress("glUniform4uiv");
  assert(pUniform4uiv);
  pUniformMatrix2fv = _GetProcAddress("glUniformMatrix2fv");
  assert(pUniformMatrix2fv);
  pUniformMatrix2x3fv = _GetProcAddress("glUniformMatrix2x3fv");
  assert(pUniformMatrix2x3fv);
  pUniformMatrix2x4fv = _GetProcAddress("glUniformMatrix2x4fv");
  assert(pUniformMatrix2x4fv);
  pUniformMatrix3fv = _GetProcAddress("glUniformMatrix3fv");
  assert(pUniformMatrix3fv);
  pUniformMatrix3x2fv = _GetProcAddress("glUniformMatrix3x2fv");
  assert(pUniformMatrix3x2fv);
  pUniformMatrix3x4fv = _GetProcAddress("glUniformMatrix3x4fv");
  assert(pUniformMatrix3x4fv);
  pUniformMatrix4fv = _GetProcAddress("glUniformMatrix4fv");
  assert(pUniformMatrix4fv);
  pUniformMatrix4x2fv = _GetProcAddress("glUniformMatrix4x2fv");
  assert(pUniformMatrix4x2fv);
  pUniformMatrix4x3fv = _GetProcAddress("glUniformMatrix4x3fv");
  assert(pUniformMatrix4x3fv);
  pFinish = _GetProcAddress("glFinish");
  assert(pFinish);
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

void
GlFunctions::BlendFunc(GLenum sfactor, GLenum dfactor) {
  typedef void (*BLENDFUNC)(GLenum sfactor, GLenum dfactor);
  ((BLENDFUNC) pBlendFunc)(sfactor, dfactor);
}

void
GlFunctions::GetFirstPerfQueryIdINTEL(GLuint *queryId) {
  if (!pGetFirstPerfQueryIdINTEL) {
    *queryId = -1;
    return;
  }
  typedef void (*GETFIRSTPERFQUERYIDINTEL)(GLuint *queryId);
  ((GETFIRSTPERFQUERYIDINTEL) pGetFirstPerfQueryIdINTEL)(queryId);
}

void
GlFunctions::GetNextPerfQueryIdINTEL(GLuint queryId, GLuint *nextQueryId) {
  typedef void (*GETNEXTPERFQUERYIDINTEL)(GLuint queryId, GLuint *nextQueryId);
  ((GETNEXTPERFQUERYIDINTEL) pGetNextPerfQueryIdINTEL)(queryId, nextQueryId);
}

void
GlFunctions::GetIntegerv(GLenum pname, GLint *params) {
  typedef void (*GETINTEGERV)(GLenum pname, GLint *params);
  ((GETINTEGERV)pGetIntegerv)(pname, params);
}

const GLubyte *
GlFunctions::GetStringi(GLenum pname, GLuint index) {
  typedef const GLubyte * (*GETSTRINGI)(GLenum pname, GLuint index);
  return ((GETSTRINGI)pGetStringi)(pname, index);
}

void GlFunctions::GetPerfQueryInfoINTEL(GLuint queryId, GLuint queryNameLength,
                                        GLchar *queryName, GLuint *dataSize,
                                        GLuint *noCounters, GLuint *noInstances,
                                        GLuint *capsMask) {
  typedef void (*GETPERFQUERYINFOINTEL)(GLuint queryId, GLuint queryNameLength,
                                        GLchar *queryName, GLuint *dataSize,
                                        GLuint *noCounters, GLuint *noInstances,
                                        GLuint *capsMask);
  ((GETPERFQUERYINFOINTEL)pGetPerfQueryInfoINTEL)(queryId, queryNameLength,
                                                  queryName, dataSize,
                                                  noCounters, noInstances,
                                                  capsMask);
}

void
GlFunctions::GetPerfCounterInfoINTEL(GLuint queryId, GLuint counterId,
                                     GLuint counterNameLength,
                                     GLchar *counterName,
                                     GLuint counterDescLength,
                                     GLchar *counterDesc,
                                     GLuint *counterOffset,
                                     GLuint *counterDataSize,
                                     GLuint *counterTypeEnum,
                                     GLuint *counterDataTypeEnum,
                                     GLuint64 *rawCounterMaxValue) {
  typedef void (*GETPERFCOUNTERINFOINTEL)(GLuint queryId, GLuint counterId,
                                          GLuint counterNameLength,
                                          GLchar *counterName,
                                          GLuint counterDescLength,
                                          GLchar *counterDesc,
                                          GLuint *counterOffset,
                                          GLuint *counterDataSize,
                                          GLuint *counterTypeEnum,
                                          GLuint *counterDataTypeEnum,
                                          GLuint64 *rawCounterMaxValue);
  ((GETPERFCOUNTERINFOINTEL)pGetPerfCounterInfoINTEL)(queryId,
                                                      counterId,
                                                      counterNameLength,
                                                      counterName,
                                                      counterDescLength,
                                                      counterDesc,
                                                      counterOffset,
                                                      counterDataSize,
                                                      counterTypeEnum,
                                                      counterDataTypeEnum,
                                                      rawCounterMaxValue);
}

void GlFunctions::CreatePerfQueryINTEL(GLuint queryId, GLuint *queryHandle) {
  typedef void (*CREATEPERFQUERYINTEL)(GLuint queryId, GLuint *queryHandle);
  ((CREATEPERFQUERYINTEL)pCreatePerfQueryINTEL)(queryId, queryHandle);
}

void GlFunctions::DeletePerfQueryINTEL(GLuint queryHandle) {
  typedef void (*DELETEPERFQUERYINTEL)(GLuint queryHandle);
  ((DELETEPERFQUERYINTEL)pDeletePerfQueryINTEL)(queryHandle);
}

void
GlFunctions::BeginPerfQueryINTEL(GLuint queryHandle) {
  typedef void (*BEGINPERFQUERYINTEL)(GLuint queryHandle);
  ((BEGINPERFQUERYINTEL) pBeginPerfQueryINTEL)(queryHandle);
}

void
GlFunctions::EndPerfQueryINTEL(GLuint queryHandle) {
  typedef void (*ENDPERFQUERYINTEL)(GLuint queryHandle);
  ((ENDPERFQUERYINTEL) pEndPerfQueryINTEL)(queryHandle);
}

void
GlFunctions::GetPerfQueryDataINTEL(GLuint queryHandle, GLuint flags,
                                   GLsizei dataSize, GLvoid *data,
                                   GLuint *bytesWritten) {
  typedef void (*GETPERFQUERYDATAINTEL)(GLuint queryHandle, GLuint flags,
                                        GLsizei dataSize, GLvoid *data,
                                        GLuint *bytesWritten);
  ((GETPERFQUERYDATAINTEL) pGetPerfQueryDataINTEL)(queryHandle, flags,
                                                   dataSize, data,
                                                   bytesWritten);
}

void
GlFunctions::GetProgramiv(GLuint program, GLenum pname, GLint *params) {
  typedef void (*GETPROGRAMIV)(GLuint program, GLenum pname, GLint *params);
  ((GETPROGRAMIV) pGetProgramiv)(program, pname, params);
}

void
GlFunctions::GetProgramInfoLog(GLuint program, GLsizei bufSize,
                               GLsizei *length, GLchar *infoLog) {
  typedef void (*GETPROGRAMINFOLOG)(GLuint program, GLsizei bufSize,
                                    GLsizei *length, GLchar *infoLog);
  ((GETPROGRAMINFOLOG) pGetProgramInfoLog)(program, bufSize,
                                           length, infoLog);
}

void
GlFunctions::GetBooleanv(GLenum pname, GLboolean *params) {
  typedef void (*GETBOOLEANV)(GLenum pname, GLboolean *params);
  ((GETBOOLEANV) pGetBooleanv)(pname, params);
}

void
GlFunctions::GetFloatv(GLenum pname, GLfloat *params) {
  typedef void (*GETFLOATV)(GLenum pname, GLfloat *params);
  ((GETFLOATV) pGetFloatv)(pname, params);
}

void
GlFunctions::BlendColor(GLclampf red, GLclampf green,
                        GLclampf blue, GLclampf alpha) {
  typedef void (*BLENDCOLOR)(GLclampf red, GLclampf green,
                             GLclampf blue, GLclampf alpha);
  ((BLENDCOLOR) pBlendColor)(red, green, blue, alpha);
}

void
GlFunctions::BlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB,
                               GLenum sfactorAlpha, GLenum dfactorAlpha) {
  typedef void (*BLENDFUNCSEPARATE)(GLenum sfactorRGB, GLenum dfactorRGB,
                                    GLenum sfactorAlpha, GLenum dfactorAlpha);
  ((BLENDFUNCSEPARATE) pBlendFuncSeparate)(sfactorRGB, dfactorRGB,
                                           sfactorAlpha, dfactorAlpha);
}

void
GlFunctions::Disable(GLenum cap) {
  typedef void (*DISABLE)(GLenum cap);
  ((DISABLE) pDisable)(cap);
}

void
GlFunctions::BlendEquation(GLenum mode) {
  typedef void (*BLENDEQUATION)(GLenum mode);
  ((BLENDEQUATION) pBlendEquation)(mode);
}

void
GlFunctions::BlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) {
  typedef void (*BLENDEQUATIONSEPARATE)(GLenum modeRGB, GLenum modeAlpha);
  ((BLENDEQUATIONSEPARATE) pBlendEquationSeparate)(modeRGB, modeAlpha);
}

void
GlFunctions::BindAttribLocation(GLuint program, GLuint index,
                                const GLchar *name) {
  typedef void (*BINDATTRIBLOCATION)(GLuint program, GLuint index,
                                     const GLchar *name);
  ((BINDATTRIBLOCATION) pBindAttribLocation)(program, index, name);
}

void
GlFunctions::ValidateProgram(GLuint program) {
  typedef void (*VALIDATEPROGRAM)(GLuint program);
  ((VALIDATEPROGRAM) pValidateProgram)(program);
}

GLboolean
GlFunctions::IsEnabled(GLenum cap) {
  typedef GLboolean (*ISENABLED)(GLenum cap);
  return ((ISENABLED) pIsEnabled)(cap);
}

GLuint
GlFunctions::GetUniformBlockIndex(GLuint program,
                                  const GLchar *uniformBlockName) {
  typedef GLuint (*GETUNIFORMBLOCKINDEX)(GLuint program,
                                         const GLchar *uniformBlockName);
  return ((GETUNIFORMBLOCKINDEX) pGetUniformBlockIndex)(program,
                                                        uniformBlockName);
}

void
GlFunctions::UniformBlockBinding(GLuint program, GLuint uniformBlockIndex,
                                 GLuint uniformBlockBinding) {
  typedef void (*UNIFORMBLOCKBINDING)(GLuint program, GLuint uniformBlockIndex,
                                      GLuint uniformBlockBinding);
  return ((UNIFORMBLOCKBINDING) pUniformBlockBinding)(program,
                                                      uniformBlockIndex,
                                                      uniformBlockBinding);
}

void
GlFunctions::BindFragDataLocation(GLuint program, GLuint colorNumber,
                                  const char * name) {
  typedef void (*BINDFRAGDATALOCATION)(GLuint program, GLuint colorNumber,
                                       const char * name);
  return ((BINDFRAGDATALOCATION) pBindFragDataLocation)(program,
                                                       colorNumber,
                                                       name);
}

void
GlFunctions::GetActiveUniform(GLuint program, GLuint index,
                              GLsizei bufSize, GLsizei *length,
                              GLint *size, GLenum *type,
                              GLchar *name) {
  typedef void (*GETACTIVEUNIFORM)(GLuint program, GLuint index,
                                   GLsizei bufSize, GLsizei *length,
                                   GLint *size, GLenum *type, GLchar *name);
  return ((GETACTIVEUNIFORM)pGetActiveUniform)(program, index, bufSize,
                                               length, size, type, name);
}

void
GlFunctions::GetUniformfv(GLuint program, GLint location, GLfloat *params) {
  typedef void (*GETUNIFORMFV)(GLuint program, GLint location, GLfloat *params);
  return ((GETUNIFORMFV) pGetUniformfv)(program, location, params);
}

void
GlFunctions::GetUniformiv(GLuint program, GLint location, GLint *params) {
  typedef void (*GETUNIFORMIV)(GLuint program, GLint location, GLint *params);
  return ((GETUNIFORMIV) pGetUniformiv)(program, location, params);
}

void
GlFunctions::Uniform1fv(GLint location, GLsizei count, const GLfloat *value) {
  typedef void (*UNIFORM1FV)(GLint location, GLsizei count,
                             const GLfloat *value);
  return ((UNIFORM1FV)pUniform1fv)(location, count, value);
}

void
GlFunctions::Uniform1iv(GLint location, GLsizei count, const GLint *value) {
  typedef void (*UNIFORM1IV)(GLint location, GLsizei count,
                             const GLint *value);
  return ((UNIFORM1IV)pUniform1iv)(location, count, value);
}

void
GlFunctions::Uniform1uiv(GLint location, GLsizei count, const GLuint *value) {
  typedef void (*UNIFORM1UIV)(GLint location, GLsizei count,
                              const GLuint *value);
  return ((UNIFORM1UIV)pUniform1uiv)(location, count, value);
}

void
GlFunctions::Uniform2fv(GLint location, GLsizei count, const GLfloat *value) {
  typedef void (*UNIFORM2FV)(GLint location, GLsizei count,
                             const GLfloat *value);
  return ((UNIFORM2FV)pUniform2fv)(location, count, value);
}

void
GlFunctions::Uniform2iv(GLint location, GLsizei count, const GLint *value) {
  typedef void (*UNIFORM2IV)(GLint location, GLsizei count, const GLint *value);
  return ((UNIFORM2IV)pUniform2iv)(location, count, value);
}

void
GlFunctions::Uniform2uiv(GLint location, GLsizei count, const GLuint *value) {
  typedef void (*UNIFORM2UIV)(GLint location, GLsizei count,
                              const GLuint *value);
  return ((UNIFORM2UIV)pUniform2uiv)(location, count, value);
}

void
GlFunctions::Uniform3fv(GLint location, GLsizei count, const GLfloat *value) {
  typedef void (*UNIFORM3FV)(GLint location, GLsizei count,
                             const GLfloat *value);
  return ((UNIFORM3FV)pUniform3fv)(location, count, value);
}

void
GlFunctions::Uniform3iv(GLint location, GLsizei count, const GLint *value) {
  typedef void (*UNIFORM3IV)(GLint location, GLsizei count, const GLint *value);
  return ((UNIFORM3IV)pUniform3iv)(location, count, value);
}

void
GlFunctions::Uniform3uiv(GLint location, GLsizei count, const GLuint *value) {
  typedef void (*UNIFORM3UIV)(GLint location, GLsizei count,
                              const GLuint *value);
  return ((UNIFORM3UIV)pUniform3uiv)(location, count, value);
}

void
GlFunctions::Uniform4fv(GLint location, GLsizei count, const GLfloat *value) {
  typedef void (*UNIFORM4FV)(GLint location, GLsizei count,
                             const GLfloat *value);
  return ((UNIFORM4FV)pUniform4fv)(location, count, value);
}

void
GlFunctions::Uniform4iv(GLint location, GLsizei count, const GLint *value) {
  typedef void (*UNIFORM4IV)(GLint location, GLsizei count, const GLint *value);
  return ((UNIFORM4IV)pUniform4iv)(location, count, value);
}

void
GlFunctions::Uniform4uiv(GLint location, GLsizei count, const GLuint *value) {
  typedef void (*UNIFORM4UIV)(GLint location, GLsizei count,
                              const GLuint *value);
  return ((UNIFORM4UIV)pUniform4uiv)(location, count, value);
}

void
GlFunctions::UniformMatrix2fv(GLint location, GLsizei count,
                              GLboolean transpose, const GLfloat *value) {
  typedef void (*UNIFORMMATRIX2FV)(GLint location, GLsizei count,
                                   GLboolean transpose, const GLfloat *value);
  return ((UNIFORMMATRIX2FV)pUniformMatrix2fv)(location, count, transpose,
                                               value);
}

void
GlFunctions::UniformMatrix2x3fv(GLint location, GLsizei count,
                                GLboolean transpose, const GLfloat *value) {
  typedef void (*UNIFORMMATRIX2X3FV)(GLint location, GLsizei count,
                                     GLboolean transpose, const GLfloat *value);
  return ((UNIFORMMATRIX2X3FV)pUniformMatrix2x3fv)(location, count, transpose,
                                                   value);
}

void
GlFunctions::UniformMatrix2x4fv(GLint location, GLsizei count,
                                GLboolean transpose, const GLfloat *value) {
  typedef void (*UNIFORMMATRIX2X4FV)(GLint location, GLsizei count,
                                     GLboolean transpose, const GLfloat *value);
  return ((UNIFORMMATRIX2X4FV)pUniformMatrix2x4fv)(location, count, transpose,
                                                   value);
}

void
GlFunctions::UniformMatrix3fv(GLint location, GLsizei count,
                              GLboolean transpose, const GLfloat *value) {
  typedef void (*UNIFORMMATRIX3FV)(GLint location, GLsizei count,
                                   GLboolean transpose, const GLfloat *value);
  return ((UNIFORMMATRIX3FV)pUniformMatrix3fv)(location, count, transpose,
                                               value);
}

void
GlFunctions::UniformMatrix3x2fv(GLint location, GLsizei count,
                                GLboolean transpose, const GLfloat *value) {
  typedef void (*UNIFORMMATRIX3X2FV)(GLint location, GLsizei count,
                                     GLboolean transpose, const GLfloat *value);
  return ((UNIFORMMATRIX3X2FV)pUniformMatrix3x2fv)(location, count, transpose,
                                                   value);
}

void
GlFunctions::UniformMatrix3x4fv(GLint location, GLsizei count,
                                GLboolean transpose, const GLfloat *value) {
  typedef void (*UNIFORMMATRIX3X4FV)(GLint location, GLsizei count,
                                     GLboolean transpose, const GLfloat *value);
  return ((UNIFORMMATRIX3X4FV)pUniformMatrix3x4fv)(location, count, transpose,
                                                   value);
}

void
GlFunctions::UniformMatrix4fv(GLint location, GLsizei count,
                              GLboolean transpose, const GLfloat *value) {
  typedef void (*UNIFORMMATRIX4FV)(GLint location, GLsizei count,
                                   GLboolean transpose, const GLfloat *value);
  return ((UNIFORMMATRIX4FV)pUniformMatrix4fv)(location, count, transpose,
                                               value);
}

void
GlFunctions::UniformMatrix4x2fv(GLint location, GLsizei count,
                                GLboolean transpose, const GLfloat *value) {
  typedef void (*UNIFORMMATRIX4X2FV)(GLint location, GLsizei count,
                                     GLboolean transpose, const GLfloat *value);
  return ((UNIFORMMATRIX4X2FV)pUniformMatrix4x2fv)(location, count, transpose,
                                                   value);
}

void
GlFunctions::UniformMatrix4x3fv(GLint location, GLsizei count,
                                GLboolean transpose, const GLfloat *value) {
  typedef void (*UNIFORMMATRIX4X3FV)(GLint location, GLsizei count,
                                     GLboolean transpose, const GLfloat *value);
  return ((UNIFORMMATRIX4X3FV)pUniformMatrix4x3fv)(location, count, transpose,
                                                   value);
}

void
GlFunctions::Finish() {
  typedef void (*FINISH)();
  return ((FINISH)pFinish)();
}
