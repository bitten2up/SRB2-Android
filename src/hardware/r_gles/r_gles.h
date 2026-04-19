// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1998-2020 by Sonic Team Junior.
// Copyright (C) 2020-2023 by SRB2 Mobile Project.
// Copyright (C) 2023-2025 by Bitten2Up.
// Copyright (C) 2025 by StarManiaKG.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file r_gles.h
/// \brief OpenGL ES API for Sonic Robo Blast 2

#ifndef _R_GLES_H_
#define _R_GLES_H_

#define GL_GLEXT_PROTOTYPES
#undef DRIVER_STRING

#ifdef HAVE_GLES2
    #include <GLES2/gl2.h>
    #include <GLES2/gl2ext.h>
    #define DRIVER_STRING "OpenGL ES 2.0"
#else
    #include <GLES/gl.h>
    #include <GLES/glext.h>
    #define DRIVER_STRING "OpenGL ES 1.1"
#endif

#define _CREATE_DLL_ // necessary for Unix AND Windows

// ==========================================================================
//                                                                DEFINITIONS
// ==========================================================================

#include "../../doomdef.h"
#include "../../z_zone.h"
#include "../hw_drv.h"

#undef DEBUG_TO_FILE

#include "../r_glcommon/r_glcommon.h"

#endif // _R_GLES_H_
