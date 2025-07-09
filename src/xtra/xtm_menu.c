// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2022 by Jaime Ita Passos.
// Copyright (C) 2023 by SRB2 Mobile Project.
// Copyright (C) 2022-2025 by Bitten2Up.
// Copyright (C) 2025 by StarManiaKG.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  xtm_menu.c
/// \brief Extra menu handling routines

#include "xtm_menu.h"

#include "../android/apk_main.h"
#include "../ts_main.h"

#include "../m_menu.h" // MAXSAVEGAMES

//
// CVAR HANDLING
//

#ifdef TOUCHINPUTS
void *XTRA_M_CVarSliding(const consvar_t *var)
{
	INT32 i;

	if (!touchscreenavailable || var == NULL)
		return NULL;

	for (i = 0; i < NUMTOUCHFINGERS; i++)
	{
		touchfinger_t *finger = &touchfingers[i];
		if (finger->pointer == var)
			return finger;
	}

	return NULL;
}
#endif

INT32 XTRA_M_CVarValue(const consvar_t *var)
{
#ifdef TOUCHINPUTS
    touchfinger_t *finger = XTRA_M_CVarSliding(var);
	if (touchscreenavailable && finger)
    {
        if (var->flags & CV_FLOAT)
            return FloatToFixed(finger->float_arr[0]);
        else
            return finger->int_arr[0];
    }
#endif
	return var->value;
}

const char *XTRA_M_LongestColorName(void)
{
	INT32 i = 1;
	size_t len, last = 0;
	const char *longest = NULL;

	for (; i < numskincolors; i++)
	{
		const char *str = NULL;

		if (!skincolors[i].accessible)
			continue;

		str = skincolors[i].name;
		len = strlen(str);

		if (len > last)
		{
			last = len;
			longest = str;
		}
	}

	return longest;
}

//
// SAVEFILE MENU
//

boolean XTRA_M_OpenSaveFileSlot(FILE **handle, char *name, char *savegamepaths, SINT8 slot)
{
    SINT8 path;

	name = savegamepaths;
	for (path = 0; path < APK_MAX_SAVE_PATHS; path++)
	{
		snprintf(name, SAVEGAMENAMELEN, savegamename[path], slot);
		name[SAVEGAMENAMELEN - 1] = '\0';

		(*handle) = fopen(name, "rb");
		if ((*handle) == NULL)
			continue;

		fclose(*handle);
		return true;
	}
	return false;
}
