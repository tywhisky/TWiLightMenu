/*-----------------------------------------------------------------
 Copyright (C) 2015
	Matthew Scholefield

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

------------------------------------------------------------------*/

#include "common/gl2d.h"
#include <list>
#include <stdio.h>
#include <nds/interrupts.h>
#include "FontGraphic.h"
#include "fontHandler.h"
#include "TextEntry.h"
#include <nds.h>

// GRIT auto-genrated arrays of images
#include "small_font.h"
#include "large_font.h"

// Texture UV coords
#include "uvcoord_small_font.h"
#include "TextPane.h"

using namespace std;

FontGraphic smallFont;
FontGraphic largeFont;

glImage smallFontImages[SMALL_FONT_NUM_IMAGES];
glImage largeFontImages[SMALL_FONT_NUM_IMAGES];

list<TextEntry> topText, bottomText;
list<TextPane> panes;


void fontInit()
{
	
	smallFont.load(0, smallFontImages, // pointer to glImage array
				SMALL_FONT_NUM_IMAGES, // Texture packer auto-generated #define
				small_font_texcoords, // Texture packer auto-generated array
				GL_RGB16, // texture type for glTexImage2D() in videoGL.h
				TEXTURE_SIZE_512, // sizeX for glTexImage2D() in videoGL.h
				TEXTURE_SIZE_128, // sizeY for glTexImage2D() in videoGL.h
				TEXGEN_OFF | GL_TEXTURE_COLOR0_TRANSPARENT, // param for glTexImage2D() in videoGL.h
				4, // Length of the palette (16 colors)
				(u16*) small_fontPal, // Palette Data
				(u8*) small_fontBitmap, // image data generated by GRIT
				small_utf16_lookup_table
				);

	//Do the same with our bigger texture
	largeFont.load(1, largeFontImages,
				SMALL_FONT_NUM_IMAGES,
				small_font_texcoords,
				GL_RGB16,
				TEXTURE_SIZE_512,
				TEXTURE_SIZE_256,
				TEXGEN_OFF | GL_TEXTURE_COLOR0_TRANSPARENT,
				4,
				(u16*) large_fontPal,
				(u8*) large_fontBitmap,
				small_utf16_lookup_table
				);
}

TextPane &createTextPane(int startX, int startY, int shownElements)
{
	if (panes.size() > 2)
		panes.pop_front();
	panes.emplace_back(startX, startY, shownElements);
	return panes.back();
}

static list<TextEntry> &getTextQueue(bool top)
{
	return top ? topText : bottomText;
}

FontGraphic &getFont(bool large)
{
	return large ? largeFont : smallFont;
}

void updateText(bool top)
{
	auto &text = getTextQueue(top);
	for (auto it = text.begin(); it != text.end(); ++it)
	{
		if (it->update())
		{
			it = text.erase(it);
			--it;
			continue;
		}
		int alpha = it->calcAlpha();
		if (alpha > 0)
		{
			glPolyFmt(POLY_ALPHA(alpha) | POLY_CULL_NONE | POLY_ID(1));
			// if (top)
			// 	glColor(RGB15(0, 0, 0));
			getFont(it->large).print(it->x / TextEntry::PRECISION, it->y / TextEntry::PRECISION, it->message);
		}
	}
	for (auto it = panes.begin(); it != panes.end(); ++it)
	{
		if (it->update(top))
		{
			it = panes.erase(it);
			--it;
			continue;
		}
	}
}

void clearText(bool top)
{
	list<TextEntry> &text = getTextQueue(top);
	for (auto it = text.begin(); it != text.end(); ++it)
	{
		if (it->immune)
			continue;
		it = text.erase(it);
		--it;
	}
}

void clearText()
{
	clearText(true);
	clearText(false);
}

void printSmall(bool top, int x, int y, const char *message)
{
	getTextQueue(top).emplace_back(false, x, y, message);
}

void printSmallCentered(bool top, int y, const char *message)
{
	getTextQueue(top).emplace_back(false, smallFont.getCenteredX(message), y, message);
}

void printLarge(bool top, int x, int y, const char *message)
{
	getTextQueue(top).emplace_back(true, x, y, message);
}

void printLargeCentered(bool top, int y, const char *message)
{
	getTextQueue(top).emplace_back(true, largeFont.getCenteredX(message), y, message);
}

int calcSmallFontWidth(const char *text)
{
	return smallFont.calcWidth(text);
}

int calcLargeFontWidth(const char *text)
{
	return largeFont.calcWidth(text);
}

TextEntry *getPreviousTextEntry(bool top)
{
	return &getTextQueue(top).back();
}

void waitForPanesToClear()
{
	while (panes.size() > 0)
		swiWaitForVBlank();
}