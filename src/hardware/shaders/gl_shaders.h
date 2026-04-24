// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2020 by Sonic Team Junior.
// Copyright (C) 2020-2021 by Jaime Ita Passos.
// Copyright (C) 2020-2023 by SRB2 Mobile Project.
// Copyright (C) 2023-2025 by Bitten2Up.
// Copyright (C) 2025 by StarManiaKG.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file gl_shaders.h
/// \brief OpenGL shaders

#ifndef _GL_SHADERS_H_
#define _GL_SHADERS_H_

#include "../r_glcommon/r_glcommon.h"

extern boolean gl_shadersenabled;
extern hwdshaderstage_t gl_allowshaders;

#ifdef GL_SHADERS

#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif
#ifndef GL_INFO_LOG_LENGTH
#define GL_INFO_LOG_LENGTH 0x8B84
#endif

enum
{
	LOC_POSITION  = 0,
	LOC_TEXCOORD  = 1,
	LOC_NORMAL    = 2,
	LOC_COLORS    = 3,

	LOC_TEXCOORD0 = LOC_TEXCOORD,
	LOC_TEXCOORD1 = 4
};

// 13062019
typedef enum
{
#ifdef HAVE_GLES2
	// transform
	gluniform_model,
	gluniform_view,
	gluniform_projection,

	// samplers
	gluniform_startscreen,
	gluniform_endscreen,
	gluniform_fademask,
#endif

	// lighting
	gluniform_poly_color,
	gluniform_tint_color,
	gluniform_fade_color,
	gluniform_lighting,
	gluniform_fade_start,
	gluniform_fade_end,

	// palette rendering
	gluniform_palette_tex, // 1d texture containing a palette
	gluniform_palette_lookup_tex, // 3d texture containing the rgb->index lookup table
	gluniform_lighttable_tex, // 2d texture containing a light table

	// misc.
#ifdef HAVE_GLES2
	gluniform_alphatest,
	gluniform_alphathreshold,
	gluniform_isfadingin,
	gluniform_istowhite,
#endif
	gluniform_leveltime,
	gluniform_scr_resolution,

	gluniform_max,
} gluniform_t;

#ifdef HAVE_GLES2
// 27072020
typedef enum
{
	glesattribute_position,     // LOC_POSITION
	glesattribute_texcoord,     // LOC_TEXCOORD + LOC_TEXCOORD0
	glesattribute_normal,       // LOC_NORMAL
	glesattribute_colors,       // LOC_COLORS
	glesattribute_fadetexcoord, // LOC_TEXCOORD1

	glesattribute_max,
} glesattribute_t;
#endif

typedef struct
{
	char *vertex;
	char *fragment;
	boolean compiled;
} shader_t; // these are in an array and accessed by indices

typedef struct
{
	int base_shader; // index of base shader_t
	int custom_shader; // index of custom shader_t
} glshadertarget_t;
extern glshadertarget_t gl_shadertargets[NUMSHADERTARGETS];

typedef struct gl_shader_s
{
	/// \todo maybe just make this a shader_t?
	char *vertex_shader;
	char *fragment_shader;
	boolean compiled;

	GLuint program;
	GLint uniforms[gluniform_max+1];

#ifdef HAVE_GLES2
	GLint gles_attributes[glesattribute_max+1];
	fmatrix4_t gles_projMatrix;
	fmatrix4_t gles_viewMatrix;
	fmatrix4_t gles_modelMatrix;
#endif
} gl_shader_t;
extern gl_shader_t gl_shaders[HWR_MAXSHADERS];
extern gl_shader_t gl_fallback_shader;

// 09102020
typedef struct gl_shaderstate_s
{
	gl_shader_t *current;
	GLuint type;
	GLuint program;
	boolean changed;
} gl_shaderstate_t;
extern gl_shaderstate_t gl_shaderstate;

typedef struct gl_shader_sources_s {
	const char *vertex;
	const char *fragment;
} gl_shadersources_t;
extern const gl_shadersources_t gl_shadersources[];

// ==========================================================================
//                                                                     PROTOS
// ==========================================================================

typedef GLuint (R_GL_APIENTRY *PFNglCreateShader)       		(GLenum);
typedef void   (R_GL_APIENTRY *PFNglShaderSource)       		(GLuint, GLsizei, const GLchar**, GLint*);
typedef void   (R_GL_APIENTRY *PFNglCompileShader)      		(GLuint);
typedef void   (R_GL_APIENTRY *PFNglGetShaderiv)        		(GLuint, GLenum, GLint*);
typedef void   (R_GL_APIENTRY *PFNglGetShaderInfoLog)   		(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void   (R_GL_APIENTRY *PFNglDeleteShader)       		(GLuint);
typedef GLuint (R_GL_APIENTRY *PFNglCreateProgram)      		(void);
typedef void   (R_GL_APIENTRY *PFNglDeleteProgram)      		(GLuint);
typedef void   (R_GL_APIENTRY *PFNglAttachShader)       		(GLuint, GLuint);
typedef void   (R_GL_APIENTRY *PFNglLinkProgram)        		(GLuint);
typedef void   (R_GL_APIENTRY *PFNglGetProgramiv)       		(GLuint, GLenum, GLint*);
typedef void   (R_GL_APIENTRY *PFNglUseProgram)         		(GLuint);
typedef void   (R_GL_APIENTRY *PFNglUniform1i)          		(GLint, GLint);
typedef void   (R_GL_APIENTRY *PFNglUniform1f)          		(GLint, GLfloat);
typedef void   (R_GL_APIENTRY *PFNglUniform2f)          		(GLint, GLfloat, GLfloat);
typedef void   (R_GL_APIENTRY *PFNglUniform3f)          		(GLint, GLfloat, GLfloat, GLfloat);
typedef void   (R_GL_APIENTRY *PFNglUniform4f)          		(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
typedef void   (R_GL_APIENTRY *PFNglUniform1fv)         		(GLint, GLsizei, const GLfloat*);
typedef void   (R_GL_APIENTRY *PFNglUniform2fv)         		(GLint, GLsizei, const GLfloat*);
typedef void   (R_GL_APIENTRY *PFNglUniform3fv)         		(GLint, GLsizei, const GLfloat*);
typedef GLint  (R_GL_APIENTRY *PFNglGetUniformLocation) 		(GLuint, const GLchar*);
typedef void   (R_GL_APIENTRY *PFNglUniformMatrix4fv)   		(GLint, GLsizei, GLboolean, const GLfloat *);

#ifdef HAVE_GLES2
typedef GLint  (R_GL_APIENTRY *PFNglGetAttribLocation)  		(GLuint, const GLchar*);
typedef void   (R_GL_APIENTRY *PFNglEnableVertexAttribArray)	(GLuint index);
typedef void   (R_GL_APIENTRY *PFNglDisableVertexAttribArray)	(GLuint index);
#endif

extern PFNglCreateShader pglCreateShader;
extern PFNglShaderSource pglShaderSource;
extern PFNglCompileShader pglCompileShader;
extern PFNglGetShaderiv pglGetShaderiv;
extern PFNglGetShaderInfoLog pglGetShaderInfoLog;
extern PFNglDeleteShader pglDeleteShader;
extern PFNglCreateProgram pglCreateProgram;
extern PFNglDeleteProgram pglDeleteProgram;
extern PFNglAttachShader pglAttachShader;
extern PFNglLinkProgram pglLinkProgram;
extern PFNglGetProgramiv pglGetProgramiv;
extern PFNglUseProgram pglUseProgram;
extern PFNglUniform1i pglUniform1i;
extern PFNglUniform1f pglUniform1f;
extern PFNglUniform2f pglUniform2f;
extern PFNglUniform3f pglUniform3f;
extern PFNglUniform4f pglUniform4f;
extern PFNglUniform1fv pglUniform1fv;
extern PFNglUniform2fv pglUniform2fv;
extern PFNglUniform3fv pglUniform3fv;
extern PFNglGetUniformLocation pglGetUniformLocation;
extern PFNglUniformMatrix4fv pglUniformMatrix4fv;

#ifdef HAVE_GLES2
extern PFNglGetAttribLocation pglGetAttribLocation;
extern PFNglEnableVertexAttribArray pglEnableVertexAttribArray;
extern PFNglDisableVertexAttribArray pglDisableVertexAttribArray;
#endif

// ==========================================================================
//                                                                  FUNCTIONS
// ==========================================================================

boolean Shader_LoadFunctions(void);
void Shader_Set(int type);
void Shader_SetIfChanged(gl_shader_t *shader);
void Shader_UnSet(void);

#ifdef HAVE_GLES2
void Shader_SetTransform(void);
#endif
boolean Shader_Init(void);
void Shader_Load(int slot, char *code, hwdshaderstage_t stage);

boolean Shader_Compile(void);
boolean Shader_CompileProgram(gl_shader_t *shader, GLint i);
void Shader_CleanPrograms(void);

void Shader_SetUniforms(FSurfaceInfo *Surface, GLRGBAFloat *poly, GLRGBAFloat *tint, GLRGBAFloat *fade);
void Shader_SetSampler(gluniform_t uniform, GLint value);
void GLShader_SetInfo(hwdshaderinfo_t info, INT32 value);

#ifdef HAVE_GLES2
int Shader_AttribLoc(int loc);
const char *Shader_AttribLocName(int loc);
boolean Shader_EnableVertexAttribArray(int attrib);
boolean Shader_DisableVertexAttribArray(int attrib);
#endif

#endif // GL_SHADERS
#endif // _GL_SHADERS_H_
