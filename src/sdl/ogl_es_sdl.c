// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 2014-2021 by Sonic Team Junior.
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
/// \file
/// \brief SDL specific part of the OpenGL ES API for SRB2

#ifdef HAVE_SDL
#define _MATH_DEFINES_DEFINED

#include "SDL.h"

#include "sdlmain.h"

#include "../doomdef.h"

#ifdef HWRENDER
#include "../hardware/r_gles/r_gles.h"
#include "ogl_es_sdl.h"
#include "../i_system.h"
#include "hwsym_sdl.h"
#include "../m_argv.h"

//PFNglClear pglClear;
//PFNglGetIntegerv pglGetIntegerv;
//PFNglGetString pglGetString;

/**	\brief SDL video display surface
*/
SDL_GLContext sdlglcontext = 0;

void *GLBackend_GetFunction(const char *proc)
{
	return SDL_GL_GetProcAddress(proc);
}

boolean GLBackend_Init(void)
{
	if (SDL_GL_LoadLibrary(NULL) != 0)
	{
		CONS_Alert(CONS_ERROR, "Could not load OpenGL Library: %s\nFalling back to Software mode.\n", SDL_GetError());
		return 0;
	}

	if (!GLBackend_InitContext())
		return false;

	return GLBackend_LoadFunctions();
}

/**	\brief	The OglSdlSurface function

	\param	w	width
	\param	h	height
	\param	isFullscreen	if true, go fullscreen

	\return	if true, changed video mode
*/
boolean OglSdlSurface(INT32 w, INT32 h)
{
	GLBackend_SetSurface(w, h);

	glanisotropicmode_cons_t[1].value = maximumAnisotropy;
	SDL_GL_SetSwapInterval(cv_vidwait.value ? 1 : 0);

	HWR_Startup();

	return true;
}

#ifdef HAVE_GL_FRAMEBUFFER
static boolean firstFramebuffer = false;
#endif

/**	\brief	The OglSdlFinishUpdate function

	\param	vidwait	wait for video sync

	\return	void
*/
void OglSdlFinishUpdate(boolean waitvbl)
{
	int sdlw, sdlh;

	static boolean oldwaitvbl = false;
	if (oldwaitvbl != waitvbl)
		SDL_GL_SetSwapInterval(waitvbl ? 1 : 0);

	oldwaitvbl = waitvbl;

	SDL_GetWindowSize(window, &sdlw, &sdlh);

#if 1
#ifdef HAVE_GL_FRAMEBUFFER
	GLFramebuffer_Disable();
	RenderToFramebuffer = FramebufferEnabled;

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
#else
	// STAR NOTE: hi opengles
	HWR_MakeScreenFinalTexture();
	HWR_DrawScreenFinalTexture(sdlw, sdlh);
	SDL_GL_SwapWindow(window);
#endif

	SDL_GL_SwapWindow(window);
	GClipRect(0, 0, realwidth, realheight, NZCLIP_PLANE);

	// Sryder:	We need to draw the final screen texture again into the other buffer in the original position so that
	//			effects that want to take the old screen can do so after this
#if 0
	DrawFinalScreenTexture(realwidth, realheight);
#else
	// STAR NOTE: hi opengles fixes
	HWR_DrawScreenFinalTexture(realwidth, realheight);
#endif
}

EXPORT void HWRAPI(OglSdlSetPalette) (RGBA_t *palette)
{
	size_t palsize = (sizeof(RGBA_t) * 256);
	// on a palette change, you have to reload all of the textures
	if (memcmp(&myPaletteData, palette, palsize))
	{
		memcpy(&myPaletteData, palette, palsize);
		GLTexture_Flush();
	}
}

#endif //HWRENDER
#endif //SDL
