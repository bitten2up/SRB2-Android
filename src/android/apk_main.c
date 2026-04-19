// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2022 by Jaime Ita Passos.
// Copyright (C) 2020-2023 by SRB2 Mobile Project.
// Copyright (C) 2023-2025 by Bitten2Up.
// Copyright (C) 2025 by StarManiaKG.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  apk_main.c
/// \brief Android Operating System

#ifdef HAVE_ZLIB
#include <zlib.h>
#endif

#define ZWAD
#ifdef ZWAD
#include <errno.h>
#include "../lzf.h"
#endif

#include "apk_main.h"
#include "../ts_draw.h"
#include "../w_handle.h"

#include "../doomstat.h"
#include "../filesrch.h"
#include "../d_main.h"
#include "../g_game.h"
#include "../lua_hook.h"
#include "../m_misc.h"
#include "../r_main.h"
#include "../v_video.h"
#include "../z_zone.h"

static CV_PossibleValue_t liveshudpos_cons_t[] = {{0, "Bottom left"}, {1, "Top right"}, {2, "Automatic"}, {0, NULL}};
consvar_t cv_android_liveshudpos = CVAR_INIT ("liveshudpos", "Automatic", CV_SAVE, liveshudpos_cons_t, NULL);

consvar_t cv_android_thinkless = CVAR_INIT(
	"thinkless",
#ifdef MOBILE_PLATFORM
	"On",
#else
	"Off",
#endif
	CV_SAVE, CV_OnOff,
	NULL
);

joystickvector2_t android_joystickmovevectors[2];
joystickvector2_t android_joysticklookvectors[2];
#ifdef TOUCHINPUTS
joystickvector2_t android_touchmovevector;
#endif
#ifdef ACCELEROMETER
joystickvector2_t android_accelmovevector;
#endif

struct android_data_s android_data; // Misc. Android Stuff

//
// COMMAND CODE
//

// Returns the longest PossibleValue string for this CVar.
// Returns NULL if the CVar has no PossibleValue.
const char *APK_CV_LongestPossibleValue(consvar_t *var)
{
	CV_PossibleValue_t *PossibleValue = var->PossibleValue;
	INT32 i, num;
	size_t last = 0;
	const char *longest = NULL;

	if (PossibleValue == NULL)
		return NULL;

	// Count how many PossibleValues there are.
	for (num = 0; PossibleValue[num].strvalue; num++);

	// It's a numeric range -- therefore, return the max value as a string.
	if (num == 2 && !strcmp("MIN", PossibleValue[0].strvalue) && !strcmp("MAX", PossibleValue[1].strvalue))
	{
		static char dig[10]; // Amount of digits in the max value of CV_Unsigned/CV_Natural, plus one
		M_snprintf(dig, sizeof dig, "%d", PossibleValue[1].value);
		return dig;
	}

	// Otherwise, the PossibleValues are strings.
	for (i = 0; i < num; i++)
	{
		const char *str = PossibleValue[i].strvalue;
		size_t len = strlen(str);
		if (len > last)
		{
			last = len;
			longest = str;
		}
	}

	return longest;
}

//
// GAME CODE
//

char *APK_G_LiveEventHasBackup(void)
{
	if (FIL_FileExists(liveeventbackup[0]))
		return liveeventbackup[0];
#ifdef USE_SAVEGAME_PATHS
	if (FIL_FileExists(liveeventbackup[1]))
		return liveeventbackup[1];
#endif
	return NULL;
}


// Returns true if you can switch your viewpoint to this player.
boolean APK_G_CanViewpointSwitchToPlayer(player_t *player)
{
	player_t *myself = &players[consoleplayer];

	if (player->spectator)
		return false;

	if (G_GametypeHasTeams())
	{
		if (myself->ctfteam && player->ctfteam != myself->ctfteam)
			return false;
	}
	else if (gametyperules & GTR_HIDEFROZEN)
	{
		if (myself->pflags & PF_TAGIT)
			return false;
	}
	// Other Tag-based gametypes?
	else if (G_TagGametype())
	{
		if (!myself->spectator && (myself->pflags & PF_TAGIT) != (player->pflags & PF_TAGIT))
			return false;
	}
	else if (G_GametypeHasSpectators() && G_RingSlingerGametype())
	{
		if (!myself->spectator)
			return false;
	}

	return true;
}

// Returns true if you can switch your viewpoint at all.
boolean APK_G_CanViewpointSwitch(boolean luahook)
{
	// ViewpointSwitch Lua hook.
	UINT8 canSwitchView = 0;
	INT32 checkdisplayplayer = displayplayer;

	if (splitscreen || !netgame)
		return false;

	if (D_NumPlayers() <= 1)
		return false;

	do
	{
		checkdisplayplayer++;
		if (checkdisplayplayer == MAXPLAYERS)
			checkdisplayplayer = 0;

		if (!playeringame[checkdisplayplayer])
			continue;

		// Call ViewpointSwitch hooks here.
		if (luahook)
		{
			canSwitchView = LUA_HookViewpointSwitch(&players[consoleplayer], &players[checkdisplayplayer], false);
			if (canSwitchView == 1) // Set viewpoint to this player
				break;
			else if (canSwitchView == 2) // Skip this player
				continue;
		}

		if (!APK_G_CanViewpointSwitchToPlayer(&players[checkdisplayplayer]))
			continue;

		break;
	} while (checkdisplayplayer != consoleplayer);

	// had any change??
	return (checkdisplayplayer != displayplayer);
}

// Handles the camera toggle key being pressed.
boolean APK_G_ToggleChaseCam(UINT8 player, boolean set_chasecam)
{
	if (!android_data.cam1_toggledelay && !player)
	{
		// Player 1
		android_data.cam1_toggledelay = NEWTICRATE / 7;
		if (set_chasecam)
			CV_SetValue(&cv_chasecam, cv_chasecam.value ? 0 : 1);
		return true;
	}
	if (!android_data.cam2_toggledelay && player)
	{
		// Player 2
		android_data.cam2_toggledelay = NEWTICRATE / 7;
		if (set_chasecam)
			CV_SetValue(&cv_chasecam2, cv_chasecam2.value ? 0 : 1);
		return true;
	}
	return false;
}

//
// HEADS UP DISPLAY
//

void APK_HU_DrawTapAnywhere(tic_t tics, INT32 flags)
{
	const char *string;
	INT32 input = inputmethod;
	INT32 x, y;

	if (input == INPUTMETHOD_TOUCH)
		string = M_GetText("Tap anywhere!");
	else if (input == INPUTMETHOD_TVREMOTE)
		string = M_GetText("Press Center!");
	else
		string = M_GetText("Press any key!");

	if (!(tics/20 & 1))
	{
		x = (BASEVIDWIDTH - V_StringWidth(string, flags))>>1;
		y = BASEVIDHEIGHT - 24;
		V_DrawString(x, y, V_YELLOWMAP | flags, string);
	}
}

//
// OBJECT CODE
//

void APK_P_MainTicker(boolean run)
{
	if (run)
	{
		android_data.cam1_toggledelay--;
		android_data.cam2_toggledelay--;
	}
}

static inline boolean P_MobjDistanceCheck(mobj_t *mobj)
{
	fixed_t tx, ty, cx, cy;
	const fixed_t blocksize = 1024*FRACUNIT;
	const fixed_t range = 4;
	tx = mobj->x / blocksize;
	ty = mobj->y / blocksize;
	cx = viewx / blocksize;
	cy = viewy / blocksize;

	if (abs(tx-cx) > range || abs(ty-cy) > range)
		return false;
	return true;
}

boolean APK_P_ReduceMobjThinking(mobj_t *mobj)
{
	if (mobj->player)
		return false;

	if (!(cv_android_thinkless.value
		&& !(netgame || multiplayer)
		&& !(demoplayback || modeattacking || marathonmode || metalrecording))
	)
	{
		// No reduced thinking!
		return false;
	}

	if (!P_MobjDistanceCheck(mobj))
	{
		switch (mobj->type)
		{
			case MT_MACEPOINT:
			case MT_CHAINMACEPOINT:
			case MT_SPRINGBALLPOINT:
			case MT_CHAINPOINT:
			case MT_FIREBARPOINT:
			case MT_CUSTOMMACEPOINT:
			case MT_HIDDEN_SLING:
				// Always update movedir to prevent desyncing (in the traditional sense, not the netplay sense).
				mobj->movedir = (mobj->movedir + mobj->lastlook) & FINEMASK;
				/* FALLTHRU */
			default:
				return true;
		}
	}

	return false;
}

//
// STATUS BAR CODE
//

boolean APK_ST_UseAltLivesHUD(void)
{
#ifdef TOUCHINPUTS
	if (cv_android_liveshudpos.value == 2)
		return TS_CanDrawButtons();
#endif
	return (cv_android_liveshudpos.value == 1);
}

hudinfo_t *APK_ST_GetLivesHUDInfo(void)
{
	if (APK_ST_UseAltLivesHUD())
		return &hudinfo[ANDROID_HUD_LIVES];
	return &hudinfo[HUD_LIVES];
}

boolean APK_ST_AltLivesHUDEnabled(void)
{
	return (APK_ST_UseAltLivesHUD() && !modeattacking);
}

void APK_ST_SetInputPosition(INT32 *x, INT32 *y, INT32 *f, hudinfo_t **pos)
{
	if (APK_ST_UseAltLivesHUD())
	{
		// We can replace our previous HUD location with the inputs!
		(*pos) = &hudinfo[HUD_LIVES];
	}
	else
	{
		// Render the inputs above the lives!
		(*pos) = &hudinfo[HUD_INPUT];
	}
	(*x) = (*pos)->x;
	(*y) = hudinfo[HUD_INPUT].y;
	(*f) = hudinfo[HUD_INPUT].f;
}

//
// MISCELLANIOUS CODE
//

// Searches for a file in:
//   filename
//   srb2home/filename
//   srb2path/filename
//  ./filename
//   (possibly) Android assets
char *APK_M_FindFile(const char *filename)
{
	static char filenamebuf[4096];
	const char *paths[3] = {
		srb2home,
		srb2path,
		"."
	};

	strlcpy(filenamebuf, filename, sizeof filenamebuf);

	// That was easy...
	if (FIL_FileExists(filenamebuf))
		return filenamebuf;

	// Look in for the file the paths specified earlier.
	for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); i++)
	{
		snprintf(filenamebuf, sizeof filenamebuf, "%s" PATHSEP "%s", paths[i], filename);
		if (FIL_FileExists(filenamebuf))
			return filenamebuf;
	}

#if defined(__ANDROID__)
	// That didn't work. Let's just File_Open it directly and check if there's a handle
	strlcpy(filenamebuf, filename, sizeof filenamebuf);

	filehandle_t *handle = File_Open(filenamebuf, "rb", FILEHANDLE_SDL);
	if (handle)
	{
		File_Close(handle);
		return filenamebuf;
	}
#endif

	// Couldn't find anything
	return NULL;
}

//
// WADFILE CODE
//

#if defined(_MSC_VER)
#pragma pack(1)
#endif
typedef struct zend_s
{
	char signature[4];
	UINT16 diskpos;
	UINT16 cdirdisk;
	UINT16 diskentries;
	UINT16 entries;
	UINT32 cdirsize;
	UINT32 cdiroffset;
	UINT16 commentlen;
} ATTRPACK zend_t;

typedef struct zentry_s
{
	char signature[4];
	UINT16 version;
	UINT16 versionneeded;
	UINT16 flags;
	UINT16 compression;
	UINT16 modtime;
	UINT16 moddate;
	UINT32 CRC32;
	UINT32 compsize;
	UINT32 size;
	UINT16 namelen;
	UINT16 xtralen;
	UINT16 commlen;
	UINT16 diskstart;
	UINT16 attrint;
	UINT32 attrext;
	UINT32 offset;
} ATTRPACK zentry_t;

typedef struct zlentry_s
{
	char signature[4];
	UINT16 versionneeded;
	UINT16 flags;
	UINT16 compression;
	UINT16 modtime;
	UINT16 moddate;
	UINT32 CRC32;
	UINT32 compsize;
	UINT32 size;
	UINT16 namelen;
	UINT16 xtralen;
} ATTRPACK zlentry_t;
#if defined(_MSC_VER)
#pragma pack()
#endif

static boolean MagicIsWAD(char id[4])
{
	// Very likely a wad
	if (!memcmp(id, "IWAD", 4) || !memcmp(id, "PWAD", 4) || !memcmp(id, "ZWAD", 4) || !memcmp(id, "SDLL", 4))
		return true;
	return false;
}

/** Optimized pattern search in a file.
 */
static boolean ResFindSignature (void* handle, char endPat[], UINT32 startpos)
{
	char *s;
	int c;

	File_Seek(handle, startpos, SEEK_SET);
	s = endPat;

#if defined(__ANDROID__)
	while (true)
	{
		c = (unsigned char)(File_GetChar(handle));
		if (File_EOF(handle))
			break;
#else
	while((c = File_GetChar(handle)) != EOF)
	{
#endif
		if (*s != c && s > endPat) // No match?
			s = endPat; // We "reset" the counter by sending the s pointer back to the start of the array.
		if (*s == c)
		{
			s++;
			if (*s == 0x00) // The array pointer has reached the key char which marks the end. It means we have matched the signature.
			{
				return true;
			}
		}
	}
	return false;
}

/** Detect a file type.
 */
static restype_t ResourceFileDetect (filehandle_t *handle, const char* filename)
{
	char id[4];
	size_t read;

	// Read the first four bytes, then seek back
	read = File_Read(&id, 1, sizeof id, handle);

	File_Seek(handle, 0, SEEK_SET);

	if (read >= sizeof id)
	{
		if (MagicIsWAD(id))
			return RET_WAD;
		// Seems to be a zip (so, a pk3)
		else if (!memcmp(id, "PK\x03\x04", 4))
			return RET_PK3;
	}

	// Couldn't figure it out, let's just look at the filename
	if (!stricmp(&filename[strlen(filename) - 4], ".pk3") || !stricmp(&filename[strlen(filename) - 4], ".zip"))
		return RET_PK3;
	if (!stricmp(&filename[strlen(filename) - 4], ".soc"))
		return RET_SOC;
	if (!stricmp(&filename[strlen(filename) - 4], ".lua"))
		return RET_LUA;

	// I give up! Assume it's a WAD
	return RET_WAD;
}

/** Create a lumpinfo_t array for a PKZip file.
 */
static lumpinfo_t* ResGetLumpsZip (void* handle, UINT16* nlmp)
{
    zend_t zend;
    zentry_t zentry;
    zlentry_t zlentry;

	UINT16 numlumps = *nlmp;
	lumpinfo_t* lumpinfo;
	lumpinfo_t *lump_p;
	size_t i;

	char pat_central[] = {0x50, 0x4b, 0x01, 0x02, 0x00};
	char pat_end[] = {0x50, 0x4b, 0x05, 0x06, 0x00};

	// Look for central directory end signature near end of file.
	// Contains entry number (number of lumps), and central directory start offset.
	File_Seek(handle, 0, SEEK_END);
	if (!ResFindSignature(handle, pat_end, max(0, File_Tell(handle) - (22 + 65536))))
	{
		CONS_Alert(CONS_ERROR, "Missing central directory\n");
		return NULL;
	}

	File_Seek(handle, -4, SEEK_CUR);
	if (File_Read(&zend, 1, sizeof zend, handle) < sizeof zend)
	{
		CONS_Alert(CONS_ERROR, "Corrupt central directory (%s)\n", File_Error(handle));
		return NULL;
	}
	numlumps = zend.entries;

	lump_p = lumpinfo = Z_Malloc(numlumps * sizeof (*lumpinfo), PU_STATIC, NULL);

	File_Seek(handle, zend.cdiroffset, SEEK_SET);
	for (i = 0; i < numlumps; i++, lump_p++)
	{
		char* fullname;
		char* trimname;
		char* dotpos;

		if (File_Read(&zentry, 1, sizeof(zentry_t), handle) < sizeof(zentry_t))
		{
			CONS_Alert(CONS_ERROR, "Failed to read central directory (%s)\n", File_Error(handle));
			Z_Free(lumpinfo);
			return NULL;
		}
		if (memcmp(zentry.signature, pat_central, 4))
		{
			CONS_Alert(CONS_ERROR, "Central directory is corrupt\n");
			Z_Free(lumpinfo);
			return NULL;
		}

		lump_p->position = zentry.offset; // NOT ACCURATE YET: we still need to read the local entry to find our true position
		lump_p->disksize = zentry.compsize;
		lump_p->diskpath = NULL;
		lump_p->size = zentry.size;

		fullname = malloc(zentry.namelen + 1);
		if (File_GetString(fullname, zentry.namelen + 1, handle) != fullname)
		{
			CONS_Alert(CONS_ERROR, "Unable to read lumpname (%s)\n", File_Error(handle));
			Z_Free(lumpinfo);
			free(fullname);
			return NULL;
		}

		// Strip away file address and extension for the 8char name.
		if ((trimname = strrchr(fullname, '/')) != 0)
			trimname++;
		else
			trimname = fullname; // Care taken for root files.

		if ((dotpos = strrchr(trimname, '.')) == 0)
			dotpos = fullname + strlen(fullname); // Watch for files without extension.

		memset(lump_p->name, '\0', 9); // Making sure they're initialized to 0. Is it necessary?
		strncpy(lump_p->name, trimname, min(8, dotpos - trimname));
		lump_p->hash = quickncasehash(lump_p->name, 8);

		lump_p->longname = Z_Calloc(dotpos - trimname + 1, PU_STATIC, NULL);
		strlcpy(lump_p->longname, trimname, dotpos - trimname + 1);

		lump_p->fullname = Z_Calloc(zentry.namelen + 1, PU_STATIC, NULL);
		strncpy(lump_p->fullname, fullname, zentry.namelen);

		switch(zentry.compression)
		{
		case 0:
			lump_p->compression = CM_NOCOMPRESSION;
			break;
#ifdef HAVE_ZLIB
		case 8:
			lump_p->compression = CM_DEFLATE;
			break;
#endif
		case 14:
			lump_p->compression = CM_LZF;
			break;
		default:
			CONS_Alert(CONS_WARNING, "%s: Unsupported compression method\n", fullname);
			lump_p->compression = CM_UNSUPPORTED;
			break;
		}

		free(fullname);

		// skip and ignore comments/extra fields
		if (File_Seek(handle, zentry.xtralen + zentry.commlen, SEEK_CUR) != 0)
		{
			CONS_Alert(CONS_ERROR, "Central directory is corrupt\n");
			Z_Free(lumpinfo);
			return NULL;
		}
	}

	// Adjust lump position values properly
	for (i = 0, lump_p = lumpinfo; i < numlumps; i++, lump_p++)
	{
		// skip and ignore comments/extra fields
		if ((File_Seek(handle, lump_p->position, SEEK_SET) != 0) || (File_Read(&zlentry, 1, sizeof(zlentry_t), handle) < sizeof(zlentry_t)))
		{
			CONS_Alert(CONS_ERROR, "Local headers for lump %s are corrupt\n", lump_p->fullname);
			Z_Free(lumpinfo);
			return NULL;
		}

		lump_p->position += sizeof(zlentry_t) + zlentry.namelen + zlentry.xtralen;
	}

	*nlmp = numlumps;
	return lumpinfo;
}

/** Create a lumpinfo_t array for a WAD file.
 */
static lumpinfo_t* ResGetLumpsWad (void* handle, UINT16* nlmp, const char* filename)
{
	UINT16 numlumps = *nlmp;
	lumpinfo_t* lumpinfo;
	size_t i;
	INT32 compressed = 0;

	wadinfo_t header;
	lumpinfo_t *lump_p;
	filelump_t *fileinfo;
	void *fileinfov;

	// read the header
	if (File_Read(&header, 1, sizeof header, handle) < sizeof header)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Can't read wad header because %s\n"), File_Error(handle));
		return NULL;
	}

	if (memcmp(header.identification, "ZWAD", 4) == 0)
		compressed = 1;
	else if (memcmp(header.identification, "IWAD", 4) != 0
		&& memcmp(header.identification, "PWAD", 4) != 0
		&& memcmp(header.identification, "SDLL", 4) != 0)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Invalid WAD header\n"));
		return NULL;
	}

	header.numlumps = LONG(header.numlumps);
	header.infotableofs = LONG(header.infotableofs);

	// read wad file directory
	i = header.numlumps * sizeof (*fileinfo);
	fileinfov = fileinfo = malloc(i);
	if (File_Seek(handle, header.infotableofs, SEEK_SET) == -1
		|| File_Read(fileinfo, 1, i, handle) < i)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Corrupt wadfile directory (%s)\n"), File_Error(handle));
		free(fileinfov);
		return NULL;
	}

	numlumps = header.numlumps;

	// fill in lumpinfo for this wad
	lump_p = lumpinfo = Z_Malloc(numlumps * sizeof (*lumpinfo), PU_STATIC, NULL);
	for (i = 0; i < numlumps; i++, lump_p++, fileinfo++)
	{
		lump_p->position = LONG(fileinfo->filepos);
		lump_p->size = lump_p->disksize = LONG(fileinfo->size);
		lump_p->diskpath = NULL;
		if (compressed) // wad is compressed, lump might be
		{
			UINT32 realsize = 0;
			if (File_Seek(handle, lump_p->position, SEEK_SET)
				== -1 || File_Read(&realsize, 1, sizeof realsize,
				handle) < sizeof realsize)
			{
				I_Error("corrupt compressed file: %s; maybe %s", /// \todo Avoid the bailout?
					filename, File_Error(handle));
			}
			realsize = LONG(realsize);
			if (realsize != 0)
			{
				lump_p->size = realsize;
				lump_p->compression = CM_LZF;
			}
			else
			{
				lump_p->size -= 4;
				lump_p->compression = CM_NOCOMPRESSION;
			}

			lump_p->position += 4;
			lump_p->disksize -= 4;
		}
		else
			lump_p->compression = CM_NOCOMPRESSION;
		memset(lump_p->name, 0x00, 9);
		strncpy(lump_p->name, fileinfo->name, 8);
		lump_p->hash = quickncasehash(lump_p->name, 8);

		// Allocate the lump's long name.
		lump_p->longname = Z_Malloc(9 * sizeof(char), PU_STATIC, NULL);
		strncpy(lump_p->longname, fileinfo->name, 8);
		lump_p->longname[8] = '\0';

		// Allocate the lump's full name.
		lump_p->fullname = Z_Malloc(9 * sizeof(char), PU_STATIC, NULL);
		strncpy(lump_p->fullname, fileinfo->name, 8);
		lump_p->fullname[8] = '\0';
	}
	free(fileinfov);
	*nlmp = numlumps;
	return lumpinfo;
}

wadfile_t *APK_W_LoadResourceFile(const char *filename, fhandletype_t handletype)
{
	void *handle;
	lumpinfo_t *lumpinfo = NULL;
	restype_t type;
	UINT16 numlumps = 0;

	// open wad file
	if ((handle = File_Open(filename, "rb", handletype)) == NULL)
	{
		CONS_Printf(M_GetText("Errors occurred while loading %s.\n"), filename);
		return NULL;
	}

	switch (type = ResourceFileDetect(handle, filename))
	{
	case RET_PK3:
		lumpinfo = ResGetLumpsZip(handle, &numlumps);
		break;
	case RET_WAD:
		lumpinfo = ResGetLumpsWad(handle, &numlumps, filename);
		break;
	default:
		CONS_Alert(CONS_ERROR, "Unsupported file format\n");
	}

	if (lumpinfo == NULL)
	{
		File_Close(handle);
		CONS_Printf(M_GetText("Errors occurred while loading %s.\n"), filename);
		return NULL;
	}

	wadfile_t *wadfile = Z_Malloc(sizeof (*wadfile), PU_STATIC, NULL);
	wadfile->filename = Z_StrDup(filename);
	wadfile->path = NULL;
	wadfile->type = type;
	wadfile->handle = handle;
	wadfile->numlumps = numlumps;
	wadfile->foldercount = 0;
	wadfile->lumpinfo = lumpinfo;
	wadfile->important = false;
	File_Seek(handle, 0, SEEK_END);
	wadfile->filesize = (unsigned)File_Tell(handle);

	// Irrelevant.
	memset(wadfile->md5sum, 0x00, 16);

	Z_Calloc(numlumps * sizeof (*wadfile->lumpcache), PU_STATIC, &wadfile->lumpcache);
	Z_Calloc(numlumps * sizeof (*wadfile->patchcache), PU_STATIC, &wadfile->patchcache);

	return wadfile;
}

void APK_W_DeleteResourceFile(wadfile_t *wad)
{
	if (!wad)
		return;

	if (wad->handle)
		File_Close(wad->handle);
	Z_Free(wad->filename);
	if (wad->path)
		Z_Free(wad->path);

	while (wad->numlumps--)
	{
		Z_Free(wad->lumpcache[wad->numlumps]);
		if (wad->patchcache[wad->numlumps])
			Patch_Free(wad->patchcache[wad->numlumps]);
		if (wad->lumpinfo[wad->numlumps].diskpath)
			Z_Free(wad->lumpinfo[wad->numlumps].diskpath);
		Z_Free(wad->lumpinfo[wad->numlumps].longname);
		Z_Free(wad->lumpinfo[wad->numlumps].fullname);
	}

	Z_Free(wad->lumpcache);
	Z_Free(wad->patchcache);
	Z_Free(wad->lumpinfo);
	Z_Free(wad);
}

UINT16 APK_Resource_CheckNumForName(wadfile_t *wad, const char *name)
{
	lumpinfo_t *lump_p = wad->lumpinfo;
	for (UINT16 i = 0; i < wad->numlumps; i++, lump_p++)
		if (!strcmp(lump_p->fullname, name))
			return i;

	// not found.
	return INT16_MAX;
}

void *APK_Resource_CacheLumpNum(wadfile_t *wad, UINT16 lump, INT32 tag)
{
	if (lump >= wad->numlumps)
		return NULL;

	lumpcache_t *lumpcache = wad->lumpcache;
	if (!lumpcache[lump])
	{
		void *ptr = Z_Malloc(APK_Resource_LumpLength(wad, lump), tag, &lumpcache[lump]);
		APK_Resource_ReadLumpHeader(wad, lump, ptr, 0, 0);  // read the lump in full
	}
	else
		Z_ChangeTag(lumpcache[lump], tag);

	return lumpcache[lump];
}

void *APK_Resource_CacheLumpName(wadfile_t *wad, const char *name, INT32 tag)
{
	UINT16 lumpnum = APK_Resource_CheckNumForName(wad, name);
	if (lumpnum == INT16_MAX)
	{
		CONS_Alert(CONS_ERROR, "Resource file %s does not contain any lump named %s\n", wad->filename, name);
		return NULL;
	}

	return APK_Resource_CacheLumpNum(wad, lumpnum, tag);
}

boolean APK_Resource_LumpExists(wadfile_t *wad, const char *name)
{
	return APK_Resource_CheckNumForName(wad, name) != INT16_MAX;
}

size_t APK_Resource_LumpLength(wadfile_t *wad, UINT16 lump)
{
	lumpinfo_t *l;

	if (lump >= wad->numlumps)
		return 0;

	l = wad->lumpinfo + lump;

	// Open the external file for this lump, if the WAD is a folder.
	if (wad->type == RET_FOLDER)
	{
		// pathisdirectory calls stat, so if anything wrong has happened,
		// this is the time to be aware of it.
		INT32 stat = pathisdirectory(l->diskpath);

		if (stat < 0)
		{
#ifndef AVOID_ERRNO
			if (direrror == ENOENT)
				I_Error("W_LumpLengthPwad: file %s doesn't exist", l->diskpath);
			else
				I_Error("W_LumpLengthPwad: could not stat %s: %s", l->diskpath, strerror(direrror));
#else
			I_Error("W_LumpLengthPwad: could not access %s", l->diskpath);
#endif
		}
		else if (stat == 1) // Path is a folder.
			return 0;
		else
		{
			FILE *handle = fopen(l->diskpath, "rb");
			if (handle == NULL)
				I_Error("W_LumpLengthPwad: could not open file %s", l->diskpath);

			fseek(handle, 0, SEEK_END);
			l->size = l->disksize = ftell(handle);
			fclose(handle);
		}
	}

	return l->size;
}

size_t APK_Resource_ReadLumpHeader(wadfile_t *wad, UINT16 lump, void *dest, size_t size, size_t offset)
{
	size_t lumpsize, bytesread;
	lumpinfo_t *l;
	void *handle = NULL;

	if (lump >= wad->numlumps)
		return 0;

	l = wad->lumpinfo + lump;

	// Open the external file for this lump, if the WAD is a folder.
	if (wad->type == RET_FOLDER)
	{
		// pathisdirectory calls stat, so if anything wrong has happened,
		// this is the time to be aware of it.
		INT32 stat = pathisdirectory(l->diskpath);

		if (stat < 0)
		{
#ifndef AVOID_ERRNO
			if (direrror == ENOENT)
				I_Error("APK_Resource_ReadLumpHeader: file %s doesn't exist", l->diskpath);
			else
				I_Error("APK_Resource_ReadLumpHeader: could not stat %s: %s", l->diskpath, strerror(direrror));
#else
			I_Error("APK_Resource_ReadLumpHeader: could not access %s", l->diskpath);
#endif
		}
		else if (stat == 1) // Path is a folder.
			return 0;
		else
		{
			handle = File_Open(l->diskpath, "rb", FILEHANDLE_STANDARD);
			if (handle == NULL)
				I_Error("APK_Resource_ReadLumpHeader: could not open file %s", l->diskpath);

			// Find length of file
			File_Seek(handle, 0, SEEK_END);
			l->size = l->disksize = File_Tell(handle);
		}
	}

	lumpsize = wad->lumpinfo[lump].size;

	// empty resource (usually markers like S_START, F_END ..)
	if (!lumpsize || lumpsize < offset)
	{
		if (wad->type == RET_FOLDER)
			File_Close(handle);
		return 0;
	}

	// zero size means read all the lump
	if (!size || size + offset > lumpsize)
		size = lumpsize - offset;

	// Let's get the raw lump data.
	// We setup the desired file handle to read the lump data.
	if (wad->type != RET_FOLDER)
		handle = wad->handle;
	File_Seek(handle, (long)(l->position + offset), SEEK_SET);

	// But let's not copy it yet. We support different compression formats on lumps, so we need to take that into account.
	switch (wad->lumpinfo[lump].compression)
	{
	case CM_NOCOMPRESSION:		// If it's uncompressed, we directly write the data into our destination, and return the bytes read.
		bytesread = File_Read(dest, 1, size, handle);
		if (wad->type == RET_FOLDER)
			fclose(handle);
#ifdef NO_PNG_LUMPS
		if (Picture_IsLumpPNG((UINT8 *)dest, bytesread))
			Picture_ThrowPNGError(l->fullname, wad->filename);
#endif
		return bytesread;
	case CM_LZF:		// Is it LZF compressed? Used by ZWADs.
		{
#ifdef ZWAD
			char *rawData; // The lump's raw data.
			char *decData; // Lump's decompressed real data.
			size_t retval; // Helper var, lzf_decompress returns 0 when an error occurs.

			rawData = Z_Malloc(l->disksize, PU_STATIC, NULL);
			decData = Z_Malloc(l->size, PU_STATIC, NULL);

			if (File_Read(rawData, 1, l->disksize, handle) < l->disksize)
				I_Error("wad %s, lump %d: cannot read compressed data", wad->filename, lump);
			retval = lzf_decompress(rawData, l->disksize, decData, l->size);
#ifndef AVOID_ERRNO
			if (retval == 0) // If this was returned, check if errno was set
			{
				// errno is a global var set by the lzf functions when something goes wrong.
				if (errno == E2BIG)
					I_Error("wad %s, lump %d: compressed data too big (bigger than %s)", wad->filename, lump, sizeu1(l->size));
				else if (errno == EINVAL)
					I_Error("wad %s, lump %d: invalid compressed data", wad->filename, lump);
			}
			// Otherwise, fall back on below error (if zero was actually the correct size then ???)
#endif
			if (retval != l->size)
			{
				I_Error("wad %s, lump %d: decompressed to wrong number of bytes (expected %s, got %s)", wad->filename, lump, sizeu1(l->size), sizeu2(retval));
			}

			if (!decData) // Did we get no data at all?
				return 0;
			M_Memcpy(dest, decData + offset, size);
			Z_Free(rawData);
			Z_Free(decData);
#ifdef NO_PNG_LUMPS
			if (Picture_IsLumpPNG((UINT8 *)dest, size))
				Picture_ThrowPNGError(l->fullname, wad->filename);
#endif
			return size;
#else
			//I_Error("ZWAD files not supported on this platform.");
			return 0;
#endif

		}
#ifdef HAVE_ZLIB
	case CM_DEFLATE: // Is it compressed via DEFLATE? Very common in ZIPs/PK3s, also what most doom-related editors support.
		{
			UINT8 *rawData; // The lump's raw data.
			UINT8 *decData; // Lump's decompressed real data.

			int zErr; // Helper var.
			z_stream strm;
			unsigned long rawSize = l->disksize;
			unsigned long decSize = l->size;

			rawData = Z_Malloc(rawSize, PU_STATIC, NULL);
			decData = Z_Malloc(decSize, PU_STATIC, NULL);

			if (File_Read(rawData, 1, rawSize, handle) < rawSize)
				I_Error("wad %s, lump %d: cannot read compressed data", wad->filename, lump);

			strm.zalloc = Z_NULL;
			strm.zfree = Z_NULL;
			strm.opaque = Z_NULL;

			strm.total_in = strm.avail_in = rawSize;
			strm.total_out = strm.avail_out = decSize;

			strm.next_in = rawData;
			strm.next_out = decData;

			zErr = inflateInit2(&strm, -15);
			if (zErr == Z_OK)
			{
				zErr = inflate(&strm, Z_FINISH);
				if (zErr == Z_STREAM_END)
				{
					M_Memcpy(dest, decData, size);
				}
				else
				{
					size = 0;
					zerr(zErr);
				}

				(void)inflateEnd(&strm);
			}
			else
			{
				size = 0;
				zerr(zErr);
			}

			Z_Free(rawData);
			Z_Free(decData);

#ifdef NO_PNG_LUMPS
			if (Picture_IsLumpPNG((UINT8 *)dest, size))
				Picture_ThrowPNGError(l->fullname, wad->filename);
#endif
			return size;
		}
#endif
	default:
		I_Error("wad %s, lump %d: unsupported compression type!", wad->filename, lump);
	}
	return 0;
}
