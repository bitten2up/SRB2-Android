// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2022 by Jaime Ita Passos.
// Copyright (C) 2022-2025 by Bitten2Up.
// Copyright (C) 2025 by StarManiaKG.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  xtv_video.h
/// \brief Extra video data handling routines

#ifndef __XTV_VIDEO__
#define __XTV_VIDEO__

#include "../v_video.h"

void XTRA_V_OffsetPatch(fixed_t *x, fixed_t *y, fixed_t pscale, fixed_t vscale, INT32 scrn, patch_t *patch);
void V_GetPatchScreenRegion(fixed_t *x, fixed_t *y, fixed_t *w, fixed_t *h, fixed_t pscale, fixed_t vscale, INT32 scrn, patch_t *patch);

// Draws a scaled string.
void V_DrawScaledString(fixed_t x, fixed_t y, fixed_t scale, INT32 option, const char *string);

// Draws a scaled thin string.
void V_DrawScaledThinString(fixed_t x, fixed_t y, fixed_t scale, INT32 option, const char *string);

#endif // __XTV_VIDEO__
