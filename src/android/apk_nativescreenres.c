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
/// \brief Native screen rendering for SRB2
/// \todo remove, we have any-res now

#include "apk_nativescreenres.h"
#include "apk_main.h"

#include "../i_video.h"
#include "../r_main.h"
#include "../screen.h"
#include "../v_video.h"

#ifdef NATIVESCREENRES

#define RESDIVFACTOR (1.0f / 16.0f)

// =========================================================================
//                                COMMANDS
// =========================================================================

// STAR NOTE: by the way my plan is to remove cv_nativeres
// maybe the whole def too, but not everything here

static void SCR_ToggleNativeRes(void);
static void SCR_NativeResDivChanged(void);
static void SCR_NativeResAutoChanged(void);

static CV_PossibleValue_t nativeresdiv_cons_t[] = {{FRACUNIT, "MIN"}, {20 * FRACUNIT, "MAX"}, {0, NULL}};
static CV_PossibleValue_t nativerescompare_cons_t[] = {{0, "Width"}, {1, "Height"}, {0, NULL}};

#define NATIVERES_CVAR_FLAGS(name, default, possiblevalue, func, flags) CVAR_INIT (name, default, (CV_CALL | CV_SAVE | CV_NOINIT | flags), possiblevalue, func)
#define NATIVERES_CVAR_CALL(name, default, possiblevalue, func) NATIVERES_CVAR_FLAGS(name, default, possiblevalue, func, 0)
#define NATIVERES_CVAR(name, default, possiblevalue) NATIVERES_CVAR_CALL(name, default, possiblevalue, SCR_ToggleNativeRes)

consvar_t cv_nativeres = NATIVERES_CVAR("nativeres", "On", CV_OnOff);
consvar_t cv_nativeresdiv = NATIVERES_CVAR_FLAGS("nativeresdiv", "1", nativeresdiv_cons_t, SCR_NativeResDivChanged, CV_FLOAT);
consvar_t cv_nativeresauto = NATIVERES_CVAR_CALL("nativeresauto", "On", CV_OnOff, SCR_NativeResAutoChanged);
consvar_t cv_nativerescompare = NATIVERES_CVAR("nativerescompare", "Height", nativerescompare_cons_t);

// =========================================================================
//                            SCREEN ROUTINES
// =========================================================================

boolean SCR_NativeRes_IsValidResolution(INT32 width, INT32 height)
{
	if (width < BASEVIDWIDTH || width > MAXVIDWIDTH) return false;
	if (height < BASEVIDHEIGHT || height > MAXVIDHEIGHT) return false;
	return true;
}

// sets the resolution as the new default to be saved in the config file
void SCR_NativeRes_SetDefaultMode(INT32 width, INT32 height)
{
	if (!SCR_NativeRes_IsValidResolution(width, height)) return;
	CV_SetValue((cv_fullscreen.value ? &cv_scr_width : &cv_scr_width_w), width);
	CV_SetValue((cv_fullscreen.value ? &cv_scr_height : &cv_scr_height_w), height);
}

// Set the mode number based on the resolution saved in the config
void SCR_NativeRes_SetModeFromConfig(void)
{
	INT32 width = (cv_fullscreen.value ? cv_scr_width.value : cv_scr_width_w.value);
	INT32 height = (cv_fullscreen.value ? cv_scr_height.value : cv_scr_height_w.value);
	SCR_ChangeResolution(width, height, true);
}

void SCR_NativeRes_CheckMode(void)
{
	INT32 w, h;

	VID_GetNativeResolution(&w, &h);
	if (w || h)
		SCR_NativeRes_SetMaxDivider(SCR_NativeRes_GetMaxDivider(w, h));

	if (cv_nativeresauto.value)
		android_data.scr_resdiv = SCR_NativeRes_GetDivider(w, h);
	else
		android_data.scr_resdiv = FixedToFloat(cv_nativeresdiv.value);
}

void SCR_NativeRes_SetDivider(float div)
{
	char f[16];

	if (!cv_nativeresauto.value) return;

	snprintf(f, sizeof(f), "%.6f", ((div > 0) ? div : android_data.scr_resdiv));
	CV_StealthSet(&cv_nativeresdiv, f);
}

void SCR_NativeRes_ResetDivider(void)
{
	char f[9];
	float resdiv = atof(cv_nativeresdiv.defaultvalue);

	android_data.scr_resdiv = resdiv;

	snprintf(f, sizeof(f), "%.6f", resdiv);
	CV_StealthSet(&cv_nativeresdiv, cv_nativeresdiv.defaultvalue);
}

static void SCR_ToggleNativeRes(void)
{
#if 1
	// off by default
	SCR_NativeRes_SetModeFromConfig();
#endif
	android_data.scr_resdiv = FixedToFloat(cv_nativeresdiv.value);
}

static void SCR_NativeResDivChanged(void)
{
	CV_StealthSetValue(&cv_nativeresauto, 0);
	//CV_StealthSetValue(&cv_nativeres, 1);
	SCR_ToggleNativeRes();

#if 0
	INT32 w = (INT32)((float)w / android_data.scr_resdiv);
	INT32 h = (INT32)((float)h / android_data.scr_resdiv);

	//INT32 w = (INT32)((float)vid.change.width / android_data.scr_resdiv);
	//INT32 h = (INT32)((float)vid.change.height / android_data.scr_resdiv);

	//INT32 w = (INT32)((float)vid.width / android_data.scr_resdiv);
	//INT32 h = (INT32)((float)vid.height / android_data.scr_resdiv);

	//VID_SetSize(w, h); // STAR NOTE: Breaks the game on startup lol
	//SCR_ChangeResolution(w, h, true); // STAR NOTE: Breaks the game on startup lol
	SCR_SetWindowSize(w, h, true);
#endif
}

static void SCR_NativeResAutoChanged(void)
{
	INT32 w = 0, h = 0;

	if (!android_data.scr_startupmodeset)
		return;

	if (cv_nativeresauto.value)
	{
		char f[16];

		// Set for next resolution change
		VID_GetNativeResolution(&w, &h);
		android_data.scr_resdiv = SCR_NativeRes_GetDivider(w, h);

		// Stealth change current resolution divider variable
		snprintf(f, sizeof(f), "%.6f", android_data.scr_resdiv);
		CV_StealthSet(&cv_nativeresdiv, f);
	}
	else
		SCR_NativeRes_ResetDivider();

	//if (cv_nativeres.value)
		SCR_NativeRes_SetModeFromConfig();
}

static INT32 SCR_CalcDup(INT32 width, INT32 height)
{
	INT32 dupx = max(1, width / BASEVIDWIDTH);
	INT32 dupy = max(1, height / BASEVIDHEIGHT);
	if (!cv_nativerescompare.value)
		return ((dupx >= dupy) ? dupx : dupy);
	else
		return ((dupx < dupy) ? dupx : dupy);
}

float SCR_NativeRes_GetDivider(INT32 width, INT32 height)
{
	if (cv_nativeresauto.value)
	{
		float w = (float)width;
		float h = (float)height;
		float wsize, hsize;
		float div = 1.0f;

		while (true)
		{
			INT32 iw, ih;
			INT32 dup, corner;

			wsize = (w / div);
			hsize = (h / div);

			iw = (INT32)wsize;
			ih = (INT32)hsize;

			dup = SCR_CalcDup(iw, ih);
			corner = (iw - (BASEVIDWIDTH * dup)) / 2;

			if (corner < iw / 5)
				break;
			if (wsize <= BASEVIDWIDTH || hsize <= BASEVIDHEIGHT)
				break;

			div += 0.25f;
		}
		return min(div, FixedToFloat(nativeresdiv_cons_t[1].value));
	}
	return FixedToFloat(cv_nativeresdiv.value);
}

float SCR_NativeRes_GetMaxDivider(INT32 nw, INT32 nh)
{
	float w, h;
	float div = 1.0f;

	if (!nw || !nh)
		VID_GetNativeResolution(&nw, &nh);

	w = (float)nw;
	h = (float)nh;

	while (true)
	{
		w = ((float)nw / div);
		h = ((float)nh / div);
		if (w <= (INT32)BASEVIDWIDTH || h <= (INT32)BASEVIDHEIGHT)
			break;
		div += RESDIVFACTOR;
	}
	return div;
}

void SCR_NativeRes_SetMaxDivider(float max)
{
	nativeresdiv_cons_t[1].value = FloatToFixed(max);
}

#endif // NATIVESCREENRES
