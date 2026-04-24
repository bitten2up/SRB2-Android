// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
//
// Copyright (C) 1998-2020 by Sonic Team Junior.
// Copyright (C) 2020-2023 by SRB2 Mobile Project.
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
/// \file r_opengl.h
/// \brief OpenGL API for Sonic Robo Blast 2

#ifndef _R_OPENGL_H_
#define _R_OPENGL_H_

#include "../r_glcommon/r_glcommon.h"

// ==========================================================================
//                                                                DEFINITIONS
// ==========================================================================

#undef DEBUG_TO_FILE            // maybe defined in previous *.h
#define DEBUG_TO_FILE           // output debugging msgs to ogllog.txt

// todo: find some way of getting SDL to log to ogllog.txt, without
// interfering with r_opengl.dll
#ifdef HAVE_SDL
#undef DEBUG_TO_FILE
#endif
//#if defined(HAVE_SDL) && !defined(_DEBUG)
//#undef DEBUG_TO_FILE
//#endif

// ==========================================================================
//                                                                     PROTOS
// ==========================================================================

#ifdef USE_WGL_SWAP
typedef BOOL (APIENTRY *PFNWGLEXTSWAPCONTROLPROC) (int);
typedef int (APIENTRY *PFNWGLEXTGETSWAPINTERVALPROC) (void);
extern PFNWGLEXTSWAPCONTROLPROC wglSwapIntervalEXT;
extern PFNWGLEXTGETSWAPINTERVALPROC wglGetSwapIntervalEXT;
#endif

#endif
