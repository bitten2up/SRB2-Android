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
extern consvar_t cv_nativeresfov, cv_nativerescompare;

void APK_SCR_SetModeFromConfig(void);

void SCR_CheckNativeMode(void);
float SCR_GetNativeResDivider(INT32 width, INT32 height);

float SCR_GetMaxNativeResDivider(INT32 nw, INT32 nh);
void SCR_SetMaxNativeResDivider(float max);

void SCR_ResetNativeResDivider(void);

void APK_R_GetNativeResFov(fixed_t *fov);

#endif

#endif // __APK_NATIVESCREENRES__
