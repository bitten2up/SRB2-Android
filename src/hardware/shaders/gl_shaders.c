// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2021 by Jaime Ita Passos.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file gl_shaders.c
/// \brief OpenGL shaders

#include "gl_shaders.h"
#include "../r_glcommon/r_glcommon.h"
#include "../../r_local.h" // For rendertimefrac, used for the leveltime shader uniform
#include "../../z_zone.h"

#if 1
#include "../hw_shaders.h" // GLSL_FALLBACK_VERTEX_SHADER //
#endif

boolean gl_shadersenabled = false;
hwdshaderstage_t gl_allowshaders = 0;

typedef GLuint (R_GL_APIENTRY *PFNglCreateShader)       (GLenum);
typedef void   (R_GL_APIENTRY *PFNglShaderSource)       (GLuint, GLsizei, const GLchar**, GLint*);
typedef void   (R_GL_APIENTRY *PFNglCompileShader)      (GLuint);
typedef void   (R_GL_APIENTRY *PFNglGetShaderiv)        (GLuint, GLenum, GLint*);
typedef void   (R_GL_APIENTRY *PFNglGetShaderInfoLog)   (GLuint, GLsizei, GLsizei*, GLchar*);
typedef void   (R_GL_APIENTRY *PFNglDeleteShader)       (GLuint);
typedef GLuint (R_GL_APIENTRY *PFNglCreateProgram)      (void);
typedef void   (R_GL_APIENTRY *PFNglDeleteProgram)      (GLuint);
typedef void   (R_GL_APIENTRY *PFNglAttachShader)       (GLuint, GLuint);
typedef void   (R_GL_APIENTRY *PFNglLinkProgram)        (GLuint);
typedef void   (R_GL_APIENTRY *PFNglGetProgramiv)       (GLuint, GLenum, GLint*);
typedef void   (R_GL_APIENTRY *PFNglUseProgram)         (GLuint);
typedef void   (R_GL_APIENTRY *PFNglUniform1i)          (GLint, GLint);
typedef void   (R_GL_APIENTRY *PFNglUniform1f)          (GLint, GLfloat);
typedef void   (R_GL_APIENTRY *PFNglUniform2f)          (GLint, GLfloat, GLfloat);
typedef void   (R_GL_APIENTRY *PFNglUniform3f)          (GLint, GLfloat, GLfloat, GLfloat);
typedef void   (R_GL_APIENTRY *PFNglUniform4f)          (GLint, GLfloat, GLfloat, GLfloat, GLfloat);
typedef void   (R_GL_APIENTRY *PFNglUniform1fv)         (GLint, GLsizei, const GLfloat*);
typedef void   (R_GL_APIENTRY *PFNglUniform2fv)         (GLint, GLsizei, const GLfloat*);
typedef void   (R_GL_APIENTRY *PFNglUniform3fv)         (GLint, GLsizei, const GLfloat*);
typedef void   (R_GL_APIENTRY *PFNglUniformMatrix4fv)   (GLint, GLsizei, GLboolean, const GLfloat *);
typedef GLint  (R_GL_APIENTRY *PFNglGetUniformLocation) (GLuint, const GLchar*);
typedef GLint  (R_GL_APIENTRY *PFNglGetAttribLocation)  (GLuint, const GLchar*);
typedef void   (R_GL_APIENTRY *PFNglEnableVertexAttribArray) (GLuint index);
typedef void   (R_GL_APIENTRY *PFNglDisableVertexAttribArray) (GLuint index);

static PFNglCreateShader pglCreateShader;
static PFNglShaderSource pglShaderSource;
static PFNglCompileShader pglCompileShader;
static PFNglGetShaderiv pglGetShaderiv;
static PFNglGetShaderInfoLog pglGetShaderInfoLog;
static PFNglDeleteShader pglDeleteShader;
static PFNglCreateProgram pglCreateProgram;
static PFNglDeleteProgram pglDeleteProgram;
static PFNglAttachShader pglAttachShader;
static PFNglLinkProgram pglLinkProgram;
static PFNglGetProgramiv pglGetProgramiv;
static PFNglUseProgram pglUseProgram;
static PFNglUniform1i pglUniform1i;
static PFNglUniform1f pglUniform1f;
static PFNglUniform2f pglUniform2f;
static PFNglUniform3f pglUniform3f;
static PFNglUniform4f pglUniform4f;
static PFNglUniform1fv pglUniform1fv;
static PFNglUniform2fv pglUniform2fv;
static PFNglUniform3fv pglUniform3fv;
static PFNglUniformMatrix4fv pglUniformMatrix4fv;
static PFNglGetUniformLocation pglGetUniformLocation;

#ifdef HAVE_GLES2
static PFNglGetAttribLocation pglGetAttribLocation;
static PFNglEnableVertexAttribArray pglEnableVertexAttribArray;
static PFNglDisableVertexAttribArray pglDisableVertexAttribArray;
#endif

gl_shader_t gl_shaders[HWR_MAXSHADERS];
gl_shader_t gl_usershaders[HWR_MAXSHADERS];
gl_shader_t gl_fallback_shader;

// 09102020
gl_shaderstate_t gl_shaderstate;

#ifdef GL_SHADERS
// STAR NOTE: ????????
static GLRGBAFloat shader_defaultcolor = {1.0f, 1.0f, 1.0f, 1.0f};
#endif

// Shader info
static float shader_leveltime = 0;

#ifdef HAVE_GLES2
#include "shaders_gles2.h"
#else
#error SHOULD BE GLES2
#include "shaders_gl2.h"
#endif

// ================
//  Shader sources
// ================

static struct {
	const char *vertex;
	const char *fragment;
} const gl_shadersources[] = {
	// Floor shader
	{GLSL_DEFAULT_VERTEX_SHADER, GLSL_FLOOR_FRAGMENT_SHADER},

	// Wall shader
	{GLSL_DEFAULT_VERTEX_SHADER, GLSL_WALL_FRAGMENT_SHADER},

	// Sprite shader
	{GLSL_DEFAULT_VERTEX_SHADER, GLSL_WALL_FRAGMENT_SHADER},

	// Model shader
	{GLSL_DEFAULT_VERTEX_SHADER, GLSL_SOFTWARE_FRAGMENT_SHADER},

	// Water shader
	{GLSL_DEFAULT_VERTEX_SHADER, GLSL_WATER_FRAGMENT_SHADER},

	// Fog shader
	{GLSL_DEFAULT_VERTEX_SHADER, GLSL_FOG_FRAGMENT_SHADER},

	// Sky shader
	{GLSL_DEFAULT_VERTEX_SHADER, GLSL_SKY_FRAGMENT_SHADER},

	// Palette postprocess shader
	{GLSL_DEFAULT_VERTEX_SHADER, GLSL_PALETTE_POSTPROCESS_FRAGMENT_SHADER},

	// UI colormap fade shader
	{GLSL_DEFAULT_VERTEX_SHADER, GLSL_UI_COLORMAP_FADE_FRAGMENT_SHADER},

	// UI tinted wipe shader
	{GLSL_DEFAULT_VERTEX_SHADER, GLSL_UI_TINTED_WIPE_FRAGMENT_SHADER},
#ifdef HAVE_GLES2
        // Default shader with alpha test
    {GLSL_DEFAULT_VERTEX_SHADER, GLSL_DEFAULT_ALPHA_TEST},

        // Floor shader with alpha test
    {GLSL_DEFAULT_VERTEX_SHADER, GLSL_SOFTWARE_ALPHA_TEST},

        // Wall shader with alpha test
    {GLSL_DEFAULT_VERTEX_SHADER, GLSL_SOFTWARE_ALPHA_TEST},

        // Sprite shader with alpha test
    {GLSL_DEFAULT_VERTEX_SHADER, GLSL_SOFTWARE_ALPHA_TEST},

        // Model shader with alpha test
    {GLSL_MODEL_VERTEX_SHADER, GLSL_SOFTWARE_ALPHA_TEST},

        // Model lighting shader with alpha test
    {GLSL_MODEL_LIGHTING_VERTEX_SHADER, GLSL_MODEL_LIGHTING_ALPHA_TEST},

        // Water shader with alpha test
    {GLSL_DEFAULT_VERTEX_SHADER, GLSL_WATER_ALPHA_TEST},

        // Fade mask shader
    {GLSL_FADEMASK_VERTEX_SHADER, GLSL_FADEMASK_FRAGMENT_SHADER},

        // Additive and subtractive fade mask shader
    {GLSL_FADEMASK_VERTEX_SHADER, GLSL_FADEMASK_ADDITIVEANDSUBTRACTIVE_FRAGMENT_SHADER},
#endif

	{NULL, NULL},
};

void Shader_LoadFunctions(void)
{
	pglCreateShader = GLBackend_GetFunction("glCreateShader");
	pglShaderSource = GLBackend_GetFunction("glShaderSource");
	pglCompileShader = GLBackend_GetFunction("glCompileShader");
	pglGetShaderiv = GLBackend_GetFunction("glGetShaderiv");
	pglGetShaderInfoLog = GLBackend_GetFunction("glGetShaderInfoLog");
	pglDeleteShader = GLBackend_GetFunction("glDeleteShader");
	pglCreateProgram = GLBackend_GetFunction("glCreateProgram");
	pglDeleteProgram = GLBackend_GetFunction("glDeleteProgram");
	pglAttachShader = GLBackend_GetFunction("glAttachShader");
	pglLinkProgram = GLBackend_GetFunction("glLinkProgram");
	pglGetProgramiv = GLBackend_GetFunction("glGetProgramiv");
	pglUseProgram = GLBackend_GetFunction("glUseProgram");
	pglUniform1i = GLBackend_GetFunction("glUniform1i");
	pglUniform1f = GLBackend_GetFunction("glUniform1f");
	pglUniform2f = GLBackend_GetFunction("glUniform2f");
	pglUniform3f = GLBackend_GetFunction("glUniform3f");
	pglUniform4f = GLBackend_GetFunction("glUniform4f");
	pglUniform1fv = GLBackend_GetFunction("glUniform1fv");
	pglUniform2fv = GLBackend_GetFunction("glUniform2fv");
	pglUniform3fv = GLBackend_GetFunction("glUniform3fv");
	pglUniformMatrix4fv = GLBackend_GetFunction("glUniformMatrix4fv");
	pglGetUniformLocation = GLBackend_GetFunction("glGetUniformLocation");
#ifdef HAVE_GLES2
	pglGetAttribLocation = GLBackend_GetFunction("glGetAttribLocation");
	pglEnableVertexAttribArray = GLBackend_GetFunction("glEnableVertexAttribArray");
	pglDisableVertexAttribArray = GLBackend_GetFunction("glDisableVertexAttribArray");
#endif
}

#ifdef HAVE_GLES2
int Shader_AttribLoc(int loc)
{
	glesattribute_t LOC_TO_ATTRIB[glesattribute_max] =
	{
		glesattribute_position,     // LOC_POSITION
		glesattribute_texcoord,     // LOC_TEXCOORD + LOC_TEXCOORD0
		glesattribute_normal,       // LOC_NORMAL
		glesattribute_colors,       // LOC_COLORS
		glesattribute_fadetexcoord, // LOC_TEXCOORD1
	};
	gl_shader_t *shader = gl_shaderstate.current;
	int pos, attrib;

	(void)pos;

	if (shader == NULL)
	{
		CONS_Printf("Shader_AttribLoc: current shader invalid, moving to fallback shader\n");
		shader = &gl_fallback_shader;
		if (shader == NULL)
			I_Error("Shader_AttribLoc: shader not set");
	}

	attrib = LOC_TO_ATTRIB[loc];

	return shader->gles_attributes[attrib];
}

const char *Shader_AttribLocName(int loc)
{
	const char *names[] = {
		"LOC_POSITION",
		"LOC_TEXCOORD0",
		"LOC_NORMAL",
		"LOC_COLORS",
		"LOC_TEXCOORD1",
	};

	if (loc < 0 || loc > LOC_TEXCOORD1)
		return "(invalid)";

	return names[loc];
}

boolean Shader_EnableVertexAttribArray(int attrib)
{
	gl_shader_t *shader = gl_shaderstate.current;
	int loc;

	if (!shader)
		return false;

	Shader_SetIfChanged(shader);
	loc = Shader_AttribLoc(attrib);

	if (loc != -1)
	{
		pglEnableVertexAttribArray(loc);
		return true;
	}

	return false;
}

boolean Shader_DisableVertexAttribArray(int attrib)
{
	gl_shader_t *shader = gl_shaderstate.current;
	int loc;

	if (!shader)
		return false;

	Shader_SetIfChanged(shader);
	loc = Shader_AttribLoc(attrib);

	if (loc != -1)
	{
		pglDisableVertexAttribArray(loc);
		return true;
	}

	return false;
}
#endif

boolean Shader_Init(void)
{
#ifdef GL_SHADERS
#ifndef HAVE_GLES2
	if (!pglUseProgram)
		return false;
#endif
	#if 0

	gl_fallback_shader.vertex = Z_StrDup(GLSL_FALLBACK_VERTEX_SHADER);
	gl_fallback_shader.fragment = Z_StrDup(GLSL_FALLBACK_FRAGMENT_SHADER);

	if (!Shader_CompileProgram(&gl_fallback_shader, -1))
	{
		GL_MSG_Error("Failed to compile the fallback shader program!\n");
		return false;
	}
	#endif

	return true;
#else
	return false;
#endif
}

//
// Custom shader loading
//
void Shader_Load(int slot, char *code, hwdshaderstage_t stage)
{
	gl_shader_t *shader;

	if (slot < 0 || slot >= HWR_MAXSHADERS)
		I_Error("Shader_Load: Invalid slot %d", slot);

	shader = &gl_shaders[slot];

#define LOADSHADER(source) { \
		if (shader->source) \
			Z_Free(shader->source); \
		shader->source = code; \
	}

	if (stage == HWD_SHADERSTAGE_VERTEX)
		LOADSHADER(vertex)
	else if (stage == HWD_SHADERSTAGE_FRAGMENT)
		LOADSHADER(fragment)
	else
		I_Error("Shader_Load: invalid shader stage");
}


//
// Shader info
// Those are given to the uniforms.
//

void Shader_SetInfo(hwdshaderinfo_t info, INT32 value)
{
	switch (info)
	{
		case HWD_SHADERINFO_LEVELTIME:
			shader_leveltime = (((float)(value-1)) + FIXED_TO_FLOAT(rendertimefrac)) / TICRATE;
			break;
		default:
			break;
	}
}

void Shader_Set(int type)
{
	gl_shader_t *shader = gl_shaderstate.current;

	if (type == SHADER_NONE)
	{
		Shader_UnSet();
		return;
	}

#ifndef HAVE_GLES2
	if (gl_allowshaders == 0)
		return;
#endif

		gl_shader_t *next_shader = &gl_shaders[type]; // the gl_shader_t we are going to switch to

		if (!next_shader->program)
			next_shader = &gl_fallback_shader; // unusable shader, use fallback instead

		// update gl_shaderstate if an actual shader switch is needed
		if (gl_shaderstate.current != next_shader)
		{
			gl_shaderstate.current = next_shader;
			gl_shaderstate.program = next_shader->program;
			gl_shaderstate.type = type;
			gl_shaderstate.changed = true;
		}

		gl_shadersenabled = true;


#ifdef HAVE_GLES2
	(void)shader;
	Shader_SetTransform();
	gl_shadersenabled = true;
#else
	gl_shadersenabled = (shader->program != 0);
#endif
	return;
}

void Shader_UnSet(void)
{
#ifdef HAVE_GLES2
#if 0
	Shader_Set(SHADER_NONE); // star note: normal
#else
	Shader_Set(SHADER_ALPHA_TEST); // bitten temp
#endif
	Shader_SetUniforms(NULL, NULL, NULL, NULL);
#else
	gl_shaderstate.current =  NULL;
	gl_shaderstate.type = 0;
	gl_shaderstate.program = 0;

	if (GLExtension_shaders)
		pglUseProgram(0);
	gl_shadersenabled = true;
#endif
}

void Shader_SetIfChanged(gl_shader_t *shader)
{
	if (shader && gl_shaderstate.changed)
	{
		pglUseProgram(shader->program);
		gl_shaderstate.changed = false;
	}
}

void Shader_CleanPrograms(void)
{
	INT32 i;

	for (i = 0; i < HWR_MAXSHADERS; i++)
	{
		gl_shader_t *shader = &gl_shaders[i];
		gl_shader_t *usershader = &gl_usershaders[i];

		shader->program = 0;
		usershader->program = 0;
	}
}

#define Shader_ErrorMessage GL_MSG_Error

static void Shader_CompileError(const char *message, GLuint program, INT32 shadernum)
{
	GLchar *infoLog = NULL;
	GLint logLength;

	pglGetShaderiv(program, GL_INFO_LOG_LENGTH, &logLength);

	if (logLength)
	{
		infoLog = malloc(logLength);
		pglGetShaderInfoLog(program, logLength, NULL, infoLog);
	}

	Shader_ErrorMessage("Shader_CompileProgram: %s (%s)\n%s\n", message, (shadernum!=-1)? HWR_GetShaderName(shadernum) : "FallbackShader", (infoLog ? infoLog : ""));

	if (infoLog)
		free(infoLog);
}

boolean Shader_CompileProgram(gl_shader_t *shader, GLint i)
{
	GLuint gl_vertShader = 0;
	GLuint gl_fragShader = 0;
	GLint result;
	const GLchar *vert_shader = gl_shadersources[i].vertex;
	const GLchar *frag_shader = gl_shadersources[i].fragment;

#if 1
	// BITTEN DEBUG
	// DUMBASS IF YOU LEAVE THIS IN THE FINAL BUILD... WHATS WRONG WITH YOU
	extern customshaderxlat_t shaderxlat[];
	CONS_Printf("SHADER \"%s\"\n", (i != -1) ? shaderxlat[i].type : "FallbackShader");
#endif

	if (shader->program)
		pglDeleteProgram(shader->program);

	if (!vert_shader && !frag_shader)
	{
		GL_MSG_Error("Shader_CompileProgram: Missing shaders for shader program %s\n", HWR_GetShaderName(i));
		return false;
	}

	if (vert_shader)
	{
		//
		// Load and compile vertex shader
		//
		gl_vertShader = pglCreateShader(GL_VERTEX_SHADER);
		if (!gl_vertShader)
		{
			GL_MSG_Error("Shader_CompileProgram: Error creating vertex shader %s\n", HWR_GetShaderName(i));
			return false;
		}

		pglShaderSource(gl_vertShader, 1, &vert_shader, NULL);
		pglCompileShader(gl_vertShader);

		// check for compile errors
		pglGetShaderiv(gl_vertShader, GL_COMPILE_STATUS, &result);
		if (result == GL_FALSE)
		{
			Shader_CompileError("Error compiling vertex shader", gl_vertShader, i);
			pglDeleteShader(gl_vertShader);
			return false;
		}
	}

	if (frag_shader)
	{
		//
		// Load and compile fragment shader
		//
		gl_fragShader = pglCreateShader(GL_FRAGMENT_SHADER);
		if (!gl_fragShader)
		{
			GL_MSG_Error("Shader_CompileProgram: Error creating fragment shader %s\n", HWR_GetShaderName(i));
			pglDeleteShader(gl_vertShader);
			pglDeleteShader(gl_fragShader);
			return false;
		}

		pglShaderSource(gl_fragShader, 1, &frag_shader, NULL);
		pglCompileShader(gl_fragShader);

		// check for compile errors
		pglGetShaderiv(gl_fragShader, GL_COMPILE_STATUS, &result);
		if (result == GL_FALSE)
		{
			Shader_CompileError("Error compiling fragment shader", gl_fragShader, i);
			pglDeleteShader(gl_vertShader);
			pglDeleteShader(gl_fragShader);
			return false;
		}
	}

	shader->program = pglCreateProgram();
	if (vert_shader)
		pglAttachShader(shader->program, gl_vertShader);
	if (frag_shader)
		pglAttachShader(shader->program, gl_fragShader);
	pglLinkProgram(shader->program);

	// check link status
	pglGetProgramiv(shader->program, GL_LINK_STATUS, &result);

	// delete the shader objects
	if (vert_shader)
		pglDeleteShader(gl_vertShader);
	if (frag_shader)
		pglDeleteShader(gl_fragShader);

	// couldn't link?
	if (result != GL_TRUE)
	{
		GL_MSG_Error("Shader_CompileProgram: Error linking shader program %s\n", HWR_GetShaderName(i));
		pglDeleteProgram(shader->program);
		return false;
	}

	// 13062019
#define GETUNI(uniform) pglGetUniformLocation(shader->program, uniform);

#ifdef HAVE_GLES2
	memset(shader->gles_projMatrix, 0x00, sizeof(fmatrix4_t));
	memset(shader->gles_viewMatrix, 0x00, sizeof(fmatrix4_t));
	memset(shader->gles_modelMatrix, 0x00, sizeof(fmatrix4_t));

	// transform
	shader->uniforms[gluniform_model]      = GETUNI("u_model");
	shader->uniforms[gluniform_view]       = GETUNI("u_view");
	shader->uniforms[gluniform_projection] = GETUNI("u_projection");

	// samplers
	shader->uniforms[gluniform_startscreen] = GETUNI("t_startscreen");
	shader->uniforms[gluniform_endscreen]   = GETUNI("t_endscreen");
	shader->uniforms[gluniform_fademask]    = GETUNI("t_fademask");

	// misc.
	shader->uniforms[gluniform_alphatest]      = GETUNI("alpha_test");
	shader->uniforms[gluniform_alphathreshold] = GETUNI("alpha_threshold");
	shader->uniforms[gluniform_isfadingin]     = GETUNI("is_fading_in");
	shader->uniforms[gluniform_istowhite]      = GETUNI("is_to_white");
#endif

	// lighting
	shader->uniforms[gluniform_poly_color] = GETUNI("poly_color");
	shader->uniforms[gluniform_tint_color] = GETUNI("tint_color");
	shader->uniforms[gluniform_fade_color] = GETUNI("fade_color");
	shader->uniforms[gluniform_lighting] = GETUNI("lighting");
	shader->uniforms[gluniform_fade_start] = GETUNI("fade_start");
	shader->uniforms[gluniform_fade_end] = GETUNI("fade_end");

	// palette rendering
	shader->uniforms[gluniform_palette_tex] = GETUNI("palette_tex");
	shader->uniforms[gluniform_palette_lookup_tex] = GETUNI("palette_lookup_tex");
	shader->uniforms[gluniform_lighttable_tex] = GETUNI("lighttable_tex");

	// misc.
	shader->uniforms[gluniform_leveltime] = GETUNI("leveltime");
#undef GETUNI

	// set permanent uniform values
#define UNIFORM_1(uniform, a, function) \
	if (uniform != -1) \
		function (uniform, a);

	pglUseProgram(shader->program);

	// texture unit numbers for the samplers used for palette rendering
	UNIFORM_1(shader->uniforms[gluniform_palette_tex], 2, pglUniform1i);
	UNIFORM_1(shader->uniforms[gluniform_palette_lookup_tex], 1, pglUniform1i);
	UNIFORM_1(shader->uniforms[gluniform_lighttable_tex], 2, pglUniform1i);

	// restore gl shader state
	pglUseProgram(gl_shaderstate.program);
#undef UNIFORM_1

#ifdef HAVE_GLES2
#define GETATTRIB(attribute) pglGetAttribLocation(shader->program, attribute)
	shader->gles_attributes[glesattribute_position]     = GETATTRIB("a_position");
	shader->gles_attributes[glesattribute_texcoord]     = GETATTRIB("a_texcoord");
	shader->gles_attributes[glesattribute_normal]       = GETATTRIB("a_normal");
	shader->gles_attributes[glesattribute_colors]       = GETATTRIB("a_colors");
	shader->gles_attributes[glesattribute_fadetexcoord] = GETATTRIB("a_fademasktexcoord");
#undef GETATTRIB
#endif

	return true;
}

boolean Shader_Compile(void)
{
#if 1
	GLint i;

	if (!GLExtension_shaders)
		return false;

	for (i = 0; gl_shadersources[i].vertex && gl_shadersources[i].fragment; i++)
	{
		gl_shader_t *shader, *usershader;

		if (i >= HWR_MAXSHADERS)
			break;

		shader = &gl_shaders[i];
		usershader = &gl_usershaders[i];

		if (shader->program)
			pglDeleteProgram(shader->program);
		if (usershader->program)
			pglDeleteProgram(usershader->program);


		shader->program = 0;
		usershader->program = 0;

		if (!Shader_CompileProgram(shader, i))
		{
			shader->program = 0;
#ifdef HAVE_GLES2
			if (i == SHADER_FLOOR)
				return false;
#endif
		}
	}
#else
	gl_fallback_shader.vertex = Z_StrDup(GLSL_FALLBACK_VERTEX_SHADER);
	gl_fallback_shader.fragment = Z_StrDup(GLSL_FALLBACK_FRAGMENT_SHADER);

	if (!Shader_CompileProgram(&gl_fallback_shader, -1))
	{
		GL_MSG_Error("Failed to compile the fallback shader program!\n");
		return false;
	}
#endif

#ifdef HAVE_GLES2
	Shader_Set(SHADER_ALPHA_TEST);
	pglUseProgram(gl_shaderstate.program);
	gl_shaderstate.changed = false;
#endif

	return true;
}

#ifdef HAVE_GLES2
void Shader_SetTransform(void)
{
	gl_shader_t *shader = gl_shaderstate.current;
	if (!shader)
		return;

	Shader_SetIfChanged(shader);

	if (memcmp(projMatrix, shader->gles_projMatrix, sizeof(fmatrix4_t)))
	{
		memcpy(shader->gles_projMatrix, projMatrix, sizeof(fmatrix4_t));
		if (shader->uniforms[gluniform_projection] != -1)
			pglUniformMatrix4fv(shader->uniforms[gluniform_projection], 1, GL_FALSE, (float *)projMatrix);
	}

	if (memcmp(viewMatrix, shader->gles_viewMatrix, sizeof(fmatrix4_t)))
	{
		memcpy(shader->gles_viewMatrix, viewMatrix, sizeof(fmatrix4_t));
		if (shader->uniforms[gluniform_view] != -1)
			pglUniformMatrix4fv(shader->uniforms[gluniform_view], 1, GL_FALSE, (float *)viewMatrix);
	}

	if (memcmp(modelMatrix, shader->gles_modelMatrix, sizeof(fmatrix4_t)))
	{
		memcpy(shader->gles_modelMatrix, modelMatrix, sizeof(fmatrix4_t));
		if (shader->uniforms[gluniform_model] != -1)
			pglUniformMatrix4fv(shader->uniforms[gluniform_model], 1, GL_FALSE, (float *)modelMatrix);
	}
}
#endif

void Shader_SetUniforms(FSurfaceInfo *Surface, GLRGBAFloat *poly, GLRGBAFloat *tint, GLRGBAFloat *fade)
{
#ifdef GL_SHADERS
	gl_shader_t *shader = gl_shaderstate.current;

	if (gl_shadersenabled && (shader != NULL) && pglUseProgram)
	{
		if (!shader->program)
		{
			pglUseProgram(0);
			return;
		}

		Shader_SetIfChanged(shader);

		// Color uniforms can be left NULL and will be set to white (1.0f, 1.0f, 1.0f, 1.0f)
		if (poly == NULL)
			poly = &shader_defaultcolor;
		if (tint == NULL)
			tint = &shader_defaultcolor;
		if (fade == NULL)
			fade = &shader_defaultcolor;

		#define UNIFORM_1(uniform, a, function) \
			if (uniform != -1) \
				function (uniform, a);

		#define UNIFORM_2(uniform, a, b, function) \
			if (uniform != -1) \
				function (uniform, a, b);

		#define UNIFORM_3(uniform, a, b, c, function) \
			if (uniform != -1) \
				function (uniform, a, b, c);

		#define UNIFORM_4(uniform, a, b, c, d, function) \
			if (uniform != -1) \
				function (uniform, a, b, c, d);

		// polygon
		UNIFORM_4(shader->uniforms[gluniform_poly_color], poly->red, poly->green, poly->blue, poly->alpha, pglUniform4f);
		UNIFORM_4(shader->uniforms[gluniform_tint_color], tint->red, tint->green, tint->blue, tint->alpha, pglUniform4f);
		UNIFORM_4(shader->uniforms[gluniform_fade_color], fade->red, fade->green, fade->blue, fade->alpha, pglUniform4f);

		if (Surface != NULL)
		{
			UNIFORM_1(shader->uniforms[gluniform_lighting], (GLfloat)Surface->LightInfo.light_level, pglUniform1f);
			UNIFORM_1(shader->uniforms[gluniform_fade_start], (GLfloat)Surface->LightInfo.fade_start, pglUniform1f);
			UNIFORM_1(shader->uniforms[gluniform_fade_end], (GLfloat)Surface->LightInfo.fade_end, pglUniform1f);
		}

		UNIFORM_1(shader->uniforms[gluniform_leveltime], shader_leveltime, pglUniform1f);

#ifdef HAVE_GLES2
		if (alpha_test)
		{
			UNIFORM_1(shader->uniforms[gluniform_alphathreshold], alpha_threshold, pglUniform1f);
			UNIFORM_1(shader->uniforms[gluniform_alphatest], true, pglUniform1i);
		}
		else
			UNIFORM_1(shader->uniforms[gluniform_alphatest], false, pglUniform1i);
#endif

		#undef UNIFORM_1
		#undef UNIFORM_2
		#undef UNIFORM_3
		#undef UNIFORM_4
	}
#else
	(void)Surface;
	(void)poly;
	(void)tint;
	(void)fade;
#endif
}

void Shader_SetSampler(gluniform_t uniform, GLint value)
{
	gl_shader_t *shader = gl_shaderstate.current;
	if (!shader)
		return;

	Shader_SetIfChanged(shader);

	if (shader->uniforms[uniform] != -1)
		pglUniform1i(shader->uniforms[uniform], value);
}
