#ifndef _GLASS_GL_H
#define _GLASS_GL_H

#include <GLASS.h>
#include <GLASS/CommonAPI.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define glTexEnvStagePICA glCombinerStagePICA

/* Color */

void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);

/* Fog */

void glFogf(GLenum pname, GLfloat param);
void glFogfv(GLenum pname, const GLfloat* params);

/* Light */

void glLightf(GLenum light, GLenum pname, GLfloat param);
void glLightfv(GLenum light, GLenum pname, const GLfloat* params);
void glLightModelf (GLenum pname, GLfloat param);
void glLightModelfv (GLenum pname, const GLfloat* params);

/* TexEnv (wrapper to combiners) */

void glTexEnvf(GLenum target, GLenum pname, GLfloat param);
void glTexEnvi(GLenum target, GLenum pname, GLint param);
void glTexEnvfv(GLenum target, GLenum pname, const GLfloat* params);
void glTexEnviv(GLenum target, GLenum pname, const GLint* params);

// ...

void glClipPlanef (GLenum p, const GLfloat* eqn);
void glFrustumf (GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);
void glGetClipPlanef (GLenum plane, GLfloat* equation);
void glGetLightfv (GLenum light, GLenum pname, GLfloat* params);
void glGetMaterialfv (GLenum face, GLenum pname, GLfloat* params);
void glGetTexEnvfv (GLenum target, GLenum pname, GLfloat* params);

void glLineWidth (GLfloat width);
void glLoadMatrixf (const GLfloat* m);
void glMaterialf (GLenum face, GLenum pname, GLfloat param);
void glMaterialfv (GLenum face, GLenum pname, const GLfloat* params);
void glMultMatrixf (const GLfloat* m);
void glMultiTexCoord4f (GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
void glNormal3f (GLfloat nx, GLfloat ny, GLfloat nz);
void glOrthof (GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);
void glPointParameterf (GLenum pname, GLfloat param);
void glPointParameterfv (GLenum pname, const GLfloat* params);
void glPointSize (GLfloat size);
void glRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void glScalef (GLfloat x, GLfloat y, GLfloat z);
void glTexParameterfv (GLenum target, GLenum pname, const GLfloat* params);
void glTranslatef (GLfloat x, GLfloat y, GLfloat z);
void glClientActiveTexture (GLenum texture);
void glColor4ub (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
void glColorPointer (GLint size, GLenum type, GLsizei stride, const void* pointer);
void glDisableClientState (GLenum array);
void glEnableClientState (GLenum array);
void glGetPointerv (GLenum pname, void** params);
void glGetTexEnviv (GLenum target, GLenum pname, GLint* params);
void glHint (GLenum target, GLenum mode);
void glLoadIdentity (void);
void glMatrixMode (GLenum mode);
void glNormalPointer (GLenum type, GLsizei stride, const void* pointer);
void glPopMatrix (void);
void glPushMatrix (void);
void glSampleCoverage (GLfloat value, GLboolean invert);
void glShadeModel (GLenum mode);
void glTexCoordPointer (GLint size, GLenum type, GLsizei stride, const void* pointer);
void glTexParameteriv (GLenum target, GLenum pname, const GLint* params);
void glVertexPointer (GLint size, GLenum type, GLsizei stride, const void* pointer);

#if defined(__cplusplus)
}
#endif // __cplusplus

#endif /* _GLASS_GL_H */