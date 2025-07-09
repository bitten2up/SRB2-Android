// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by SRB2 Mobile Project.
// Copyright (C) 2025 by Bitten2Up.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  apk_m_textreader.h
/// \brief Text reader

#include "../doomtype.h"

#ifndef __APK_M_TEXTREADER__
#define __APK_M_TEXTREADER__

typedef struct textreader_s
{
	char *source;
	char *current;
	size_t size;

	int line;
} textreader_t;

textreader_t *TextReader_New(char *text, size_t size);
void TextReader_Delete(textreader_t *r);

size_t TextReader_GetLine(textreader_t *r, char *buf, size_t n);
size_t TextReader_GetLineLength(textreader_t *r);

#endif // __APK_M_TEXTREADER__
