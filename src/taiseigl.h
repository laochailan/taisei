#if 0
#
#   Run this file with python3 to update the defs.
#   It automatically picks up all the gl* functions used in Taisei code and updates itself.
#

# You can force the script to pick up functions that are not directly called in the code here

force_funcs = [
    'glDrawArraysInstancedEXT',
    'glDrawArraysInstancedARB',
]

import sys, re
from pathlib import Path as P

thisfile = P(sys.argv[0])
srcdir = thisfile.parent
glfuncs = set()

try:
    idir = sys.argv[1]
except IndexError:
    idir = '/usr/include/SDL2'

glheaders = (
    P('%s/SDL_opengl.h' % idir).read_text() +
    P('%s/SDL_opengl_glext.h' % idir).read_text()
)

regex_glcall = re.compile(r'(gl[A-Z][a-zA-Z0-9]+)\(')
regex_glproto_template = r'(^GLAPI\s+([a-zA-Z_0-9\s]+?\**)\s*((?:GL)?APIENTRY)\s+%s\s*\(\s*(.*?)\s*\);)'

for src in srcdir.glob('**/*.c'):
    for func in regex_glcall.findall(src.read_text()):
        glfuncs.add(func)

glfuncs = sorted(list(glfuncs) + force_funcs)

typedefs = []
prototypes = []

for func in glfuncs:
    try:
        proto, rtype, callconv, params = re.findall(regex_glproto_template % func, glheaders, re.DOTALL|re.MULTILINE)[0]
    except IndexError:
        print("Function '%s' not found in the GL headers. Either it really does not exist, or this script is bugged. Please check the opengl headers in %s\nUpdate aborted." % (func, idir))
        exit(1)

    proto = re.sub(r'\s+', ' ', proto, re.DOTALL|re.MULTILINE)
    params = ', '.join(re.split(r',\s*', params))

    typename = 'ts%s_ptr' % func
    typedef = 'typedef %s (%s *%s)(%s);' % (rtype, callconv, typename, params)

    typedefs.append(typedef)
    prototypes.append(proto)

text = thisfile.read_text()

subs = {
    'gldefs': '#define GLDEFS \\\n' + ' \\\n'.join('GLDEF(%s, ts%s, ts%s_ptr)' % (f, f, f) for f in glfuncs),
    'undefs': '\n'.join('#undef %s' % f for f in glfuncs),
    'redefs': '\n'.join('#define %s ts%s' % (f, f) for f in glfuncs),
    'reversedefs': '\n'.join('#define ts%s %s' % (f, f) for f in glfuncs),
    'typedefs': '\n'.join(typedefs),
    'protos': '\n'.join(prototypes),
}

for key, val in subs.items():
    text = re.sub(
        r'(// @BEGIN:%s@)(.*?)(// @END:%s@)' % (key, key),
        r'\1\n%s\n\3' % val,
        text,
        flags=re.DOTALL|re.MULTILINE
    )

thisfile.write_text(text)

#/*
"""
*/
#endif

/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef TAISEIGL_H
#define TAISEIGL_H

#include <SDL_platform.h>
#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>

struct glext_s; // defined at the very bottom
extern struct glext_s glext;

void load_gl_library(void);
void load_gl_functions(void);
void check_gl_extensions(void);

#define gluPerspective tsgluPerspective
void gluPerspective(GLdouble fovY, GLdouble aspect, GLdouble zNear, GLdouble zFar);

#ifndef APIENTRY
#define APIENTRY
#endif

#ifndef GLAPIENTRY
#define GLAPIENTRY
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
typedef void (GLAPIENTRY *tsglBlendEquation_ptr)(GLenum mode);
typedef void (GLAPIENTRY *tsglBlendFunc_ptr)(GLenum sfactor, GLenum dfactor);
typedef void (APIENTRY *tsglBlendFuncSeparate_ptr)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
typedef void (APIENTRY *tsglBufferData_ptr)(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void (APIENTRY *tsglBufferSubData_ptr)(GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
typedef void (GLAPIENTRY *tsglClear_ptr)(GLbitfield mask);
typedef void (GLAPIENTRY *tsglClearColor_ptr)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
typedef void (GLAPIENTRY *tsglColor3f_ptr)(GLfloat red, GLfloat green, GLfloat blue);
typedef void (GLAPIENTRY *tsglColor4f_ptr)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
typedef void (GLAPIENTRY *tsglColor4fv_ptr)(const GLfloat *v);
typedef void (APIENTRY *tsglCompileShader_ptr)(GLuint shader);
typedef GLuint (APIENTRY *tsglCreateProgram_ptr)(void);
typedef GLuint (APIENTRY *tsglCreateShader_ptr)(GLenum type);
typedef void (GLAPIENTRY *tsglCullFace_ptr)(GLenum mode);
typedef void (APIENTRY *tsglDeleteBuffers_ptr)(GLsizei n, const GLuint *buffers);
typedef void (APIENTRY *tsglDeleteFramebuffers_ptr)(GLsizei n, const GLuint *framebuffers);
typedef void (APIENTRY *tsglDeleteProgram_ptr)(GLuint program);
typedef void (APIENTRY *tsglDeleteShader_ptr)(GLuint shader);
typedef void (GLAPIENTRY *tsglDeleteTextures_ptr)(GLsizei n, const GLuint *textures);
typedef void (GLAPIENTRY *tsglDepthFunc_ptr)(GLenum func);
typedef void (GLAPIENTRY *tsglDisable_ptr)(GLenum cap);
typedef void (GLAPIENTRY *tsglDrawArrays_ptr)(GLenum mode, GLint first, GLsizei count);
typedef void (APIENTRY *tsglDrawArraysInstanced_ptr)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
typedef void (APIENTRY *tsglDrawArraysInstancedARB_ptr)(GLenum mode, GLint first, GLsizei count, GLsizei primcount);
typedef void (APIENTRY *tsglDrawArraysInstancedEXT_ptr)(GLenum mode, GLint start, GLsizei count, GLsizei primcount);
typedef void (GLAPIENTRY *tsglDrawElements_ptr)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
typedef void (GLAPIENTRY *tsglEnable_ptr)(GLenum cap);
typedef void (GLAPIENTRY *tsglEnableClientState_ptr)(GLenum cap);
typedef void (APIENTRY *tsglFramebufferTexture2D_ptr)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (GLAPIENTRY *tsglFrustum_ptr)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val);
typedef void (APIENTRY *tsglGenBuffers_ptr)(GLsizei n, GLuint *buffers);
typedef void (APIENTRY *tsglGenFramebuffers_ptr)(GLsizei n, GLuint *framebuffers);
typedef void (GLAPIENTRY *tsglGenTextures_ptr)(GLsizei n, GLuint *textures);
typedef void (APIENTRY *tsglGetActiveUniform_ptr)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
typedef void (APIENTRY *tsglGetProgramInfoLog_ptr)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRY *tsglGetProgramiv_ptr)(GLuint program, GLenum pname, GLint *params);
typedef void (APIENTRY *tsglGetShaderInfoLog_ptr)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRY *tsglGetShaderiv_ptr)(GLuint shader, GLenum pname, GLint *params);
typedef const GLubyte * (GLAPIENTRY *tsglGetString_ptr)(GLenum name);
typedef GLint (APIENTRY *tsglGetUniformLocation_ptr)(GLuint program, const GLchar *name);
typedef void (APIENTRY *tsglLinkProgram_ptr)(GLuint program);
typedef void (GLAPIENTRY *tsglLoadIdentity_ptr)(void);
typedef void (GLAPIENTRY *tsglMatrixMode_ptr)(GLenum mode);
typedef void (GLAPIENTRY *tsglNormalPointer_ptr)(GLenum type, GLsizei stride, const GLvoid *ptr);
typedef void (GLAPIENTRY *tsglOrtho_ptr)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val);
typedef void (GLAPIENTRY *tsglPopMatrix_ptr)(void);
typedef void (GLAPIENTRY *tsglPushMatrix_ptr)(void);
typedef void (GLAPIENTRY *tsglReadBuffer_ptr)(GLenum mode);
typedef void (GLAPIENTRY *tsglReadPixels_ptr)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
typedef void (GLAPIENTRY *tsglRotatef_ptr)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
typedef void (GLAPIENTRY *tsglScalef_ptr)(GLfloat x, GLfloat y, GLfloat z);
typedef void (GLAPIENTRY *tsglScissor_ptr)(GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (APIENTRY *tsglShaderSource_ptr)(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void (GLAPIENTRY *tsglTexCoordPointer_ptr)(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr);
typedef void (GLAPIENTRY *tsglTexImage2D_ptr)(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (GLAPIENTRY *tsglTexParameterf_ptr)(GLenum target, GLenum pname, GLfloat param);
typedef void (GLAPIENTRY *tsglTexParameteri_ptr)(GLenum target, GLenum pname, GLint param);
typedef void (GLAPIENTRY *tsglTranslatef_ptr)(GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY *tsglUniform1f_ptr)(GLint location, GLfloat v0);
typedef void (APIENTRY *tsglUniform1i_ptr)(GLint location, GLint v0);
typedef void (APIENTRY *tsglUniform2f_ptr)(GLint location, GLfloat v0, GLfloat v1);
typedef void (APIENTRY *tsglUniform3f_ptr)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void (APIENTRY *tsglUniform3fv_ptr)(GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY *tsglUniform4f_ptr)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void (APIENTRY *tsglUniform4fv_ptr)(GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY *tsglUseProgram_ptr)(GLuint program);
typedef void (GLAPIENTRY *tsglVertexPointer_ptr)(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr);
typedef void (GLAPIENTRY *tsglViewport_ptr)(GLint x, GLint y, GLsizei width, GLsizei height);
// @END:typedefs@

// @BEGIN:undefs@
#undef glActiveTexture
#undef glAttachShader
#undef glBindBuffer
#undef glBindFramebuffer
#undef glBindTexture
#undef glBlendEquation
#undef glBlendFunc
#undef glBlendFuncSeparate
#undef glBufferData
#undef glBufferSubData
#undef glClear
#undef glClearColor
#undef glColor3f
#undef glColor4f
#undef glColor4fv
#undef glCompileShader
#undef glCreateProgram
#undef glCreateShader
#undef glCullFace
#undef glDeleteBuffers
#undef glDeleteFramebuffers
#undef glDeleteProgram
#undef glDeleteShader
#undef glDeleteTextures
#undef glDepthFunc
#undef glDisable
#undef glDrawArrays
#undef glDrawArraysInstanced
#undef glDrawArraysInstancedARB
#undef glDrawArraysInstancedEXT
#undef glDrawElements
#undef glEnable
#undef glEnableClientState
#undef glFramebufferTexture2D
#undef glFrustum
#undef glGenBuffers
#undef glGenFramebuffers
#undef glGenTextures
#undef glGetActiveUniform
#undef glGetProgramInfoLog
#undef glGetProgramiv
#undef glGetShaderInfoLog
#undef glGetShaderiv
#undef glGetString
#undef glGetUniformLocation
#undef glLinkProgram
#undef glLoadIdentity
#undef glMatrixMode
#undef glNormalPointer
#undef glOrtho
#undef glPopMatrix
#undef glPushMatrix
#undef glReadBuffer
#undef glReadPixels
#undef glRotatef
#undef glScalef
#undef glScissor
#undef glShaderSource
#undef glTexCoordPointer
#undef glTexImage2D
#undef glTexParameterf
#undef glTexParameteri
#undef glTranslatef
#undef glUniform1f
#undef glUniform1i
#undef glUniform2f
#undef glUniform3f
#undef glUniform3fv
#undef glUniform4f
#undef glUniform4fv
#undef glUseProgram
#undef glVertexPointer
#undef glViewport
// @END:undefs@

#ifndef LINK_TO_LIBGL
// @BEGIN:redefs@
#define glActiveTexture tsglActiveTexture
#define glAttachShader tsglAttachShader
#define glBindBuffer tsglBindBuffer
#define glBindFramebuffer tsglBindFramebuffer
#define glBindTexture tsglBindTexture
#define glBlendEquation tsglBlendEquation
#define glBlendFunc tsglBlendFunc
#define glBlendFuncSeparate tsglBlendFuncSeparate
#define glBufferData tsglBufferData
#define glBufferSubData tsglBufferSubData
#define glClear tsglClear
#define glClearColor tsglClearColor
#define glColor3f tsglColor3f
#define glColor4f tsglColor4f
#define glColor4fv tsglColor4fv
#define glCompileShader tsglCompileShader
#define glCreateProgram tsglCreateProgram
#define glCreateShader tsglCreateShader
#define glCullFace tsglCullFace
#define glDeleteBuffers tsglDeleteBuffers
#define glDeleteFramebuffers tsglDeleteFramebuffers
#define glDeleteProgram tsglDeleteProgram
#define glDeleteShader tsglDeleteShader
#define glDeleteTextures tsglDeleteTextures
#define glDepthFunc tsglDepthFunc
#define glDisable tsglDisable
#define glDrawArrays tsglDrawArrays
#define glDrawArraysInstanced tsglDrawArraysInstanced
#define glDrawArraysInstancedARB tsglDrawArraysInstancedARB
#define glDrawArraysInstancedEXT tsglDrawArraysInstancedEXT
#define glDrawElements tsglDrawElements
#define glEnable tsglEnable
#define glEnableClientState tsglEnableClientState
#define glFramebufferTexture2D tsglFramebufferTexture2D
#define glFrustum tsglFrustum
#define glGenBuffers tsglGenBuffers
#define glGenFramebuffers tsglGenFramebuffers
#define glGenTextures tsglGenTextures
#define glGetActiveUniform tsglGetActiveUniform
#define glGetProgramInfoLog tsglGetProgramInfoLog
#define glGetProgramiv tsglGetProgramiv
#define glGetShaderInfoLog tsglGetShaderInfoLog
#define glGetShaderiv tsglGetShaderiv
#define glGetString tsglGetString
#define glGetUniformLocation tsglGetUniformLocation
#define glLinkProgram tsglLinkProgram
#define glLoadIdentity tsglLoadIdentity
#define glMatrixMode tsglMatrixMode
#define glNormalPointer tsglNormalPointer
#define glOrtho tsglOrtho
#define glPopMatrix tsglPopMatrix
#define glPushMatrix tsglPushMatrix
#define glReadBuffer tsglReadBuffer
#define glReadPixels tsglReadPixels
#define glRotatef tsglRotatef
#define glScalef tsglScalef
#define glScissor tsglScissor
#define glShaderSource tsglShaderSource
#define glTexCoordPointer tsglTexCoordPointer
#define glTexImage2D tsglTexImage2D
#define glTexParameterf tsglTexParameterf
#define glTexParameteri tsglTexParameteri
#define glTranslatef tsglTranslatef
#define glUniform1f tsglUniform1f
#define glUniform1i tsglUniform1i
#define glUniform2f tsglUniform2f
#define glUniform3f tsglUniform3f
#define glUniform3fv tsglUniform3fv
#define glUniform4f tsglUniform4f
#define glUniform4fv tsglUniform4fv
#define glUseProgram tsglUseProgram
#define glVertexPointer tsglVertexPointer
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
GLDEF(glBlendEquation, tsglBlendEquation, tsglBlendEquation_ptr) \
GLDEF(glBlendFunc, tsglBlendFunc, tsglBlendFunc_ptr) \
GLDEF(glBlendFuncSeparate, tsglBlendFuncSeparate, tsglBlendFuncSeparate_ptr) \
GLDEF(glBufferData, tsglBufferData, tsglBufferData_ptr) \
GLDEF(glBufferSubData, tsglBufferSubData, tsglBufferSubData_ptr) \
GLDEF(glClear, tsglClear, tsglClear_ptr) \
GLDEF(glClearColor, tsglClearColor, tsglClearColor_ptr) \
GLDEF(glColor3f, tsglColor3f, tsglColor3f_ptr) \
GLDEF(glColor4f, tsglColor4f, tsglColor4f_ptr) \
GLDEF(glColor4fv, tsglColor4fv, tsglColor4fv_ptr) \
GLDEF(glCompileShader, tsglCompileShader, tsglCompileShader_ptr) \
GLDEF(glCreateProgram, tsglCreateProgram, tsglCreateProgram_ptr) \
GLDEF(glCreateShader, tsglCreateShader, tsglCreateShader_ptr) \
GLDEF(glCullFace, tsglCullFace, tsglCullFace_ptr) \
GLDEF(glDeleteBuffers, tsglDeleteBuffers, tsglDeleteBuffers_ptr) \
GLDEF(glDeleteFramebuffers, tsglDeleteFramebuffers, tsglDeleteFramebuffers_ptr) \
GLDEF(glDeleteProgram, tsglDeleteProgram, tsglDeleteProgram_ptr) \
GLDEF(glDeleteShader, tsglDeleteShader, tsglDeleteShader_ptr) \
GLDEF(glDeleteTextures, tsglDeleteTextures, tsglDeleteTextures_ptr) \
GLDEF(glDepthFunc, tsglDepthFunc, tsglDepthFunc_ptr) \
GLDEF(glDisable, tsglDisable, tsglDisable_ptr) \
GLDEF(glDrawArrays, tsglDrawArrays, tsglDrawArrays_ptr) \
GLDEF(glDrawArraysInstanced, tsglDrawArraysInstanced, tsglDrawArraysInstanced_ptr) \
GLDEF(glDrawArraysInstancedARB, tsglDrawArraysInstancedARB, tsglDrawArraysInstancedARB_ptr) \
GLDEF(glDrawArraysInstancedEXT, tsglDrawArraysInstancedEXT, tsglDrawArraysInstancedEXT_ptr) \
GLDEF(glDrawElements, tsglDrawElements, tsglDrawElements_ptr) \
GLDEF(glEnable, tsglEnable, tsglEnable_ptr) \
GLDEF(glEnableClientState, tsglEnableClientState, tsglEnableClientState_ptr) \
GLDEF(glFramebufferTexture2D, tsglFramebufferTexture2D, tsglFramebufferTexture2D_ptr) \
GLDEF(glFrustum, tsglFrustum, tsglFrustum_ptr) \
GLDEF(glGenBuffers, tsglGenBuffers, tsglGenBuffers_ptr) \
GLDEF(glGenFramebuffers, tsglGenFramebuffers, tsglGenFramebuffers_ptr) \
GLDEF(glGenTextures, tsglGenTextures, tsglGenTextures_ptr) \
GLDEF(glGetActiveUniform, tsglGetActiveUniform, tsglGetActiveUniform_ptr) \
GLDEF(glGetProgramInfoLog, tsglGetProgramInfoLog, tsglGetProgramInfoLog_ptr) \
GLDEF(glGetProgramiv, tsglGetProgramiv, tsglGetProgramiv_ptr) \
GLDEF(glGetShaderInfoLog, tsglGetShaderInfoLog, tsglGetShaderInfoLog_ptr) \
GLDEF(glGetShaderiv, tsglGetShaderiv, tsglGetShaderiv_ptr) \
GLDEF(glGetString, tsglGetString, tsglGetString_ptr) \
GLDEF(glGetUniformLocation, tsglGetUniformLocation, tsglGetUniformLocation_ptr) \
GLDEF(glLinkProgram, tsglLinkProgram, tsglLinkProgram_ptr) \
GLDEF(glLoadIdentity, tsglLoadIdentity, tsglLoadIdentity_ptr) \
GLDEF(glMatrixMode, tsglMatrixMode, tsglMatrixMode_ptr) \
GLDEF(glNormalPointer, tsglNormalPointer, tsglNormalPointer_ptr) \
GLDEF(glOrtho, tsglOrtho, tsglOrtho_ptr) \
GLDEF(glPopMatrix, tsglPopMatrix, tsglPopMatrix_ptr) \
GLDEF(glPushMatrix, tsglPushMatrix, tsglPushMatrix_ptr) \
GLDEF(glReadBuffer, tsglReadBuffer, tsglReadBuffer_ptr) \
GLDEF(glReadPixels, tsglReadPixels, tsglReadPixels_ptr) \
GLDEF(glRotatef, tsglRotatef, tsglRotatef_ptr) \
GLDEF(glScalef, tsglScalef, tsglScalef_ptr) \
GLDEF(glScissor, tsglScissor, tsglScissor_ptr) \
GLDEF(glShaderSource, tsglShaderSource, tsglShaderSource_ptr) \
GLDEF(glTexCoordPointer, tsglTexCoordPointer, tsglTexCoordPointer_ptr) \
GLDEF(glTexImage2D, tsglTexImage2D, tsglTexImage2D_ptr) \
GLDEF(glTexParameterf, tsglTexParameterf, tsglTexParameterf_ptr) \
GLDEF(glTexParameteri, tsglTexParameteri, tsglTexParameteri_ptr) \
GLDEF(glTranslatef, tsglTranslatef, tsglTranslatef_ptr) \
GLDEF(glUniform1f, tsglUniform1f, tsglUniform1f_ptr) \
GLDEF(glUniform1i, tsglUniform1i, tsglUniform1i_ptr) \
GLDEF(glUniform2f, tsglUniform2f, tsglUniform2f_ptr) \
GLDEF(glUniform3f, tsglUniform3f, tsglUniform3f_ptr) \
GLDEF(glUniform3fv, tsglUniform3fv, tsglUniform3fv_ptr) \
GLDEF(glUniform4f, tsglUniform4f, tsglUniform4f_ptr) \
GLDEF(glUniform4fv, tsglUniform4fv, tsglUniform4fv_ptr) \
GLDEF(glUseProgram, tsglUseProgram, tsglUseProgram_ptr) \
GLDEF(glVertexPointer, tsglVertexPointer, tsglVertexPointer_ptr) \
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
GLAPI void GLAPIENTRY glBlendEquation( GLenum mode );
GLAPI void GLAPIENTRY glBlendFunc( GLenum sfactor, GLenum dfactor );
GLAPI void APIENTRY glBlendFuncSeparate (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
GLAPI void APIENTRY glBufferData (GLenum target, GLsizeiptr size, const void *data, GLenum usage);
GLAPI void APIENTRY glBufferSubData (GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
GLAPI void GLAPIENTRY glClear( GLbitfield mask );
GLAPI void GLAPIENTRY glClearColor( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha );
GLAPI void GLAPIENTRY glColor3f( GLfloat red, GLfloat green, GLfloat blue );
GLAPI void GLAPIENTRY glColor4f( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );
GLAPI void GLAPIENTRY glColor4fv( const GLfloat *v );
GLAPI void APIENTRY glCompileShader (GLuint shader);
GLAPI GLuint APIENTRY glCreateProgram (void);
GLAPI GLuint APIENTRY glCreateShader (GLenum type);
GLAPI void GLAPIENTRY glCullFace( GLenum mode );
GLAPI void APIENTRY glDeleteBuffers (GLsizei n, const GLuint *buffers);
GLAPI void APIENTRY glDeleteFramebuffers (GLsizei n, const GLuint *framebuffers);
GLAPI void APIENTRY glDeleteProgram (GLuint program);
GLAPI void APIENTRY glDeleteShader (GLuint shader);
GLAPI void GLAPIENTRY glDeleteTextures( GLsizei n, const GLuint *textures);
GLAPI void GLAPIENTRY glDepthFunc( GLenum func );
GLAPI void GLAPIENTRY glDisable( GLenum cap );
GLAPI void GLAPIENTRY glDrawArrays( GLenum mode, GLint first, GLsizei count );
GLAPI void APIENTRY glDrawArraysInstanced (GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
GLAPI void APIENTRY glDrawArraysInstancedARB (GLenum mode, GLint first, GLsizei count, GLsizei primcount);
GLAPI void APIENTRY glDrawArraysInstancedEXT (GLenum mode, GLint start, GLsizei count, GLsizei primcount);
GLAPI void GLAPIENTRY glDrawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices );
GLAPI void GLAPIENTRY glEnable( GLenum cap );
GLAPI void GLAPIENTRY glEnableClientState( GLenum cap );
GLAPI void APIENTRY glFramebufferTexture2D (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
GLAPI void GLAPIENTRY glFrustum( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val );
GLAPI void APIENTRY glGenBuffers (GLsizei n, GLuint *buffers);
GLAPI void APIENTRY glGenFramebuffers (GLsizei n, GLuint *framebuffers);
GLAPI void GLAPIENTRY glGenTextures( GLsizei n, GLuint *textures );
GLAPI void APIENTRY glGetActiveUniform (GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
GLAPI void APIENTRY glGetProgramInfoLog (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
GLAPI void APIENTRY glGetProgramiv (GLuint program, GLenum pname, GLint *params);
GLAPI void APIENTRY glGetShaderInfoLog (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
GLAPI void APIENTRY glGetShaderiv (GLuint shader, GLenum pname, GLint *params);
GLAPI const GLubyte * GLAPIENTRY glGetString( GLenum name );
GLAPI GLint APIENTRY glGetUniformLocation (GLuint program, const GLchar *name);
GLAPI void APIENTRY glLinkProgram (GLuint program);
GLAPI void GLAPIENTRY glLoadIdentity( void );
GLAPI void GLAPIENTRY glMatrixMode( GLenum mode );
GLAPI void GLAPIENTRY glNormalPointer( GLenum type, GLsizei stride, const GLvoid *ptr );
GLAPI void GLAPIENTRY glOrtho( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val );
GLAPI void GLAPIENTRY glPopMatrix( void );
GLAPI void GLAPIENTRY glPushMatrix( void );
GLAPI void GLAPIENTRY glReadBuffer( GLenum mode );
GLAPI void GLAPIENTRY glReadPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels );
GLAPI void GLAPIENTRY glRotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z );
GLAPI void GLAPIENTRY glScalef( GLfloat x, GLfloat y, GLfloat z );
GLAPI void GLAPIENTRY glScissor( GLint x, GLint y, GLsizei width, GLsizei height);
GLAPI void APIENTRY glShaderSource (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
GLAPI void GLAPIENTRY glTexCoordPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *ptr );
GLAPI void GLAPIENTRY glTexImage2D( GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels );
GLAPI void GLAPIENTRY glTexParameterf( GLenum target, GLenum pname, GLfloat param );
GLAPI void GLAPIENTRY glTexParameteri( GLenum target, GLenum pname, GLint param );
GLAPI void GLAPIENTRY glTranslatef( GLfloat x, GLfloat y, GLfloat z );
GLAPI void APIENTRY glUniform1f (GLint location, GLfloat v0);
GLAPI void APIENTRY glUniform1i (GLint location, GLint v0);
GLAPI void APIENTRY glUniform2f (GLint location, GLfloat v0, GLfloat v1);
GLAPI void APIENTRY glUniform3f (GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
GLAPI void APIENTRY glUniform3fv (GLint location, GLsizei count, const GLfloat *value);
GLAPI void APIENTRY glUniform4f (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
GLAPI void APIENTRY glUniform4fv (GLint location, GLsizei count, const GLfloat *value);
GLAPI void APIENTRY glUseProgram (GLuint program);
GLAPI void GLAPIENTRY glVertexPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *ptr );
GLAPI void GLAPIENTRY glViewport( GLint x, GLint y, GLsizei width, GLsizei height );
// @END:protos@

// @BEGIN:reversedefs@
#define tsglActiveTexture glActiveTexture
#define tsglAttachShader glAttachShader
#define tsglBindBuffer glBindBuffer
#define tsglBindFramebuffer glBindFramebuffer
#define tsglBindTexture glBindTexture
#define tsglBlendEquation glBlendEquation
#define tsglBlendFunc glBlendFunc
#define tsglBlendFuncSeparate glBlendFuncSeparate
#define tsglBufferData glBufferData
#define tsglBufferSubData glBufferSubData
#define tsglClear glClear
#define tsglClearColor glClearColor
#define tsglColor3f glColor3f
#define tsglColor4f glColor4f
#define tsglColor4fv glColor4fv
#define tsglCompileShader glCompileShader
#define tsglCreateProgram glCreateProgram
#define tsglCreateShader glCreateShader
#define tsglCullFace glCullFace
#define tsglDeleteBuffers glDeleteBuffers
#define tsglDeleteFramebuffers glDeleteFramebuffers
#define tsglDeleteProgram glDeleteProgram
#define tsglDeleteShader glDeleteShader
#define tsglDeleteTextures glDeleteTextures
#define tsglDepthFunc glDepthFunc
#define tsglDisable glDisable
#define tsglDrawArrays glDrawArrays
#define tsglDrawArraysInstanced glDrawArraysInstanced
#define tsglDrawArraysInstancedARB glDrawArraysInstancedARB
#define tsglDrawArraysInstancedEXT glDrawArraysInstancedEXT
#define tsglDrawElements glDrawElements
#define tsglEnable glEnable
#define tsglEnableClientState glEnableClientState
#define tsglFramebufferTexture2D glFramebufferTexture2D
#define tsglFrustum glFrustum
#define tsglGenBuffers glGenBuffers
#define tsglGenFramebuffers glGenFramebuffers
#define tsglGenTextures glGenTextures
#define tsglGetActiveUniform glGetActiveUniform
#define tsglGetProgramInfoLog glGetProgramInfoLog
#define tsglGetProgramiv glGetProgramiv
#define tsglGetShaderInfoLog glGetShaderInfoLog
#define tsglGetShaderiv glGetShaderiv
#define tsglGetString glGetString
#define tsglGetUniformLocation glGetUniformLocation
#define tsglLinkProgram glLinkProgram
#define tsglLoadIdentity glLoadIdentity
#define tsglMatrixMode glMatrixMode
#define tsglNormalPointer glNormalPointer
#define tsglOrtho glOrtho
#define tsglPopMatrix glPopMatrix
#define tsglPushMatrix glPushMatrix
#define tsglReadBuffer glReadBuffer
#define tsglReadPixels glReadPixels
#define tsglRotatef glRotatef
#define tsglScalef glScalef
#define tsglScissor glScissor
#define tsglShaderSource glShaderSource
#define tsglTexCoordPointer glTexCoordPointer
#define tsglTexImage2D glTexImage2D
#define tsglTexParameterf glTexParameterf
#define tsglTexParameteri glTexParameteri
#define tsglTranslatef glTranslatef
#define tsglUniform1f glUniform1f
#define tsglUniform1i glUniform1i
#define tsglUniform2f glUniform2f
#define tsglUniform3f glUniform3f
#define tsglUniform3fv glUniform3fv
#define tsglUniform4f glUniform4f
#define tsglUniform4fv glUniform4fv
#define tsglUseProgram glUseProgram
#define tsglVertexPointer glVertexPointer
#define tsglViewport glViewport
// @END:reversedefs@
#endif // LINK_TO_LIBGL

struct glext_s {
    struct {
        char major;
        char minor;
    } version;

    int draw_instanced: 1;
    int EXT_draw_instanced: 1;
    int ARB_draw_instanced: 1;

    tsglDrawArraysInstanced_ptr DrawArraysInstanced;
};

#endif // !TAISEIGL_H

// Intentionally not guarded

#ifndef TAISEIGL_NO_EXT_ABSTRACTION
    #undef glDrawArraysInstanced
    #define glDrawArraysInstanced (glext.DrawArraysInstanced)
#elif defined(LINK_TO_LIBGL)
    #undef glDrawArraysInstanced
#endif // !TAISEIGL_NO_EXT_ABSTRACTION

// Don't even think about touching the construct below
/*
"""
#*/
