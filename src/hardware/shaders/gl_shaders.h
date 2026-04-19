// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2021 by Jaime Ita Passos.
// Copyright (C) 1998-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file gl_shaders.h
/// \brief OpenGL shaders

#ifndef _GL_SHADERS_H_
#define _GL_SHADERS_H_

#include "../../doomdef.h"
#include "../hw_drv.h"

#include "../r_glcommon/r_glcommon.h"

extern boolean gl_shadersenabled;
extern hwdshaderstage_t gl_allowshaders;

#ifdef GL_SHADERS

void Shader_LoadFunctions(void);

enum
{
	LOC_POSITION  = 0,
	LOC_TEXCOORD  = 1,
	LOC_NORMAL    = 2,
	LOC_COLORS    = 3,

	LOC_TEXCOORD0 = LOC_TEXCOORD,
	LOC_TEXCOORD1 = 4
};

#define MAXSHADERS 16
#define MAXSHADERPROGRAMS 16

// 08072020
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

	gluniform_max,
} gluniform_t;

// 27072020
#ifdef HAVE_GLES2
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

// STAR NOTE: alright, here's our main point of contention...
typedef struct gl_shader_s
{
	char *vertex; //char *vertex_shader;
	char *fragment; //char *fragment_shader;
	GLuint program;
	GLint uniforms[gluniform_max+1];
	boolean compiled;
#ifdef HAVE_GLES2
	GLint gles_attributes[glesattribute_max+1];
	fmatrix4_t gles_projMatrix;
	fmatrix4_t gles_viewMatrix;
	fmatrix4_t gles_modelMatrix;
#endif
} gl_shader_t;
#if 0
// STAR NOTE: for reference
typedef struct gl_shader_s
{
	char *vertex_shader;
	char *fragment_shader;
	GLuint program;
	GLint uniforms[gluniform_max+1];
} gl_shader_t;
#endif

extern gl_shader_t gl_shaders[HWR_MAXSHADERS];
// i fucked up and accidently removed this from gl_shaders.c, fix later though i doubt anyone will fucking care -bitten
extern gl_shader_t gl_usershaders[HWR_MAXSHADERS];
#if 1
// STAR NOTE: i'm not paid enough to understand this
extern gl_shader_t gl_fallback_shader;
#endif

// 09102020
typedef struct gl_shaderstate_s
{
	gl_shader_t *current;
	GLuint type;
	GLuint program;
	boolean changed;
} gl_shaderstate_t;
extern gl_shaderstate_t gl_shaderstate;

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
#define Shader_SetIntegerUniform Shader_SetSampler
void Shader_SetInfo(hwdshaderinfo_t info, INT32 value);

#ifdef HAVE_GLES2
int Shader_AttribLoc(int loc);
const char *Shader_AttribLocName(int loc);
boolean Shader_EnableVertexAttribArray(int attrib);
boolean Shader_DisableVertexAttribArray(int attrib);
#endif

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

#endif // GL_SHADERS

#endif // _GL_SHADERS_H_
