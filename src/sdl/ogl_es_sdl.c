// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 2014-2023 by Sonic Team Junior.
// Copyright (C) 2023-2025 by Bitten2Up.
// Copyright (C) 2025 by StarManiaKG.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------
/// \file sdl/ogl_es_sdl.c
/// \brief SDL specific part of the OpenGL-ES API for SRB2

#if defined (HAVE_SDL) && defined (HWRENDER)

#include "ogl_sdl.h"
#include "hwsym_sdl.h"

#include "../hardware/r_gles/r_gles.h"
#include "../i_system.h"
#include "../m_argv.h"

void *GLUhandle = NULL;
SDL_GLContext sdlglcontext = 0;

#ifdef HAVE_GL_FRAMEBUFFER
static boolean firstFramebuffer = false;
#endif

/**	\brief SDL video display surface
*/

void *GLBackend_GetFunction(const char *proc)
{
	return SDL_GL_GetProcAddress(proc);
}

boolean GLBackend_Init(void)
{
#ifndef STATIC_OPENGL
	const char *OGLLibname = NULL;

	if (M_CheckParm("-OGLlib") && M_IsNextParm())
		OGLLibname = M_GetNextParm();

	if (SDL_GL_LoadLibrary(OGLLibname) != 0)
	{
		CONS_Alert(CONS_ERROR, "Could not load OpenGL Library: %s\n" "Falling back to Software mode.\n", SDL_GetError());
		if (!M_CheckParm("-OGLlib"))
			CONS_Printf("If you know what is the OpenGL library's name, use -OGLlib\n");
		return false;
	}
#endif

	if (!GLBackend_InitContext())
		return false;
	if (!GLBackend_LoadCommonFunctions())
		return false;
	return GLBackend_LoadFunctions();
}

/**	\brief	The OglSdlFinishUpdate function

	\param	vidwait	wait for video sync

	\return	void
*/
void OglSdlFinishUpdate(boolean waitvbl)
{
	int sdlw, sdlh;
	static boolean oldwaitvbl = false;

	if (oldwaitvbl != waitvbl)
	{
		SDL_GL_SetSwapInterval(waitvbl ? 1 : 0);
	}
	oldwaitvbl = waitvbl;

	SDL_GetWindowSize(window, &sdlw, &sdlh);

#ifdef HAVE_GL_FRAMEBUFFER
	GLFramebuffer_Disable();
	RenderToFramebuffer = FramebufferEnabled;
#endif

	HWR_MakeScreenFinalTexture();
	HWR_DrawScreenFinalTexture(sdlw, sdlh);
	SDL_GL_SwapWindow(window);

#ifdef HAVE_GL_FRAMEBUFFER
	if (RenderToFramebuffer)
	{
		// I have no idea why I have to do this.
		if (!firstFramebuffer)
		{
			GLBackend_SetBlend(PF_Translucent|PF_Occlude|PF_Masked);
			firstFramebuffer = true;
		}

		GLFramebuffer_Enable();
	}
#endif

	GClipRect(0, 0, realwidth, realheight, NZCLIP_PLANE);

	// Sryder:	We need to draw the final screen texture again into the other buffer in the original position so that
	//			effects that want to take the old screen can do so after this
	// Generic2 has the screen image without palette rendering brightness adjustments.
	// Using that here will prevent brightness adjustments being applied twice.
	DrawScreenTexture(HWD_SCREENTEXTURE_GENERIC2, NULL, 0);
}

#endif // defined (HAVE_SDL) && defined (HWRENDER)
