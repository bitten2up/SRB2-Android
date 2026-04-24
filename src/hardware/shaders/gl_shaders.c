// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2021 by Jaime Ita Passos.
// Copyright (C) 2021-2023 by SRB2 Mobile Project.
// Copyright (C) 2023-2025 by Bitten2Up.
// Copyright (C) 2025 by StarManiaKG.
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

#ifdef GL_SHADERS

// STAR NOTE: return here
#if defined (HAVE_GLES2)
	#include "shaders_gles2.h"
#elif defined (HAVE_GLES)
	#if defined (__ANDROID__)
		#error SHOULD BE GLES2
	#endif
	#include "shaders_gl2.h"
#else
	//#include "../hw_shaders.h" // GLSL_FALLBACK_VERTEX_SHADER //
#endif
#include "../hw_shaders.h" // GLSL_FALLBACK_VERTEX_SHADER //
#include "shaders_gl2.h"

boolean gl_shadersenabled = false;
hwdshaderstage_t gl_allowshaders = 0;

PFNglCreateShader pglCreateShader;
PFNglShaderSource pglShaderSource;
PFNglCompileShader pglCompileShader;
PFNglGetShaderiv pglGetShaderiv;
PFNglGetShaderInfoLog pglGetShaderInfoLog;
PFNglDeleteShader pglDeleteShader;
PFNglCreateProgram pglCreateProgram;
PFNglDeleteProgram pglDeleteProgram;
PFNglAttachShader pglAttachShader;
PFNglLinkProgram pglLinkProgram;
PFNglGetProgramiv pglGetProgramiv;
PFNglUseProgram pglUseProgram;
PFNglUniform1i pglUniform1i;
PFNglUniform1f pglUniform1f;
PFNglUniform2f pglUniform2f;
PFNglUniform3f pglUniform3f;
PFNglUniform4f pglUniform4f;
PFNglUniform1fv pglUniform1fv;
PFNglUniform2fv pglUniform2fv;
PFNglUniform3fv pglUniform3fv;
PFNglGetUniformLocation pglGetUniformLocation;
PFNglUniformMatrix4fv pglUniformMatrix4fv;

#ifdef HAVE_GLES2
PFNglGetAttribLocation pglGetAttribLocation;
PFNglEnableVertexAttribArray pglEnableVertexAttribArray;
PFNglDisableVertexAttribArray pglDisableVertexAttribArray;
#endif

// the array has NUMSHADERTARGETS entries for base shaders and for custom shaders
// the array could be expanded in the future to fit "dynamic" custom shaders that
// aren't fixed to shader targets
gl_shader_t gl_shaders[HWR_MAXSHADERS];
gl_shader_t gl_fallback_shader;

// 09102020
gl_shaderstate_t gl_shaderstate;
glshadertarget_t gl_shadertargets[NUMSHADERTARGETS];

// Shader info
static GLRGBAFloat shader_defaultcolor = {1.0f, 1.0f, 1.0f, 1.0f};
static float shader_leveltime = 0;

// ==================
//  Shader functions
// ==================

boolean Shader_LoadFunctions(void)
{
	GETOPENGLFUNC(CreateShader)
	GETOPENGLFUNC(ShaderSource)
	GETOPENGLFUNC(CompileShader)
	GETOPENGLFUNC(GetShaderiv)
	GETOPENGLFUNC(GetShaderInfoLog)
	GETOPENGLFUNC(AttachShader)
	GETOPENGLFUNC(DeleteShader)
	GETOPENGLFUNC(CreateProgram)
	GETOPENGLFUNC(LinkProgram)
	GETOPENGLFUNC(UseProgram)
	GETOPENGLFUNC(DeleteProgram)
	GETOPENGLFUNC(GetProgramiv)

	GETOPENGLFUNC(Uniform1i)
	GETOPENGLFUNC(Uniform1f)
	GETOPENGLFUNC(Uniform2f)
	GETOPENGLFUNC(Uniform3f)
	GETOPENGLFUNC(Uniform4f)
	GETOPENGLFUNC(Uniform1fv)
	GETOPENGLFUNC(Uniform2fv)
	GETOPENGLFUNC(Uniform3fv)
	GETOPENGLFUNC(GetUniformLocation)

	GETOPENGLFUNC(UniformMatrix4fv)

#ifdef HAVE_GLES2
	GETOPENGLFUNCTRY(GetAttribLocation)
	GETOPENGLFUNCTRY(EnableVertexAttribArray)
	GETOPENGLFUNCTRY(DisableVertexAttribArray)
#endif

	return true;
}

#ifdef HAVE_GLES2
int Shader_AttribLoc(int loc)
{
	glesattribute_t LOC_TO_ATTRIB[glesattribute_max] = {
		glesattribute_position,     // LOC_POSITION
		glesattribute_texcoord,     // LOC_TEXCOORD + LOC_TEXCOORD0
		glesattribute_normal,       // LOC_NORMAL
		glesattribute_colors,       // LOC_COLORS
		glesattribute_fadetexcoord, // LOC_TEXCOORD1
	};
	gl_shader_t *shader = gl_shaderstate.current;
	int attrib;

	if (shader == NULL)
	{
		CONS_Printf("Shader_AttribLoc: Current shader invalid, moving to fallback shader...\n");
		shader = &gl_fallback_shader;
		if (shader == NULL)
			I_Error("Shader_AttribLoc: No shader could be set!");
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
#ifndef HAVE_GLES2
	if (!pglUseProgram)
		return false;
#endif

	gl_fallback_shader.vertex_shader = Z_StrDup(GLSL_FALLBACK_VERTEX_SHADER);
	gl_fallback_shader.fragment_shader = Z_StrDup(GLSL_FALLBACK_FRAGMENT_SHADER);

	if (!Shader_CompileProgram(&gl_fallback_shader, -1))
	{
		GL_MSG_Error("Failed to compile the fallback shader program!\n");
		return false;
	}
	return true;
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
		LOADSHADER(vertex_shader)
	else if (stage == HWD_SHADERSTAGE_FRAGMENT)
		LOADSHADER(fragment_shader)
	else
		I_Error("Shader_Load: invalid shader stage");
}

//
// Shader info
// Those are given to the uniforms.
//
void GLShader_SetInfo(hwdshaderinfo_t info, INT32 value)
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

void Shader_Set(int shader_type)
{
	gl_shader_t *shader = gl_shaderstate.current;
	gl_shader_t *next_shader; // the gl_shader_t we are going to switch to

	if (shader_type == SHADER_NONE)
	{
		Shader_UnSet();
		return;
	}

#ifndef HAVE_GLES2
	if (gl_allowshaders == 0)
	{
		gl_shadersenabled = false;
		return;
	}
#endif

	next_shader = &gl_shaders[shader_type];

#ifdef HAVE_GLES2
	if (!next_shader->program && alpha_test)
		next_shader = &gl_shaders[GLBackend_InvertAlphaTestShader(shader_type)];
#endif

	if (!next_shader->program)
	{
		next_shader = &gl_fallback_shader; // unusable shader, use fallback instead
		alpha_test = false;
	}

	// update gl_shaderstate if an actual shader switch is needed
	if (gl_shaderstate.current != next_shader)
	{
		gl_shaderstate.current = next_shader;
		gl_shaderstate.program = next_shader->program;
		gl_shaderstate.type = shader_type;
		gl_shaderstate.changed = true;
	}

#ifdef HAVE_GLES2
	Shader_SetTransform();
#endif

	gl_shadersenabled = (shader->program != 0);
}

void Shader_UnSet(void)
{
	gl_shaderstate.current =  NULL;
	gl_shaderstate.type = 0;
	gl_shaderstate.program = 0;

#ifdef HAVE_GLES2
	if (gl_shadersenabled)
	{
		Shader_Set(SHADER_NONE);
		Shader_SetUniforms(NULL, NULL, NULL, NULL);
	}
#endif

	if (gl_shadersenabled && GLExtension_shaders)
		pglUseProgram(0);

	gl_shadersenabled = false;
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
		shader->program = 0;
	}
}

static void Shader_CompileError(const char *message, GLuint program, INT32 shadernum)
{
	GLchar *infoLog = NULL;
	GLint logLength;

	//if (program)
	{
		pglGetShaderiv(program, GL_INFO_LOG_LENGTH, &logLength);

		if (logLength)
		{
			infoLog = malloc(logLength);
			pglGetShaderInfoLog(program, logLength, NULL, infoLog);
		}
	}
	GL_MSG_Error("Shader_CompileProgram: %s (\"%s\")\n%s", message, HWR_GetShaderName(shadernum), (infoLog ? infoLog : ""));

	if (infoLog)
		free(infoLog);
}

//#define GL_SHADERSOURCES

boolean Shader_CompileProgram(gl_shader_t *shader, GLint i)
{
	GLuint gl_vertShader = 0;
	GLuint gl_fragShader = 0;
	GLint result;

	const GLchar *vert_shader = shader->vertex_shader;
	const GLchar *frag_shader = shader->fragment_shader;

	if (shader->program)
		pglDeleteProgram(shader->program);

	if (!vert_shader && !frag_shader)
	{
		Shader_CompileError("Missing shaders for shader program", 0, i);
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
			Shader_CompileError("Error creating vertex shader", gl_vertShader, i);
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
			Shader_CompileError("Error creating fragment shader", gl_fragShader, i);
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
		Shader_CompileError("Error linking shader program", shader->program, i);
		pglDeleteProgram(shader->program);
		return false;
	}
	GL_DBG_Printf("Shader_CompileProgram() - Shader '%s' loaded!\n", HWR_GetShaderName(i));

	// 13062019
#define GETUNI(uniform) pglGetUniformLocation(shader->program, uniform);

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
	shader->uniforms[gluniform_scr_resolution] = GETUNI("scr_resolution");
	shader->uniforms[gluniform_leveltime] = GETUNI("leveltime");

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
	GLint i;

	if (!GLExtension_shaders)
		return false;

#ifdef GL_SHADERSOURCES
	for (i = 0; gl_shadersources[i].vertex && gl_shadersources[i].fragment; i++)
	{
		gl_shader_t *shader;

		if (i >= HWR_MAXSHADERS)
			break;

		shader = &gl_shaders[i];

		if (shader->program)
			pglDeleteProgram(shader->program);

		shader->program = 0;

		if (!Shader_CompileProgram(shader, i))
		{
			shader->program = 0;
#ifdef HAVE_GLES2
			if (i == SHADER_FLOOR)
				return false;
#endif
		}
	}
#endif

#if 1
#ifdef HAVE_GLES2
	//Shader_Set(SHADER_ALPHA_TEST);
	Shader_Set(SHADER_NONE); // STAR NOTE: normal
	pglUseProgram(gl_shaderstate.program);
	gl_shaderstate.changed = false;
#endif
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
			pglUniformMatrix4fv(shader->uniforms[gluniform_projection], 1, GL_FALSE, (GLfloat *)projMatrix);
	}

	if (memcmp(viewMatrix, shader->gles_viewMatrix, sizeof(fmatrix4_t)))
	{
		memcpy(shader->gles_viewMatrix, viewMatrix, sizeof(fmatrix4_t));
		if (shader->uniforms[gluniform_view] != -1)
			pglUniformMatrix4fv(shader->uniforms[gluniform_view], 1, GL_FALSE, (GLfloat *)viewMatrix);
	}

	if (memcmp(modelMatrix, shader->gles_modelMatrix, sizeof(fmatrix4_t)))
	{
		memcpy(shader->gles_modelMatrix, modelMatrix, sizeof(fmatrix4_t));
		if (shader->uniforms[gluniform_model] != -1)
			pglUniformMatrix4fv(shader->uniforms[gluniform_model], 1, GL_FALSE, (GLfloat *)modelMatrix);
	}
}
#endif

void Shader_SetUniforms(FSurfaceInfo *Surface, GLRGBAFloat *poly, GLRGBAFloat *tint, GLRGBAFloat *fade)
{
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

#else

boolean gl_shadersenabled = false;
hwdshaderstage_t gl_allowshaders = 0;

gl_shader_t gl_shaders[HWR_MAXSHADERS];
gl_shader_t gl_fallback_shader;

boolean Shader_Compile(void)
{
	return false;
}

void GLShader_SetInfo(hwdshaderinfo_t info, INT32 value)
{
	(void)info;
	(void)value;
}

void Shader_UnSet(void)
{
	gl_shadersenabled = false;
}

void Shader_SetUniforms(FSurfaceInfo *Surface, GLRGBAFloat *poly, GLRGBAFloat *tint, GLRGBAFloat *fade)
{
	(void)Surface;
	(void)poly;
	(void)tint;
	(void)fade;
}

#endif // GL_SHADERS
