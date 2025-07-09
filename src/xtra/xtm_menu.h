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
/// \file  xtm_menu.h
/// \brief Extra menu handling routines

#ifndef __XTM_MENU__
#define __XTM_MENU__

#include "../doomdef.h"
#include "../command.h"

void *XTRA_M_CVarSliding(const consvar_t *var);
INT32 XTRA_M_CVarValue(const consvar_t *var);
const char *XTRA_M_LongestColorName(void);

boolean XTRA_M_OpenSaveFileSlot(FILE **handle, char *name, char *savegamepaths, SINT8 slot);

#endif // __XTM_MENU__
