#ifndef _GLASS_GL2_H
#define _GLASS_GL2_H

#include <GLASS.h>
#include <GLASS/CommonAPI.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/* Attributes */

void glBindAttribLocation(GLuint program, GLuint index, const GLchar* name);
void glDisableVertexAttribArray(GLuint index);
void glEnableVertexAttribArray(GLuint index);
void glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name);
GLint glGetAttribLocation(GLuint program, const GLchar* name);
void glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params);
void glGetVertexAttribiv(GLuint index, GLenum pname, GLint* params);
void glGetVertexAttribPointerv(GLuint index, GLenum pname, GLvoid** pointer);
void glVertexAttrib1f(GLuint index, GLfloat v0);
void glVertexAttrib2f(GLuint index, GLfloat v0, GLfloat v1);
void glVertexAttrib3f(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2);
void glVertexAttrib4f(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
void glVertexAttrib1fv(GLuint index, const GLfloat* v);
void glVertexAttrib2fv(GLuint index, const GLfloat* v);
void glVertexAttrib3fv(GLuint index, const GLfloat* v);
void glVertexAttrib4fv(GLuint index, const GLfloat* v);
void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer);

/* Framebuffer */

void glBindFramebuffer(GLenum target, GLuint framebuffer);
void glBindRenderbuffer(GLenum target, GLuint renderbuffer);
GLenum glCheckFramebufferStatus(GLenum target);
void glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers);
void glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers);
void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
void glGenFramebuffers(GLsizei n, GLuint* framebuffers);
void glGenRenderbuffers(GLsizei n, GLuint* renderbuffers);
void glGenerateMipmap(GLenum target);
void glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params);
void glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params);
GLboolean glIsFramebuffer(GLuint framebuffer);
GLboolean glIsRenderbuffer(GLuint renderbuffer);
void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);

/* Shaders */

void glAttachShader(GLuint program, GLuint shader);
void glCompileShader(GLuint shader);
GLuint glCreateProgram(void);
GLuint glCreateShader(GLenum shaderType);
void glDeleteProgram(GLuint program);
void glDeleteShader(GLuint shader);
void glDetachShader(GLuint program, GLuint shader);
void glGetAttachedShaders(GLuint program, GLsizei maxCount, GLsizei* count, GLuint* shaders);
void glGetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
void glGetProgramiv(GLuint program, GLenum pname, GLint* params);
void glGetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
void glGetShaderPrecisionFormat(GLenum shaderType, GLenum precisionType, GLint* range, GLint* precision);
void glGetShaderSource(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* source);
void glGetShaderiv(GLuint shader, GLenum pname, GLint* params);
GLboolean glIsProgram(GLuint program);
GLboolean glIsShader(GLuint shader);
void glLinkProgram(GLuint program);
void glReleaseShaderCompiler(void);
void glShaderBinary(GLsizei n, const GLuint* shaders, GLenum binaryformat, const void* binary, GLsizei length);
void glShaderSource(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
void glUseProgram(GLuint program);
void glValidateProgram(GLuint program);

/* Shaders (extensions) */

void glProgramGeometryStridePICA(GLuint program, GLuint stride);
void glProgramGeometryPermutationsPICA(GLuint program, const GLuint* permutations);

/* Uniform */

void glGetActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name);
void glGetUniformfv(GLuint program, GLint location, GLfloat* params);
void glGetUniformiv(GLuint program, GLint location, GLint* params);
GLint glGetUniformLocation(GLuint program, const GLchar* name);
void glUniform1f(GLint location, GLfloat v0);
void glUniform2f(GLint location, GLfloat v0, GLfloat v1);
void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
void glUniform1fv(GLint location, GLsizei count, const GLfloat* value);
void glUniform2fv(GLint location, GLsizei count, const GLfloat* value);
void glUniform3fv(GLint location, GLsizei count, const GLfloat* value);
void glUniform4fv(GLint location, GLsizei count, const GLfloat* value);
void glUniform1i(GLint location, GLint v0);
void glUniform2i(GLint location, GLint v0, GLint v1);
void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2);
void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
void glUniform1iv(GLint location, GLsizei count, const GLint* value);
void glUniform2iv(GLint location, GLsizei count, const GLint* value);
void glUniform3iv(GLint location, GLsizei count, const GLint* value);
void glUniform4iv(GLint location, GLsizei count, const GLint* value);
void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);

#if defined(__cplusplus)
}
#endif // __cplusplus

#endif /* _GLASS_GL2_H */