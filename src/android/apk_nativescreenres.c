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

#include "apk_nativescreenres.h"
#include "apk_main.h"

#include "../i_video.h"
#include "../r_main.h"
#include "../screen.h"
#include "../v_video.h"

#ifdef NATIVESCREENRES

#if 0 // bitten keeping just in case
	// Size of statusbar.
	#define ST_HEIGHT 32
	#define ST_WIDTH 320
#endif

#define RESDIVFACTOR (1.0f / 16.0f)

// =========================================================================
//                                COMMANDS
// =========================================================================

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
consvar_t cv_nativeresfov = NATIVERES_CVAR_CALL("nativeresfov", "On", CV_OnOff, R_SetViewSize);
consvar_t cv_nativerescompare = NATIVERES_CVAR("nativerescompare", "Height", nativerescompare_cons_t);

// =========================================================================
//                            SCREEN ROUTINES
// =========================================================================

// Set the mode number based on the resolution saved in the config
void APK_SCR_SetModeFromConfig(void)
{
	if (cv_fullscreen.value)
		setmodeneeded = VID_GetModeForSize(cv_scr_width.value, cv_scr_height.value);
	else
		setmodeneeded = VID_GetModeForSize(cv_scr_width_w.value, cv_scr_height_w.value);
	setmodeneeded++;
}

void SCR_CheckNativeMode(void)
{
	INT32 w, h;

	VID_GetNativeResolution(&w, &h);

	if (w || h)
		SCR_SetMaxNativeResDivider(SCR_GetMaxNativeResDivider(w, h));

	if (cv_nativeresauto.value)
		android_data.scr_resdiv = SCR_GetNativeResDivider(w, h);
	else
		android_data.scr_resdiv = FixedToFloat(cv_nativeresdiv.value);
}

void SCR_ResetNativeResDivider(void)
{
	float resdiv = atof(cv_nativeresdiv.defaultvalue);
	char f[9];

	android_data.scr_resdiv = resdiv;

	snprintf(f, sizeof(f), "%.6f", resdiv);
	CV_StealthSet(&cv_nativeresdiv, cv_nativeresdiv.defaultvalue);
}

static void SCR_ToggleNativeRes(void)
{
	INT32 mode;

	if (cv_fullscreen.value)
		mode = VID_GetModeForSize(cv_scr_width.value, cv_scr_height.value);
	else
		mode = VID_GetModeForSize(cv_scr_width_w.value, cv_scr_height_w.value);

	if (mode == -1)
		mode = VID_GetModeForSize(BASEVIDWIDTH, BASEVIDHEIGHT);

	setmodeneeded = mode + 1;
	android_data.scr_resdiv = FixedToFloat(cv_nativeresdiv.value);
}

static void SCR_NativeResDivChanged(void)
{
	CV_StealthSetValue(&cv_nativeresauto, 0);
	CV_StealthSetValue(&cv_nativeres, 1);
	SCR_ToggleNativeRes();
}

static void SCR_NativeResAutoChanged(void)
{
	if (!android_data.scr_startupmodeset)
		return;

	if (cv_nativeresauto.value)
	{
		INT32 w = 0, h = 0;
		char f[16];

		// Set for next resolution change
		VID_GetNativeResolution(&w, &h);
		android_data.scr_resdiv = SCR_GetNativeResDivider(w, h);

		// Stealth change current resolution divider variable
		snprintf(f, sizeof(f), "%.6f", android_data.scr_resdiv);
		CV_StealthSet(&cv_nativeresdiv, f);
	}
	else
		SCR_ResetNativeResDivider();

	if (cv_nativeres.value)
	{
		APK_SCR_SetModeFromConfig();
		if (setmodeneeded <= 0)
			setmodeneeded = VID_GetModeForSize(BASEVIDWIDTH, BASEVIDHEIGHT) + 1;
	}
}

static INT32 SCR_CalcDup(INT32 width, INT32 height)
{
	INT32 dupx = max(1, width / BASEVIDWIDTH);
	INT32 dupy = max(1, height / BASEVIDHEIGHT);

	if (!cv_nativerescompare.value)
		return (dupx >= dupy ? dupx : dupy);
	else
		return (dupx < dupy ? dupx : dupy);
}

float SCR_GetNativeResDivider(INT32 width, INT32 height)
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

float SCR_GetMaxNativeResDivider(INT32 nw, INT32 nh)
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
			return div;
		div += RESDIVFACTOR;
	}

	return div;
}

void SCR_SetMaxNativeResDivider(float max)
{
	nativeresdiv_cons_t[1].value = FloatToFixed(max);
}

void APK_R_GetNativeResFov(fixed_t *fov)
{
#if 0
	if (cv_nativeres.value && cv_nativeresfov.value)
	{
		fixed_t resmul = FloatToFixed(((float)vid.width / (float)vid.height));
		(*fov) = atan(tan(fov*M_PI/360)*(resmul*0.7))*360/M_PI;
	}
#else
	if (cv_nativeres.value && cv_nativeresfov.value)
	{
		fixed_t resmul = FixedDiv(vid.width * FRACUNIT, vid.height * FRACUNIT);
		if (resmul > FRACUNIT)
			fovtan = FixedMul(fovtan, (7*resmul/10));
		(*fov) = resmul;
	}
#endif
}

#endif // NATIVESCREENRES
