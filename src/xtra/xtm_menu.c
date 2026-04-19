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
#include "../m_misc.h" // FIL_ReadFileOK
#include "../z_zone.h"

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

char *XTRA_G_GetSaveGameSlot(UINT32 slot)
{
	char *current_savegame_name = NULL;
	SINT8 cur_file = 0;

	cursavegamename = savegamename[0];
	curliveeventbackup = liveeventbackup[0];
	APK_CHECK_FOR_STORAGE_ACCESS({ return NULL; })

	while (cur_file < APK_MAX_SAVE_PATHS)
	{
		if (marathonmode)
			current_savegame_name = liveeventbackup[cur_file];
		else
			current_savegame_name = savegamename[cur_file];
		current_savegame_name = va(current_savegame_name, slot);

		if (!FIL_ReadFileOK(current_savegame_name))
		{
			cur_file++;
			current_savegame_name = NULL;
			continue;
		}

		cursavegamename = savegamename[cur_file];
		curliveeventbackup = liveeventbackup[cur_file];
		break;
	}

	return current_savegame_name;
}

size_t XTRA_G_ReadSaveGameInfo(char *savename, UINT8 **savebuffer, UINT32 slot)
{
	SINT8 cur_file;
	size_t length = 0;

	for (cur_file = 0; cur_file < APK_MAX_SAVE_PATHS; cur_file++)
	{
		sprintf(savename, savegamename[cur_file], slot);
		length = FIL_ReadFile(savename, savebuffer);
		if (length)
			break;
	}

	return length;
}

boolean XTRA_M_OpenSaveFileSlot(FILE **handle, char *name, char *savegamepaths, SINT8 slot)
{
	name = savegamepaths;
	for (SINT8 path = 0; path < APK_MAX_SAVE_PATHS; path++)
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
