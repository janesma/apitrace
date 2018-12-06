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

#ifndef RETRACE_DAEMON_BARGRAPH_GLFRAME_GLHELPER_H_
#define RETRACE_DAEMON_BARGRAPH_GLFRAME_GLHELPER_H_

#include <stdio.h>

#include <GL/gl.h>
#include <GL/glext.h>

#include <string>
#include <vector>

namespace glretrace {

class GlFunctions {
 public:
  static void Init(void *lookup_fn = NULL);
  static GLuint CreateProgram(void);
  static GLuint CreateShader(GLenum type);
  static void ShaderSource(GLuint shader, GLsizei count,
                            const GLchar *const*str,
                            const GLint *length);
  static void CompileShader(GLuint shader);
  static void AttachShader(GLuint program, GLuint shader);
  static void LinkProgram(GLuint program);
  static void UseProgram(GLuint program);
  static GLenum GetError();
  static void GetShaderiv(GLuint shader, GLenum pname, GLint *params);
  static void GetShaderInfoLog(GLuint shader, GLsizei bufSize,
                               GLsizei *length, GLchar *infoLog);
  static void GenBuffers(GLsizei n, GLuint *buffers);
  static void BindBuffer(GLenum target, GLuint buffer);
  static GLint GetAttribLocation(GLuint program, const GLchar *name);
  static GLint GetUniformLocation(GLuint program, const GLchar *name);

  static void ClearColor(GLclampf red, GLclampf green, GLclampf blue,
                         GLclampf alpha);
  static void Clear(GLbitfield mask);
  static void Uniform1f(GLint location, GLfloat v0);
  static void Uniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2,
                        GLfloat v3);
  static void BufferData(GLenum target, GLsizeiptr size, const void *data,
                         GLenum usage);
  static void EnableVertexAttribArray(GLuint index);
  static void VertexAttribPointer(GLuint index, GLint size, GLenum type,
                                  GLboolean normalized, GLsizei stride,
                                  const void *pointer);
  static void DrawArrays(GLenum mode, GLint first, GLsizei count);
  static void DisableVertexAttribArray(GLuint index);
  static void Enable(GLenum cap);

  static void ReadPixels(GLint x, GLint y,
                           GLsizei width, GLsizei height,
                           GLenum format, GLenum type,
                           GLvoid *pixels);
  static void DrawElements(GLenum mode, GLsizei count,
                           GLenum type, const GLvoid *indices);
  static void BlendFunc(GLenum sfactor, GLenum dfactor);

  static void GetFirstPerfQueryIdINTEL(GLuint *queryId);
  static void GetNextPerfQueryIdINTEL(GLuint queryId, GLuint *nextQueryId);
  static void GetIntegerv(GLenum pname, GLint *params);
  static const GLubyte *GetString(GLenum pname);
  static const GLubyte *GetStringi(GLenum pname, GLuint index);
  static void GetGlExtensions(std::string *extensions);
  static void GetPerfQueryInfoINTEL(GLuint queryId, GLuint queryNameLength,
                                    GLchar *queryName, GLuint *dataSize,
                                    GLuint *noCounters, GLuint *noInstances,
                                    GLuint *capsMask);
  static void GetPerfCounterInfoINTEL(GLuint queryId, GLuint counterId,
                                      GLuint counterNameLength,
                                      GLchar *counterName,
                                      GLuint counterDescLength,
                                      GLchar *counterDesc,
                                      GLuint *counterOffset,
                                      GLuint *counterDataSize,
                                      GLuint *counterTypeEnum,
                                      GLuint *counterDataTypeEnum,
                                      GLuint64 *rawCounterMaxValue);

  static void CreatePerfQueryINTEL(GLuint queryId, GLuint *queryHandle);
  static void DeletePerfQueryINTEL(GLuint queryHandle);
  static void BeginPerfQueryINTEL(GLuint queryHandle);
  static void EndPerfQueryINTEL(GLuint queryHandle);
  static void GetPerfQueryDataINTEL(GLuint queryHandle, GLuint flags,
                                    GLsizei dataSize, GLvoid *data,
                                    GLuint *bytesWritten);
  static void GetProgramiv(GLuint program, GLenum pname, GLint *params);
  static void GetProgramInfoLog(GLuint program, GLsizei bufSize,
                                GLsizei *length, GLchar *infoLog);
  static void GetProgramResourceName(GLuint program, GLenum programInterface,
                                     GLuint index, GLsizei bufSize,
                                     GLsizei *length, GLchar *name);

  static void GetBooleanv(GLenum pname, GLboolean *params);
  static void GetFloatv(GLenum pname, GLfloat *params);
  static void BlendColor(GLclampf red, GLclampf green,
                         GLclampf blue, GLclampf alpha);
  static void BlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB,
                                GLenum sfactorAlpha, GLenum dfactorAlpha);
  static void Disable(GLenum cap);
  static void BlendEquation(GLenum mode);
  static void BlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha);

  static void BindAttribLocation(GLuint program, GLuint index,
                                 const GLchar *name);

  static void ValidateProgram(GLuint program);
  static GLboolean IsEnabled(GLenum cap);

  static GLuint GetUniformBlockIndex(GLuint program,
                                     const GLchar *uniformBlockName);
  static void UniformBlockBinding(GLuint program, GLuint uniformBlockIndex,
                                  GLuint uniformBlockBinding);
  static void BindFragDataLocation(GLuint program, GLuint colorNumber,
                                   const char * name);
  static void GetActiveUniform(GLuint program, GLuint index,
                               GLsizei bufSize, GLsizei *length,
                               GLint *size, GLenum *type,
                               GLchar *name);
  static void GetUniformfv(GLuint program, GLint location, GLfloat *params);
  static void GetUniformiv(GLuint program, GLint location, GLint *params);
  static void Uniform1fv(GLint location, GLsizei count, const GLfloat *value);
  static void Uniform1iv(GLint location, GLsizei count, const GLint *value);
  static void Uniform1uiv(GLint location, GLsizei count, const GLuint *value);
  static void Uniform2fv(GLint location, GLsizei count, const GLfloat *value);
  static void Uniform2iv(GLint location, GLsizei count, const GLint *value);
  static void Uniform2uiv(GLint location, GLsizei count, const GLuint *value);
  static void Uniform3fv(GLint location, GLsizei count, const GLfloat *value);
  static void Uniform3iv(GLint location, GLsizei count, const GLint *value);
  static void Uniform3uiv(GLint location, GLsizei count, const GLuint *value);
  static void Uniform4fv(GLint location, GLsizei count, const GLfloat *value);
  static void Uniform4iv(GLint location, GLsizei count, const GLint *value);
  static void Uniform4uiv(GLint location, GLsizei count, const GLuint *value);
  static void UniformMatrix2fv(GLint location, GLsizei count,
                               GLboolean transpose, const GLfloat *value);
  static void UniformMatrix2x3fv(GLint location, GLsizei count,
                                 GLboolean transpose, const GLfloat *value);
  static void UniformMatrix2x4fv(GLint location, GLsizei count,
                                 GLboolean transpose, const GLfloat *value);
  static void UniformMatrix3fv(GLint location, GLsizei count,
                               GLboolean transpose, const GLfloat *value);
  static void UniformMatrix3x2fv(GLint location, GLsizei count,
                                 GLboolean transpose, const GLfloat *value);
  static void UniformMatrix3x4fv(GLint location, GLsizei count,
                                 GLboolean transpose, const GLfloat *value);
  static void UniformMatrix4fv(GLint location, GLsizei count,
                               GLboolean transpose, const GLfloat *value);
  static void UniformMatrix4x2fv(GLint location, GLsizei count,
                                 GLboolean transpose, const GLfloat *value);
  static void UniformMatrix4x3fv(GLint location, GLsizei count,
                                 GLboolean transpose, const GLfloat *value);
  static void Finish();
  static void CullFace(GLenum mode);
  static void LineWidth(GLfloat width);
  static void ColorMask(GLboolean red, GLboolean green,
                        GLboolean blue, GLboolean alpha);
  static void ClearDepthf(GLfloat d);
  static void DepthFunc(GLenum func);
  static void DepthRangef(GLfloat n, GLfloat f);
  static void DepthMask(GLboolean flag);
  static void FrontFace(GLenum mode);
  static void PolygonOffset(GLfloat factor, GLfloat units);
  static void PolygonMode(GLenum face,  GLenum mode);
  static void SampleCoverage(GLfloat value, GLboolean invert);
  static void GetBufferParameteriv(GLenum target, GLenum pname, GLint *params);
  static void *MapBufferRange(GLenum target, GLintptr offset,
                              GLsizeiptr length, GLbitfield access);
  static GLboolean UnmapBuffer(GLenum target);
  static void Scissor(GLint x, GLint y, GLsizei width, GLsizei height);

  static void ClearStencil(GLint s);
  static void ClearBufferfv(GLenum buffer, GLint drawbuffer,
                            const GLfloat *value);
  static void ClearBufferiv(GLenum buffer, GLint drawbuffer,
                            const GLint *value);
  static void StencilOpSeparate(
      GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
  static void StencilFuncSeparate(
      GLenum face, GLenum func, GLint ref, GLuint mask);
  static void StencilMaskSeparate(GLenum face, GLuint mask);

  static void ActiveTexture(GLenum texture);
  static void BindTexture(GLenum target, GLuint texture);
  static void GenTextures(GLsizei n, GLuint *textures);
  static void TexParameteri(GLenum target, GLenum pname, GLint param);
  static void TexImage2D(GLenum target, GLint level, GLint internalformat,
                         GLsizei width, GLsizei height, GLint border,
                         GLenum format, GLenum type, const void *pixels);
  static void GetPerfMonitorGroupsAMD(
      GLint *numGroups, GLsizei groupsSize, GLuint *groups);
  static void GetPerfMonitorCountersAMD(
      GLuint group, GLint *numCounters, GLint *maxActiveCounters,
      GLsizei counterSize, GLuint *counters);
  static void GetPerfMonitorGroupStringAMD(
      GLuint group, GLsizei bufSize, GLsizei *length, GLchar *groupString);
  static void GetPerfMonitorCounterStringAMD(
      GLuint group, GLuint counter, GLsizei bufSize, GLsizei *length,
      GLchar *counterString);
  static void GetPerfMonitorCounterInfoAMD(
      GLuint group, GLuint counter, GLenum pname, void *data);
  static void GenPerfMonitorsAMD(GLsizei n, GLuint *monitors);
  static void DeletePerfMonitorsAMD(GLsizei n, GLuint *monitors);
  static void SelectPerfMonitorCountersAMD(
      GLuint monitor, GLboolean enable, GLuint group,
      GLint numCounters, GLuint *counterList);
  static void BeginPerfMonitorAMD(GLuint monitor);
  static void EndPerfMonitorAMD(GLuint monitor);
  static void GetPerfMonitorCounterDataAMD(
      GLuint monitor, GLenum pname, GLsizei dataSize,
      GLuint *data, GLint *bytesWritten);
  static void GenQueries(GLsizei n, GLuint *ids);
  static void DeleteQueries(GLsizei n, const GLuint *ids);
  static void BeginQuery(GLenum target, GLuint id);
  static void EndQuery(GLenum target);
  static void GetQueryiv(GLenum target, GLenum pname, GLint *params);
  static void GetQueryObjectiv(GLuint id, GLenum pname, GLint *params);
  static void GetQueryObjectui64v(GLuint id, GLenum pname, GLuint64 *params);
  static void QueryCounter(GLuint id, GLenum target);

  static void DebugMessageControl(GLenum source, GLenum type,
                                  GLenum severity, GLsizei count,
                                   const GLuint *ids, GLboolean enabled);
  static void DebugMessageCallback(GLDEBUGPROC callback,
                                   const void *userParam);

 private:
  GlFunctions();
  static bool m_is_initialized;
};

typedef GlFunctions GL;

inline void CheckError(const char * file, int line) {
  const int error = GL::GetError();
  if ( error == GL_NO_ERROR)
    return;
  printf("ERROR: %x %s:%i\n", error, file, line);
}

inline void GetCompileError(GLint shader, std::string *message) {
  GLint status;
  GL::GetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (status == GL_TRUE)
    return;
  static const int MAXLEN = 1024;
  std::vector<char> log(MAXLEN);
  GLsizei len;
  GL::GetShaderInfoLog(shader,  MAXLEN,  &len, log.data());
  *message = log.data();
}

inline void GetLinkError(GLint program, std::string *message) {
  GLint status;
  GL::GetProgramiv(program, GL_LINK_STATUS, &status);
  if (status == GL_TRUE) {
    *message = "";
    return;
  }
  static const int MAXLEN = 1024;
  std::vector<char> log(MAXLEN);
  GLsizei len;
  GL::GetProgramInfoLog(program, MAXLEN, &len, log.data());
  *message = log.data();
}

inline void PrintCompileError(GLint shader) {
  std::string message;
  GetCompileError(shader, &message);
  if (message.size() > 0)
    printf("ERROR -- compile failed: %s\n", message.c_str());
}

}  // namespace glretrace

#define GL_CHECK() glretrace::CheckError(__FILE__, __LINE__)

#endif  // RETRACE_DAEMON_BARGRAPH_GLFRAME_GLHELPER_H_
