#pragma once

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "common.h"

typedef struct
{
	int xoffset;
	int yoffset;
	int width;
	int height;
	unsigned char* data;
} CFontCodePoint;

typedef struct
{
	stbtt_fontinfo fontinfo;
	int baseline;
	float yscale;

	CFontCodePoint cp[95]; // ASCII code points

} CFont;

static void init_font(CFont* fnt, const char* ttf_file_path, float font_size_in_pixels)
{
	const unsigned char ttf_buffer[MAXBUFLEN + 1] = { 0 };
	FILE* ttf_file = fopen(ttf_file_path, "rb");
	fread((void*)ttf_buffer, 1, MAXBUFLEN + 1, ttf_file);
	fclose(ttf_file);

	stbtt_fontinfo fi;
	stbtt_InitFont(&fi, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer, 0));
	fnt->fontinfo = fi;

	float scale = stbtt_ScaleForPixelHeight(&fi, font_size_in_pixels);
	fnt->yscale = scale;

	s32 ascent;
	stbtt_GetFontVMetrics(&fi, &ascent, 0, 0);
	s32 baseline = (s32)(ascent*scale);

	fnt->baseline = baseline;

	// initialize ASCII codepoints
	CFontCodePoint _cp[95];
	for (s32 i = 0; i < 95; ++i)
	{
		fnt->cp[i].data = stbtt_GetCodepointBitmap(&fi, 0, scale, i + 32, &_cp[i].width, &_cp[i].height, &_cp[i].xoffset, &_cp[i].yoffset);
		fnt->cp[i].width = _cp[i].width;
		fnt->cp[i].height = _cp[i].height;
		fnt->cp[i].xoffset = _cp[i].xoffset;
		fnt->cp[i].yoffset = _cp[i].yoffset;
	}
}

static void draw_char(CFontCodePoint *fcp, int x, int y, u32 color)
{
	u32* dst = (u32*)back_buffer;
	dst += (y*buffer_width);
	dst += x;

	u8* src = fcp->data;

	for (int j = 0; j < fcp->height; ++j)
	{
		for (int i = 0; i < fcp->width; ++i)
		{
			u8 s_r = color >> 16;
			u8 s_g = color >> 8;
			u8 s_b = color >> 0;

			u8 d_r = (*dst >> 16) & 0xFF;
			u8 d_g = (*dst >> 8) & 0xFF;
			u8 d_b = *dst & 0xFF;

			float alpha = *src / 255.0f;

			u8 ar, ag, ab;

			ar = (u8)(alpha * (double)(s_r - d_r) + d_r);
			ag = (u8)(alpha * (double)(s_g - d_g) + d_g);
			ab = (u8)(alpha * (double)(s_b - d_b) + d_b);

			*dst++ = (ar << 16 | ag << 8 | ab);
			++src;
		}
		dst += (buffer_width - fcp->width);
	}
}

static void draw_string(const char* s, CFont *f, int x, int y, u32 color)
{
	if (x < 0 || y < 0)
		return;

	int xpos = x;
	int advance, lsb;

	for (int i = 0; i < (int)strlen(s); ++i)
	{
		if (s[i] == '\0')
			break;

		draw_char(&f->cp[s[i] - 32], xpos + f->cp[s[i] - 32].xoffset, y + f->baseline + f->cp[s[i] - 32].yoffset, color);
		stbtt_GetCodepointHMetrics(&f->fontinfo, s[i] - 32, &advance, &lsb);

		// Advance horizontal placement
		if (s[i] == ' ')
		{
			xpos += (int)(advance*f->yscale);
		}
		else
		{
			xpos += (int)(advance*f->yscale);

			if (i < (int)strlen(s) - 1)
				xpos += (int)f->yscale*stbtt_GetCodepointKernAdvance(&f->fontinfo, s[i] - 32, s[i + 1] - 32);
		}

	}
}
