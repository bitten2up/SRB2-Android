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
/// \file  xtv_video.c
/// \brief Extra video data handling routines

#include "xtv_video.h"

#include "../g_game.h"
#include "../p_local.h" // stplyr

// So it turns out offsets aren't scaled in V_NOSCALESTART unless V_OFFSET is applied ...poo, that's terrible
// For now let's just at least give V_OFFSET the ability to support V_FLIP
// I'll probably make a better fix for 2.2 where I don't have to worry about breaking existing support for stuff
// -- Monster Iestyn 29/10/18
void XTRA_V_OffsetPatch(fixed_t *x, fixed_t *y, fixed_t pscale, fixed_t vscale, INT32 scrn, patch_t *patch)
{
	fixed_t offsetx = 0, offsety = 0;

	// left offset
	if (scrn & V_FLIP)
		offsetx = FixedMul((patch->width - patch->leftoffset)<<FRACBITS, pscale) + 1;
	else
		offsetx = FixedMul(patch->leftoffset<<FRACBITS, pscale);

	// top offset
	offsety = FixedMul(patch->topoffset<<FRACBITS, vscale);

	// Subtract the offsets from x/y positions
	(*x) -= offsetx;
	(*y) -= offsety;
}

void V_GetPatchScreenRegion(fixed_t *x, fixed_t *y, fixed_t *w, fixed_t *h, fixed_t pscale, fixed_t vscale, INT32 scrn, patch_t *patch)
{
	fixed_t colfrac, rowfrac, fdup, vdup;
	INT32 dupx, dupy;
	UINT8 perplayershuffle = 0;

	if (patch == NULL)
		return;

	dupx = vid.dup;
	dupy = vid.dup;
	if (scrn & V_SCALEPATCHMASK) switch ((scrn & V_SCALEPATCHMASK) >> V_SCALEPATCHSHIFT)
	{
		case 1: // V_NOSCALEPATCH
			dupx = dupy = 1;
			break;
		case 2: // V_SMALLSCALEPATCH
			dupx = vid.smalldup;
			dupy = vid.smalldup;
			break;
		case 3: // V_MEDSCALEPATCH
			dupx = vid.meddup;
			dupy = vid.meddup;
			break;
		default:
			break;
	}

	// only use one dup, to avoid stretching (har har)
	dupx = dupy = (dupx < dupy ? dupx : dupy);
	fdup = vdup = FixedMul(dupx<<FRACBITS, pscale);
	if (vscale != pscale)
		vdup = FixedMul(dupx<<FRACBITS, vscale);
	colfrac = FixedDiv(FRACUNIT, fdup);
	rowfrac = FixedDiv(FRACUNIT, vdup);

	XTRA_V_OffsetPatch(x, y, pscale, vscale, scrn, patch);

	if (splitscreen && (scrn & V_PERPLAYER))
	{
		fixed_t adjusty = ((scrn & V_NOSCALESTART) ? vid.height : BASEVIDHEIGHT)<<(FRACBITS-1);
		vdup >>= 1;
		rowfrac <<= 1;
		(*y) >>= 1;
#ifdef QUADS
		if (splitscreen > 1) // 3 or 4 players
		{
			fixed_t adjustx = ((scrn & V_NOSCALESTART) ? vid.height : BASEVIDHEIGHT)<<(FRACBITS-1);
			fdup >>= 1;
			colfrac <<= 1;
			(*x) >>= 1;
			if (stplyr == &players[displayplayer])
			{
				if (!(scrn & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 1;
				if (!(scrn & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 4;
				scrn &= ~V_SNAPTOBOTTOM|V_SNAPTORIGHT;
			}
			else if (stplyr == &players[secondarydisplayplayer])
			{
				if (!(scrn & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 1;
				if (!(scrn & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 8;
				(*x) += adjustx;
				scrn &= ~V_SNAPTOBOTTOM|V_SNAPTOLEFT;
			}
			else if (stplyr == &players[thirddisplayplayer])
			{
				if (!(scrn & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 2;
				if (!(scrn & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 4;
				(*y) += adjusty;
				scrn &= ~V_SNAPTOTOP|V_SNAPTORIGHT;
			}
			else //if (stplyr == &players[fourthdisplayplayer])
			{
				if (!(scrn & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle |= 2;
				if (!(scrn & (V_SNAPTOLEFT|V_SNAPTORIGHT)))
					perplayershuffle |= 8;
				(*x) += adjustx;
				(*y) += adjusty;
				scrn &= ~V_SNAPTOTOP|V_SNAPTOLEFT;
			}
		}
		else
#endif
		// 2 players
		{
			if (stplyr == &players[displayplayer])
			{
				if (!(scrn & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle = 1;
				scrn &= ~V_SNAPTOBOTTOM;
			}
			else //if (stplyr == &players[secondarydisplayplayer])
			{
				if (!(scrn & (V_SNAPTOTOP|V_SNAPTOBOTTOM)))
					perplayershuffle = 2;
				(*y) += adjusty;
				scrn &= ~V_SNAPTOTOP;
			}
		}
	}

	if (!(scrn & V_NOSCALESTART))
	{
		(*x) = FixedMul((*x), dupx<<FRACBITS);
		(*y) = FixedMul((*y), dupy<<FRACBITS);

		// Center it if necessary
		if (!(scrn & V_SCALEPATCHMASK))
		{
			if (vid.width != BASEVIDWIDTH * dupx)
			{
				// dupx adjustments pretend that screen width is BASEVIDWIDTH * dupx,
				// so center this imaginary screen
				INT32 ox = 0;
				if (scrn & V_SNAPTORIGHT)
					ox += (vid.width - (BASEVIDWIDTH * dupx));
				else if (!(scrn & V_SNAPTOLEFT))
					ox += (vid.width - (BASEVIDWIDTH * dupx)) / 2;
				if (perplayershuffle & 4)
					ox -= (vid.width - (BASEVIDWIDTH * dupx)) / 4;
				else if (perplayershuffle & 8)
					ox += (vid.width - (BASEVIDWIDTH * dupx)) / 4;
				(*x) += (ox << FRACBITS);
			}
			if (vid.height != BASEVIDHEIGHT * dupy)
			{
				// same thing here
				INT32 oy = 0;
				if (scrn & V_SNAPTOBOTTOM)
					oy += (vid.height - (BASEVIDHEIGHT * dupy));
				else if (!(scrn & V_SNAPTOTOP))
					oy += (vid.height - (BASEVIDHEIGHT * dupy)) / 2;
				if (perplayershuffle & 1)
					oy -= (vid.height - (BASEVIDHEIGHT * dupy)) / 4;
				else if (perplayershuffle & 2)
					oy += (vid.height - (BASEVIDHEIGHT * dupy)) / 4;
				(*y) += (oy << FRACBITS);
			}
		}
	}

	(*w) = FixedDiv(patch->width << FRACBITS, colfrac);
	(*h) = FixedDiv(patch->height << FRACBITS, rowfrac);
}

// Draws a scaled string.
void V_DrawScaledString(fixed_t x, fixed_t y, fixed_t scale, INT32 option, const char *string)
{
	fixed_t cx = x, cy = y;
	INT32 w, c, dupx, dupy, scrwidth, center = 0, left = 0;
	const char *ch = string;
	INT32 charflags = 0;
	const UINT8 *colormap = NULL;
	INT32 spacewidth = 4, charwidth = 0;

	INT32 lowercase = (option & V_ALLOWLOWERCASE);
	option &= ~V_FLIP; // which is also shared with V_ALLOWLOWERCASE...

	if (option & V_NOSCALESTART)
	{
		dupx = vid.dup;
		dupy = vid.dup;
		scrwidth = vid.width;
	}
	else
	{
		dupx = dupy = 1;
		scrwidth = vid.width/vid.dup;
		left = (scrwidth - BASEVIDWIDTH)/2;
		scrwidth -= left;
	}

	if (option & V_NOSCALEPATCH)
		scrwidth *= vid.dup;

	charflags = (option & V_CHARCOLORMASK);

	switch (option & V_SPACINGMASK)
	{
		case V_MONOSPACE:
			spacewidth = 8;
			/* FALLTHRU */
		case V_OLDSPACING:
			charwidth = 8;
			break;
		case V_6WIDTHSPACE:
			spacewidth = 6;
		default:
			break;
	}

	for (;;ch++)
	{
		if (!*ch)
			break;
		if (*ch & 0x80) //color ignoring
		{
			// manually set flags override color codes
			if (!(option & V_CHARCOLORMASK))
				charflags = ((*ch & 0x7f) << V_CHARCOLORSHIFT) & V_CHARCOLORMASK;
			continue;
		}
		if (*ch == '\n')
		{
			cx = x;

			if (option & V_RETURN8)
				cy += (8*dupy)*scale;
			else
				cy += (12*dupy)*scale;

			continue;
		}

		c = *ch;
		if (!lowercase)
			c = toupper(c);
		c -= FONTSTART;

		// character does not exist or is a space
		if (c < 0 || c >= FONTSIZE || !hu_font.chars[c])
		{
			cx += (spacewidth * dupx)*scale;
			continue;
		}

		if (charwidth)
		{
			w = charwidth * dupx;
			center = w/2 - hu_font.chars[c]->width*(dupx/2);
		}
		else
			w = hu_font.chars[c]->width * dupx;

		if ((cx>>FRACBITS) > scrwidth)
			continue;
		if ((cx>>FRACBITS)+left + w < 0) //left boundary check
		{
			cx += w*scale;
			continue;
		}

		colormap = V_GetStringColormap(charflags);
		V_DrawFixedPatch(cx + (center*scale), cy, scale, option, hu_font.chars[c], colormap);

		cx += w*scale;
	}
}

// Draws a scaled thin string.
void V_DrawScaledThinString(fixed_t x, fixed_t y, fixed_t scale, INT32 option, const char *string)
{
	fixed_t cx = x, cy = y;
	INT32 w, c, dupx, dupy, scrwidth, center = 0, left = 0;
	const char *ch = string;
	INT32 charflags = 0;
	const UINT8 *colormap = NULL;
	INT32 spacewidth = 2, charwidth = 0;

	INT32 lowercase = (option & V_ALLOWLOWERCASE);
	option &= ~V_FLIP; // which is also shared with V_ALLOWLOWERCASE...

	if (option & V_NOSCALESTART)
	{
		dupx = vid.dup;
		dupy = vid.dup;
		scrwidth = vid.width;
	}
	else
	{
		dupx = dupy = 1;
		scrwidth = vid.width/vid.dup;
		left = (scrwidth - BASEVIDWIDTH)/2;
		scrwidth -= left;
	}

	if (option & V_NOSCALEPATCH)
		scrwidth *= vid.dup;

	charflags = (option & V_CHARCOLORMASK);

	switch (option & V_SPACINGMASK)
	{
		case V_MONOSPACE:
			spacewidth = 8;
			/* FALLTHRU */
		case V_OLDSPACING:
			charwidth = 8;
			break;
		case V_6WIDTHSPACE:
			spacewidth = 6;
		default:
			break;
	}

	for (;;ch++)
	{
		if (!*ch)
			break;
		if (*ch & 0x80) //color parsing -x 2.16.09
		{
			// manually set flags override color codes
			if (!(option & V_CHARCOLORMASK))
				charflags = ((*ch & 0x7f) << V_CHARCOLORSHIFT) & V_CHARCOLORMASK;
			continue;
		}
		if (*ch == '\n')
		{
			cx = x;

			if (option & V_RETURN8)
				cy += (8*dupy)*scale;
			else
				cy += (12*dupy)*scale;

			continue;
		}

		c = *ch;
		if (!lowercase || !tny_font.chars[c-FONTSTART])
			c = toupper(c);
		c -= FONTSTART;

		// character does not exist or is a space
		if (c < 0 || c >= FONTSIZE || !tny_font.chars[c])
		{
			cx += (spacewidth * dupx)*scale;
			continue;
		}

		if (charwidth)
		{
			w = charwidth * dupx;
			center = w/2 - tny_font.chars[c]->width*(dupx/2);
		}
		else
			w = tny_font.chars[c]->width * dupx;

		if ((cx>>FRACBITS) > scrwidth)
			break;
		if ((cx>>FRACBITS)+left + w < 0) //left boundary check
		{
			cx += w*scale;
			continue;
		}

		colormap = V_GetStringColormap(charflags);

		V_DrawFixedPatch(cx + (center*scale), cy, scale, option, tny_font.chars[c], colormap);

		cx += w*scale;
	}
}
