#pragma once

static void draw_rect(int x0, int y0, int x1, int y1, u32 color)
{
	u32* dst = (u32*)back_buffer;

	dst += (y0*buffer_width);
	dst += x0;

	int dx = x1 - x0;
	int dy = y1 - y0;

	memset(dst, color, 4 * dx);

	for (int i = 0; i < dy; ++i)
	{
		*dst = color;
		dst += dx;
		*dst = color;
		dst += (buffer_width - dx);
	}

	memset(dst, color, 4 * (dx+1));

}

static void draw_horizontal_bar_with_bounds(int y, int x0, int x1, u32 color)
{
	u32* dst = (u32*)back_buffer;
	dst += (y*buffer_width);
	dst += x0;

	memset(dst, color, 4 * (x1 - x0));
}

static void draw_horizontal_bar(int y, u32 color)
{
	draw_horizontal_bar_with_bounds(y, 0, window_width, color);
}

static void draw_vertical_bar_with_bounds(int x, int y0, int y1, u32 color)
{
	u32* dst = (u32*)back_buffer;

	dst += (y0*buffer_width);
	dst += x;

	for (int i = 0; i < y1 - y0; ++i)
	{
		*dst = color;
		dst += (buffer_width);
	}
}


static void draw_vertical_bar(int x, u32 color)
{
	draw_vertical_bar_with_bounds(x, 0, window_height, color);
}

