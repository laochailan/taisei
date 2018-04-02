/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#ifndef TAISEIGL_H
#define TAISEIGL_H

#include <SDL_platform.h>
#include <SDL_opengl.h>
// #include <SDL_opengl_glext.h>

struct glext_s; // defined at the very bottom
extern struct glext_s glext;

void load_gl_library(void);
void load_gl_functions(void);
void check_gl_extensions(void);
void unload_gl_library(void);

#ifndef APIENTRY
#define APIENTRY
#endif

#ifndef GLAPIENTRY
#define GLAPIENTRY
#endif

#ifdef DEBUG
#ifdef TAISEI_BUILDCONF_DEBUG_OPENGL
#define DEBUG_GL
#endif
#endif

/*
 *
 *      From here on it's mostly generated code (by the embedded python script)
 *
 */

// @BEGIN:typedefs@
typedef void (GLAPIENTRY *tsglActiveTexture_ptr)(GLenum texture);
typedef void (APIENTRY *tsglAttachShader_ptr)(GLuint program, GLuint shader);
typedef void (APIENTRY *tsglBindBuffer_ptr)(GLenum target, GLuint buffer);
typedef void (APIENTRY *tsglBindFramebuffer_ptr)(GLenum target, GLuint framebuffer);
typedef void (GLAPIENTRY *tsglBindTexture_ptr)(GLenum target, GLuint texture);
typedef void (APIENTRY *tsglBindVertexArray_ptr)(GLuint array);
typedef void (APIENTRY *tsglBlendEquationSeparate_ptr)(GLenum modeRGB, GLenum modeAlpha);
typedef void (APIENTRY *tsglBlendFuncSeparate_ptr)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
typedef void (APIENTRY *tsglBufferData_ptr)(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void (APIENTRY *tsglBufferSubData_ptr)(GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
typedef void (GLAPIENTRY *tsglClear_ptr)(GLbitfield mask);
typedef void (GLAPIENTRY *tsglClearColor_ptr)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
typedef void (APIENTRY *tsglCompileShader_ptr)(GLuint shader);
typedef GLuint (APIENTRY *tsglCreateProgram_ptr)(void);
typedef GLuint (APIENTRY *tsglCreateShader_ptr)(GLenum type);
typedef void (GLAPIENTRY *tsglCullFace_ptr)(GLenum mode);
typedef void (APIENTRY *tsglDebugMessageCallback_ptr)(GLDEBUGPROC callback, const void *userParam);
typedef void (APIENTRY *tsglDebugMessageCallbackARB_ptr)(GLDEBUGPROCARB callback, const void *userParam);
typedef void (APIENTRY *tsglDebugMessageControl_ptr)(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled);
typedef void (APIENTRY *tsglDebugMessageControlARB_ptr)(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled);
typedef void (APIENTRY *tsglDeleteBuffers_ptr)(GLsizei n, const GLuint *buffers);
typedef void (APIENTRY *tsglDeleteFramebuffers_ptr)(GLsizei n, const GLuint *framebuffers);
typedef void (APIENTRY *tsglDeleteProgram_ptr)(GLuint program);
typedef void (APIENTRY *tsglDeleteShader_ptr)(GLuint shader);
typedef void (GLAPIENTRY *tsglDeleteTextures_ptr)(GLsizei n, const GLuint *textures);
typedef void (APIENTRY *tsglDeleteVertexArrays_ptr)(GLsizei n, const GLuint *arrays);
typedef void (GLAPIENTRY *tsglDepthFunc_ptr)(GLenum func);
typedef void (GLAPIENTRY *tsglDepthMask_ptr)(GLboolean flag);
typedef void (GLAPIENTRY *tsglDisable_ptr)(GLenum cap);
typedef void (APIENTRY *tsglDisableVertexAttribArray_ptr)(GLuint index);
typedef void (GLAPIENTRY *tsglDrawArrays_ptr)(GLenum mode, GLint first, GLsizei count);
typedef void (APIENTRY *tsglDrawArraysInstanced_ptr)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
typedef void (APIENTRY *tsglDrawBuffers_ptr)(GLsizei n, const GLenum *bufs);
typedef void (GLAPIENTRY *tsglDrawElements_ptr)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
typedef void (APIENTRY *tsglDrawElementsInstanced_ptr)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount);
typedef void (GLAPIENTRY *tsglEnable_ptr)(GLenum cap);
typedef void (APIENTRY *tsglEnableVertexAttribArray_ptr)(GLuint index);
typedef void (APIENTRY *tsglFramebufferTexture2D_ptr)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (APIENTRY *tsglGenBuffers_ptr)(GLsizei n, GLuint *buffers);
typedef void (APIENTRY *tsglGenFramebuffers_ptr)(GLsizei n, GLuint *framebuffers);
typedef void (GLAPIENTRY *tsglGenTextures_ptr)(GLsizei n, GLuint *textures);
typedef void (APIENTRY *tsglGenVertexArrays_ptr)(GLsizei n, GLuint *arrays);
typedef void (APIENTRY *tsglGetActiveUniform_ptr)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
typedef void (GLAPIENTRY *tsglGetIntegerv_ptr)(GLenum pname, GLint *params);
typedef void (APIENTRY *tsglGetProgramInfoLog_ptr)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRY *tsglGetProgramiv_ptr)(GLuint program, GLenum pname, GLint *params);
typedef void (APIENTRY *tsglGetShaderInfoLog_ptr)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRY *tsglGetShaderiv_ptr)(GLuint shader, GLenum pname, GLint *params);
typedef const GLubyte * (GLAPIENTRY *tsglGetString_ptr)(GLenum name);
typedef const GLubyte * (APIENTRY *tsglGetStringi_ptr)(GLenum name, GLuint index);
typedef GLint (APIENTRY *tsglGetUniformLocation_ptr)(GLuint program, const GLchar *name);
typedef void (APIENTRY *tsglLinkProgram_ptr)(GLuint program);
typedef void (APIENTRY *tsglObjectLabel_ptr)(GLenum identifier, GLuint name, GLsizei length, const GLchar *label);
typedef void (GLAPIENTRY *tsglPixelStorei_ptr)(GLenum pname, GLint param);
typedef void (GLAPIENTRY *tsglReadBuffer_ptr)(GLenum mode);
typedef void (GLAPIENTRY *tsglReadPixels_ptr)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
typedef void (APIENTRY *tsglShaderSource_ptr)(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void (GLAPIENTRY *tsglTexImage2D_ptr)(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (GLAPIENTRY *tsglTexParameterf_ptr)(GLenum target, GLenum pname, GLfloat param);
typedef void (GLAPIENTRY *tsglTexParameteri_ptr)(GLenum target, GLenum pname, GLint param);
typedef void (GLAPIENTRY *tsglTexSubImage2D_ptr)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (APIENTRY *tsglUniform1fv_ptr)(GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY *tsglUniform1iv_ptr)(GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY *tsglUniform2fv_ptr)(GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY *tsglUniform2iv_ptr)(GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY *tsglUniform3fv_ptr)(GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY *tsglUniform3iv_ptr)(GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY *tsglUniform4fv_ptr)(GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY *tsglUniform4iv_ptr)(GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY *tsglUniformMatrix3fv_ptr)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRY *tsglUniformMatrix4fv_ptr)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRY *tsglUseProgram_ptr)(GLuint program);
typedef void (APIENTRY *tsglVertexAttribDivisor_ptr)(GLuint index, GLuint divisor);
typedef void (APIENTRY *tsglVertexAttribIPointer_ptr)(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer);
typedef void (APIENTRY *tsglVertexAttribPointer_ptr)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef void (GLAPIENTRY *tsglViewport_ptr)(GLint x, GLint y, GLsizei width, GLsizei height);
// @END:typedefs@

// @BEGIN:undefs@
#undef glActiveTexture
#undef glAttachShader
#undef glBindBuffer
#undef glBindFramebuffer
#undef glBindTexture
#undef glBindVertexArray
#undef glBlendEquationSeparate
#undef glBlendFuncSeparate
#undef glBufferData
#undef glBufferSubData
#undef glClear
#undef glClearColor
#undef glCompileShader
#undef glCreateProgram
#undef glCreateShader
#undef glCullFace
#undef glDebugMessageCallback
#undef glDebugMessageCallbackARB
#undef glDebugMessageControl
#undef glDebugMessageControlARB
#undef glDeleteBuffers
#undef glDeleteFramebuffers
#undef glDeleteProgram
#undef glDeleteShader
#undef glDeleteTextures
#undef glDeleteVertexArrays
#undef glDepthFunc
#undef glDepthMask
#undef glDisable
#undef glDisableVertexAttribArray
#undef glDrawArrays
#undef glDrawArraysInstanced
#undef glDrawBuffers
#undef glDrawElements
#undef glDrawElementsInstanced
#undef glEnable
#undef glEnableVertexAttribArray
#undef glFramebufferTexture2D
#undef glGenBuffers
#undef glGenFramebuffers
#undef glGenTextures
#undef glGenVertexArrays
#undef glGetActiveUniform
#undef glGetIntegerv
#undef glGetProgramInfoLog
#undef glGetProgramiv
#undef glGetShaderInfoLog
#undef glGetShaderiv
#undef glGetString
#undef glGetStringi
#undef glGetUniformLocation
#undef glLinkProgram
#undef glObjectLabel
#undef glPixelStorei
#undef glReadBuffer
#undef glReadPixels
#undef glShaderSource
#undef glTexImage2D
#undef glTexParameterf
#undef glTexParameteri
#undef glTexSubImage2D
#undef glUniform1fv
#undef glUniform1iv
#undef glUniform2fv
#undef glUniform2iv
#undef glUniform3fv
#undef glUniform3iv
#undef glUniform4fv
#undef glUniform4iv
#undef glUniformMatrix3fv
#undef glUniformMatrix4fv
#undef glUseProgram
#undef glVertexAttribDivisor
#undef glVertexAttribIPointer
#undef glVertexAttribPointer
#undef glViewport
// @END:undefs@

#ifndef LINK_TO_LIBGL
// @BEGIN:redefs@
#define glActiveTexture tsglActiveTexture
#define glAttachShader tsglAttachShader
#define glBindBuffer tsglBindBuffer
#define glBindFramebuffer tsglBindFramebuffer
#define glBindTexture tsglBindTexture
#define glBindVertexArray tsglBindVertexArray
#define glBlendEquationSeparate tsglBlendEquationSeparate
#define glBlendFuncSeparate tsglBlendFuncSeparate
#define glBufferData tsglBufferData
#define glBufferSubData tsglBufferSubData
#define glClear tsglClear
#define glClearColor tsglClearColor
#define glCompileShader tsglCompileShader
#define glCreateProgram tsglCreateProgram
#define glCreateShader tsglCreateShader
#define glCullFace tsglCullFace
#define glDebugMessageCallback tsglDebugMessageCallback
#define glDebugMessageCallbackARB tsglDebugMessageCallbackARB
#define glDebugMessageControl tsglDebugMessageControl
#define glDebugMessageControlARB tsglDebugMessageControlARB
#define glDeleteBuffers tsglDeleteBuffers
#define glDeleteFramebuffers tsglDeleteFramebuffers
#define glDeleteProgram tsglDeleteProgram
#define glDeleteShader tsglDeleteShader
#define glDeleteTextures tsglDeleteTextures
#define glDeleteVertexArrays tsglDeleteVertexArrays
#define glDepthFunc tsglDepthFunc
#define glDepthMask tsglDepthMask
#define glDisable tsglDisable
#define glDisableVertexAttribArray tsglDisableVertexAttribArray
#define glDrawArrays tsglDrawArrays
#define glDrawArraysInstanced tsglDrawArraysInstanced
#define glDrawBuffers tsglDrawBuffers
#define glDrawElements tsglDrawElements
#define glDrawElementsInstanced tsglDrawElementsInstanced
#define glEnable tsglEnable
#define glEnableVertexAttribArray tsglEnableVertexAttribArray
#define glFramebufferTexture2D tsglFramebufferTexture2D
#define glGenBuffers tsglGenBuffers
#define glGenFramebuffers tsglGenFramebuffers
#define glGenTextures tsglGenTextures
#define glGenVertexArrays tsglGenVertexArrays
#define glGetActiveUniform tsglGetActiveUniform
#define glGetIntegerv tsglGetIntegerv
#define glGetProgramInfoLog tsglGetProgramInfoLog
#define glGetProgramiv tsglGetProgramiv
#define glGetShaderInfoLog tsglGetShaderInfoLog
#define glGetShaderiv tsglGetShaderiv
#define glGetString tsglGetString
#define glGetStringi tsglGetStringi
#define glGetUniformLocation tsglGetUniformLocation
#define glLinkProgram tsglLinkProgram
#define glObjectLabel tsglObjectLabel
#define glPixelStorei tsglPixelStorei
#define glReadBuffer tsglReadBuffer
#define glReadPixels tsglReadPixels
#define glShaderSource tsglShaderSource
#define glTexImage2D tsglTexImage2D
#define glTexParameterf tsglTexParameterf
#define glTexParameteri tsglTexParameteri
#define glTexSubImage2D tsglTexSubImage2D
#define glUniform1fv tsglUniform1fv
#define glUniform1iv tsglUniform1iv
#define glUniform2fv tsglUniform2fv
#define glUniform2iv tsglUniform2iv
#define glUniform3fv tsglUniform3fv
#define glUniform3iv tsglUniform3iv
#define glUniform4fv tsglUniform4fv
#define glUniform4iv tsglUniform4iv
#define glUniformMatrix3fv tsglUniformMatrix3fv
#define glUniformMatrix4fv tsglUniformMatrix4fv
#define glUseProgram tsglUseProgram
#define glVertexAttribDivisor tsglVertexAttribDivisor
#define glVertexAttribIPointer tsglVertexAttribIPointer
#define glVertexAttribPointer tsglVertexAttribPointer
#define glViewport tsglViewport
// @END:redefs@
#endif // !LINK_TO_LIBGL

#ifndef LINK_TO_LIBGL
// @BEGIN:gldefs@
#define GLDEFS \
GLDEF(glActiveTexture, tsglActiveTexture, tsglActiveTexture_ptr) \
GLDEF(glAttachShader, tsglAttachShader, tsglAttachShader_ptr) \
GLDEF(glBindBuffer, tsglBindBuffer, tsglBindBuffer_ptr) \
GLDEF(glBindFramebuffer, tsglBindFramebuffer, tsglBindFramebuffer_ptr) \
GLDEF(glBindTexture, tsglBindTexture, tsglBindTexture_ptr) \
GLDEF(glBindVertexArray, tsglBindVertexArray, tsglBindVertexArray_ptr) \
GLDEF(glBlendEquationSeparate, tsglBlendEquationSeparate, tsglBlendEquationSeparate_ptr) \
GLDEF(glBlendFuncSeparate, tsglBlendFuncSeparate, tsglBlendFuncSeparate_ptr) \
GLDEF(glBufferData, tsglBufferData, tsglBufferData_ptr) \
GLDEF(glBufferSubData, tsglBufferSubData, tsglBufferSubData_ptr) \
GLDEF(glClear, tsglClear, tsglClear_ptr) \
GLDEF(glClearColor, tsglClearColor, tsglClearColor_ptr) \
GLDEF(glCompileShader, tsglCompileShader, tsglCompileShader_ptr) \
GLDEF(glCreateProgram, tsglCreateProgram, tsglCreateProgram_ptr) \
GLDEF(glCreateShader, tsglCreateShader, tsglCreateShader_ptr) \
GLDEF(glCullFace, tsglCullFace, tsglCullFace_ptr) \
GLDEF(glDebugMessageCallback, tsglDebugMessageCallback, tsglDebugMessageCallback_ptr) \
GLDEF(glDebugMessageCallbackARB, tsglDebugMessageCallbackARB, tsglDebugMessageCallbackARB_ptr) \
GLDEF(glDebugMessageControl, tsglDebugMessageControl, tsglDebugMessageControl_ptr) \
GLDEF(glDebugMessageControlARB, tsglDebugMessageControlARB, tsglDebugMessageControlARB_ptr) \
GLDEF(glDeleteBuffers, tsglDeleteBuffers, tsglDeleteBuffers_ptr) \
GLDEF(glDeleteFramebuffers, tsglDeleteFramebuffers, tsglDeleteFramebuffers_ptr) \
GLDEF(glDeleteProgram, tsglDeleteProgram, tsglDeleteProgram_ptr) \
GLDEF(glDeleteShader, tsglDeleteShader, tsglDeleteShader_ptr) \
GLDEF(glDeleteTextures, tsglDeleteTextures, tsglDeleteTextures_ptr) \
GLDEF(glDeleteVertexArrays, tsglDeleteVertexArrays, tsglDeleteVertexArrays_ptr) \
GLDEF(glDepthFunc, tsglDepthFunc, tsglDepthFunc_ptr) \
GLDEF(glDepthMask, tsglDepthMask, tsglDepthMask_ptr) \
GLDEF(glDisable, tsglDisable, tsglDisable_ptr) \
GLDEF(glDisableVertexAttribArray, tsglDisableVertexAttribArray, tsglDisableVertexAttribArray_ptr) \
GLDEF(glDrawArrays, tsglDrawArrays, tsglDrawArrays_ptr) \
GLDEF(glDrawArraysInstanced, tsglDrawArraysInstanced, tsglDrawArraysInstanced_ptr) \
GLDEF(glDrawBuffers, tsglDrawBuffers, tsglDrawBuffers_ptr) \
GLDEF(glDrawElements, tsglDrawElements, tsglDrawElements_ptr) \
GLDEF(glDrawElementsInstanced, tsglDrawElementsInstanced, tsglDrawElementsInstanced_ptr) \
GLDEF(glEnable, tsglEnable, tsglEnable_ptr) \
GLDEF(glEnableVertexAttribArray, tsglEnableVertexAttribArray, tsglEnableVertexAttribArray_ptr) \
GLDEF(glFramebufferTexture2D, tsglFramebufferTexture2D, tsglFramebufferTexture2D_ptr) \
GLDEF(glGenBuffers, tsglGenBuffers, tsglGenBuffers_ptr) \
GLDEF(glGenFramebuffers, tsglGenFramebuffers, tsglGenFramebuffers_ptr) \
GLDEF(glGenTextures, tsglGenTextures, tsglGenTextures_ptr) \
GLDEF(glGenVertexArrays, tsglGenVertexArrays, tsglGenVertexArrays_ptr) \
GLDEF(glGetActiveUniform, tsglGetActiveUniform, tsglGetActiveUniform_ptr) \
GLDEF(glGetIntegerv, tsglGetIntegerv, tsglGetIntegerv_ptr) \
GLDEF(glGetProgramInfoLog, tsglGetProgramInfoLog, tsglGetProgramInfoLog_ptr) \
GLDEF(glGetProgramiv, tsglGetProgramiv, tsglGetProgramiv_ptr) \
GLDEF(glGetShaderInfoLog, tsglGetShaderInfoLog, tsglGetShaderInfoLog_ptr) \
GLDEF(glGetShaderiv, tsglGetShaderiv, tsglGetShaderiv_ptr) \
GLDEF(glGetString, tsglGetString, tsglGetString_ptr) \
GLDEF(glGetStringi, tsglGetStringi, tsglGetStringi_ptr) \
GLDEF(glGetUniformLocation, tsglGetUniformLocation, tsglGetUniformLocation_ptr) \
GLDEF(glLinkProgram, tsglLinkProgram, tsglLinkProgram_ptr) \
GLDEF(glObjectLabel, tsglObjectLabel, tsglObjectLabel_ptr) \
GLDEF(glPixelStorei, tsglPixelStorei, tsglPixelStorei_ptr) \
GLDEF(glReadBuffer, tsglReadBuffer, tsglReadBuffer_ptr) \
GLDEF(glReadPixels, tsglReadPixels, tsglReadPixels_ptr) \
GLDEF(glShaderSource, tsglShaderSource, tsglShaderSource_ptr) \
GLDEF(glTexImage2D, tsglTexImage2D, tsglTexImage2D_ptr) \
GLDEF(glTexParameterf, tsglTexParameterf, tsglTexParameterf_ptr) \
GLDEF(glTexParameteri, tsglTexParameteri, tsglTexParameteri_ptr) \
GLDEF(glTexSubImage2D, tsglTexSubImage2D, tsglTexSubImage2D_ptr) \
GLDEF(glUniform1fv, tsglUniform1fv, tsglUniform1fv_ptr) \
GLDEF(glUniform1iv, tsglUniform1iv, tsglUniform1iv_ptr) \
GLDEF(glUniform2fv, tsglUniform2fv, tsglUniform2fv_ptr) \
GLDEF(glUniform2iv, tsglUniform2iv, tsglUniform2iv_ptr) \
GLDEF(glUniform3fv, tsglUniform3fv, tsglUniform3fv_ptr) \
GLDEF(glUniform3iv, tsglUniform3iv, tsglUniform3iv_ptr) \
GLDEF(glUniform4fv, tsglUniform4fv, tsglUniform4fv_ptr) \
GLDEF(glUniform4iv, tsglUniform4iv, tsglUniform4iv_ptr) \
GLDEF(glUniformMatrix3fv, tsglUniformMatrix3fv, tsglUniformMatrix3fv_ptr) \
GLDEF(glUniformMatrix4fv, tsglUniformMatrix4fv, tsglUniformMatrix4fv_ptr) \
GLDEF(glUseProgram, tsglUseProgram, tsglUseProgram_ptr) \
GLDEF(glVertexAttribDivisor, tsglVertexAttribDivisor, tsglVertexAttribDivisor_ptr) \
GLDEF(glVertexAttribIPointer, tsglVertexAttribIPointer, tsglVertexAttribIPointer_ptr) \
GLDEF(glVertexAttribPointer, tsglVertexAttribPointer, tsglVertexAttribPointer_ptr) \
GLDEF(glViewport, tsglViewport, tsglViewport_ptr)
// @END:gldefs@

#define GLDEF(glname,tsname,typename) extern typename tsname;
GLDEFS
#undef GLDEF

#endif // !LINK_TO_LIBGL

#ifdef LINK_TO_LIBGL
// @BEGIN:protos@
GLAPI void GLAPIENTRY glActiveTexture( GLenum texture );
GLAPI void APIENTRY glAttachShader (GLuint program, GLuint shader);
GLAPI void APIENTRY glBindBuffer (GLenum target, GLuint buffer);
GLAPI void APIENTRY glBindFramebuffer (GLenum target, GLuint framebuffer);
GLAPI void GLAPIENTRY glBindTexture( GLenum target, GLuint texture );
GLAPI void APIENTRY glBindVertexArray (GLuint array);
GLAPI void APIENTRY glBlendEquationSeparate (GLenum modeRGB, GLenum modeAlpha);
GLAPI void APIENTRY glBlendFuncSeparate (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
GLAPI void APIENTRY glBufferData (GLenum target, GLsizeiptr size, const void *data, GLenum usage);
GLAPI void APIENTRY glBufferSubData (GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
GLAPI void GLAPIENTRY glClear( GLbitfield mask );
GLAPI void GLAPIENTRY glClearColor( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha );
GLAPI void APIENTRY glCompileShader (GLuint shader);
GLAPI GLuint APIENTRY glCreateProgram (void);
GLAPI GLuint APIENTRY glCreateShader (GLenum type);
GLAPI void GLAPIENTRY glCullFace( GLenum mode );
GLAPI void APIENTRY glDebugMessageCallback (GLDEBUGPROC callback, const void *userParam);
GLAPI void APIENTRY glDebugMessageCallbackARB (GLDEBUGPROCARB callback, const void *userParam);
GLAPI void APIENTRY glDebugMessageControl (GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled);
GLAPI void APIENTRY glDebugMessageControlARB (GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled);
GLAPI void APIENTRY glDeleteBuffers (GLsizei n, const GLuint *buffers);
GLAPI void APIENTRY glDeleteFramebuffers (GLsizei n, const GLuint *framebuffers);
GLAPI void APIENTRY glDeleteProgram (GLuint program);
GLAPI void APIENTRY glDeleteShader (GLuint shader);
GLAPI void GLAPIENTRY glDeleteTextures( GLsizei n, const GLuint *textures);
GLAPI void APIENTRY glDeleteVertexArrays (GLsizei n, const GLuint *arrays);
GLAPI void GLAPIENTRY glDepthFunc( GLenum func );
GLAPI void GLAPIENTRY glDepthMask( GLboolean flag );
GLAPI void GLAPIENTRY glDisable( GLenum cap );
GLAPI void APIENTRY glDisableVertexAttribArray (GLuint index);
GLAPI void GLAPIENTRY glDrawArrays( GLenum mode, GLint first, GLsizei count );
GLAPI void APIENTRY glDrawArraysInstanced (GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
GLAPI void APIENTRY glDrawBuffers (GLsizei n, const GLenum *bufs);
GLAPI void GLAPIENTRY glDrawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices );
GLAPI void APIENTRY glDrawElementsInstanced (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount);
GLAPI void GLAPIENTRY glEnable( GLenum cap );
GLAPI void APIENTRY glEnableVertexAttribArray (GLuint index);
GLAPI void APIENTRY glFramebufferTexture2D (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
GLAPI void APIENTRY glGenBuffers (GLsizei n, GLuint *buffers);
GLAPI void APIENTRY glGenFramebuffers (GLsizei n, GLuint *framebuffers);
GLAPI void GLAPIENTRY glGenTextures( GLsizei n, GLuint *textures );
GLAPI void APIENTRY glGenVertexArrays (GLsizei n, GLuint *arrays);
GLAPI void APIENTRY glGetActiveUniform (GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
GLAPI void GLAPIENTRY glGetIntegerv( GLenum pname, GLint *params );
GLAPI void APIENTRY glGetProgramInfoLog (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
GLAPI void APIENTRY glGetProgramiv (GLuint program, GLenum pname, GLint *params);
GLAPI void APIENTRY glGetShaderInfoLog (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
GLAPI void APIENTRY glGetShaderiv (GLuint shader, GLenum pname, GLint *params);
GLAPI const GLubyte * GLAPIENTRY glGetString( GLenum name );
GLAPI const GLubyte *APIENTRY glGetStringi (GLenum name, GLuint index);
GLAPI GLint APIENTRY glGetUniformLocation (GLuint program, const GLchar *name);
GLAPI void APIENTRY glLinkProgram (GLuint program);
GLAPI void APIENTRY glObjectLabel (GLenum identifier, GLuint name, GLsizei length, const GLchar *label);
GLAPI void GLAPIENTRY glPixelStorei( GLenum pname, GLint param );
GLAPI void GLAPIENTRY glReadBuffer( GLenum mode );
GLAPI void GLAPIENTRY glReadPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels );
GLAPI void APIENTRY glShaderSource (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
GLAPI void GLAPIENTRY glTexImage2D( GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels );
GLAPI void GLAPIENTRY glTexParameterf( GLenum target, GLenum pname, GLfloat param );
GLAPI void GLAPIENTRY glTexParameteri( GLenum target, GLenum pname, GLint param );
GLAPI void GLAPIENTRY glTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels );
GLAPI void APIENTRY glUniform1fv (GLint location, GLsizei count, const GLfloat *value);
GLAPI void APIENTRY glUniform1iv (GLint location, GLsizei count, const GLint *value);
GLAPI void APIENTRY glUniform2fv (GLint location, GLsizei count, const GLfloat *value);
GLAPI void APIENTRY glUniform2iv (GLint location, GLsizei count, const GLint *value);
GLAPI void APIENTRY glUniform3fv (GLint location, GLsizei count, const GLfloat *value);
GLAPI void APIENTRY glUniform3iv (GLint location, GLsizei count, const GLint *value);
GLAPI void APIENTRY glUniform4fv (GLint location, GLsizei count, const GLfloat *value);
GLAPI void APIENTRY glUniform4iv (GLint location, GLsizei count, const GLint *value);
GLAPI void APIENTRY glUniformMatrix3fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
GLAPI void APIENTRY glUniformMatrix4fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
GLAPI void APIENTRY glUseProgram (GLuint program);
GLAPI void APIENTRY glVertexAttribDivisor (GLuint index, GLuint divisor);
GLAPI void APIENTRY glVertexAttribIPointer (GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer);
GLAPI void APIENTRY glVertexAttribPointer (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
GLAPI void GLAPIENTRY glViewport( GLint x, GLint y, GLsizei width, GLsizei height );
// @END:protos@

// @BEGIN:reversedefs@
#define tsglActiveTexture glActiveTexture
#define tsglAttachShader glAttachShader
#define tsglBindBuffer glBindBuffer
#define tsglBindFramebuffer glBindFramebuffer
#define tsglBindTexture glBindTexture
#define tsglBindVertexArray glBindVertexArray
#define tsglBlendEquationSeparate glBlendEquationSeparate
#define tsglBlendFuncSeparate glBlendFuncSeparate
#define tsglBufferData glBufferData
#define tsglBufferSubData glBufferSubData
#define tsglClear glClear
#define tsglClearColor glClearColor
#define tsglCompileShader glCompileShader
#define tsglCreateProgram glCreateProgram
#define tsglCreateShader glCreateShader
#define tsglCullFace glCullFace
#define tsglDebugMessageCallback glDebugMessageCallback
#define tsglDebugMessageCallbackARB glDebugMessageCallbackARB
#define tsglDebugMessageControl glDebugMessageControl
#define tsglDebugMessageControlARB glDebugMessageControlARB
#define tsglDeleteBuffers glDeleteBuffers
#define tsglDeleteFramebuffers glDeleteFramebuffers
#define tsglDeleteProgram glDeleteProgram
#define tsglDeleteShader glDeleteShader
#define tsglDeleteTextures glDeleteTextures
#define tsglDeleteVertexArrays glDeleteVertexArrays
#define tsglDepthFunc glDepthFunc
#define tsglDepthMask glDepthMask
#define tsglDisable glDisable
#define tsglDisableVertexAttribArray glDisableVertexAttribArray
#define tsglDrawArrays glDrawArrays
#define tsglDrawArraysInstanced glDrawArraysInstanced
#define tsglDrawBuffers glDrawBuffers
#define tsglDrawElements glDrawElements
#define tsglDrawElementsInstanced glDrawElementsInstanced
#define tsglEnable glEnable
#define tsglEnableVertexAttribArray glEnableVertexAttribArray
#define tsglFramebufferTexture2D glFramebufferTexture2D
#define tsglGenBuffers glGenBuffers
#define tsglGenFramebuffers glGenFramebuffers
#define tsglGenTextures glGenTextures
#define tsglGenVertexArrays glGenVertexArrays
#define tsglGetActiveUniform glGetActiveUniform
#define tsglGetIntegerv glGetIntegerv
#define tsglGetProgramInfoLog glGetProgramInfoLog
#define tsglGetProgramiv glGetProgramiv
#define tsglGetShaderInfoLog glGetShaderInfoLog
#define tsglGetShaderiv glGetShaderiv
#define tsglGetString glGetString
#define tsglGetStringi glGetStringi
#define tsglGetUniformLocation glGetUniformLocation
#define tsglLinkProgram glLinkProgram
#define tsglObjectLabel glObjectLabel
#define tsglPixelStorei glPixelStorei
#define tsglReadBuffer glReadBuffer
#define tsglReadPixels glReadPixels
#define tsglShaderSource glShaderSource
#define tsglTexImage2D glTexImage2D
#define tsglTexParameterf glTexParameterf
#define tsglTexParameteri glTexParameteri
#define tsglTexSubImage2D glTexSubImage2D
#define tsglUniform1fv glUniform1fv
#define tsglUniform1iv glUniform1iv
#define tsglUniform2fv glUniform2fv
#define tsglUniform2iv glUniform2iv
#define tsglUniform3fv glUniform3fv
#define tsglUniform3iv glUniform3iv
#define tsglUniform4fv glUniform4fv
#define tsglUniform4iv glUniform4iv
#define tsglUniformMatrix3fv glUniformMatrix3fv
#define tsglUniformMatrix4fv glUniformMatrix4fv
#define tsglUseProgram glUseProgram
#define tsglVertexAttribDivisor glVertexAttribDivisor
#define tsglVertexAttribIPointer glVertexAttribIPointer
#define tsglVertexAttribPointer glVertexAttribPointer
#define tsglViewport glViewport
// @END:reversedefs@
#endif // LINK_TO_LIBGL

struct glext_s {
	struct {
		char major;
		char minor;
	} version;

	uint debug_output: 1;
	uint KHR_debug: 1;
	uint ARB_debug_output: 1;

	tsglDebugMessageControl_ptr DebugMessageControl;
	tsglDebugMessageCallback_ptr DebugMessageCallback;
};

#endif // !TAISEIGL_H

// Intentionally not guarded

#ifndef TAISEIGL_NO_EXT_ABSTRACTION
	#undef glDebugMessageControl
	#undef glDebugMessageCallback
	#define glDebugMessageControl (glext.DebugMessageControl)
	#define glDebugMessageCallback (glext.DebugMessageCallback)
#elif defined(LINK_TO_LIBGL)
	#undef glDebugMessageControl
	#undef glDebugMessageCallback
#endif // !TAISEIGL_NO_EXT_ABSTRACTION
