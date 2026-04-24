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
/// \file  apk_nativescreenres.c
/// \brief Native screen rendering routines

#ifndef __APK_NATIVESCREENRES__
#define __APK_NATIVESCREENRES__

#include "../command.h"

#ifdef NATIVESCREENRES

extern consvar_t cv_nativeres;
extern consvar_t cv_nativeresdiv, cv_nativeresauto;
extern consvar_t cv_nativerescompare;

boolean SCR_NativeRes_IsValidResolution(INT32 width, INT32 height);
void SCR_NativeRes_SetDefaultMode(INT32 width, INT32 height);
void SCR_NativeRes_SetModeFromConfig(void);

void SCR_NativeRes_CheckMode(void);
float SCR_NativeRes_GetDivider(INT32 width, INT32 height);

float SCR_NativeRes_GetMaxDivider(INT32 nw, INT32 nh);
void SCR_NativeRes_SetMaxDivider(float max);

void SCR_NativeRes_SetDivider(float div);
void SCR_NativeRes_ResetDivider(void);

#endif

#endif // __APK_NATIVESCREENRES__
