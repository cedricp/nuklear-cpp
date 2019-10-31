#include <vt100_widget.h>

void
NkVt100FramebufferDisplay::draw_character(char c, unsigned char char_set, uint32_t bg_col, uint32_t fg_col, int x, int y){
	uint32_t pushedData[4 * 6];
	uint32_t *pushedDataHead = pushedData; // pointer to latest pixel in pushedData

	const int char_base = ((char_set & 0x01) * 128 + // ensure 0-1 value
			(c & 0x7f) // ensure max 7-bit character value
			) * m_char_height;
	for (int char_font_row = 0; char_font_row < m_char_height; char_font_row++) {
		const unsigned char font_hline = font454[char_base + char_font_row];

		for (int char_font_bitoffset = m_char_height; char_font_bitoffset >= 0;
				char_font_bitoffset -= 2) {
			const unsigned char font_hline_mask = 3 << char_font_bitoffset;
			const unsigned char fg_value =
					(font_hline & font_hline_mask) >> char_font_bitoffset;

			*pushedDataHead = fg_value * (fg_col) + (1-fg_value) * bg_col;
			pushedDataHead++;
		}
	}

	// perform data push up to bottom edge of buffer
	const int preWrapHeight = min(m_screen_height - y, m_char_height);
	const int postWrapHeight = m_char_height - preWrapHeight;

	draw_pixels((int16_t) x, (int16_t) y, (int16_t) m_char_width,
			(int16_t) preWrapHeight, (uint32_t*) pushedData);

	// perform data push wrapped around from top edge of buffer
	if (postWrapHeight > 0) {
		draw_pixels((int16_t) x, (int16_t) 0, (int16_t) m_char_width,
				(int16_t) postWrapHeight,
				(uint32_t*) pushedData + m_char_width * preWrapHeight);
	}
}

void
NkVt100FramebufferDisplay::fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color){
	int width = m_framebuffer.get_width();
	int height = m_framebuffer.get_height();
	if (x + w > width) w = width - x;
	if (y + h > height) h = height - y;
	uint32_t* fbpos = ((uint32_t*)m_framebuffer.get_data());
	fbpos += x + (width * y);
	int xpos = 0;
	for (int i = 0; i < w * h; ++i){
		*fbpos = color;
		if (++xpos < w){
			fbpos++;
		} else {
			fbpos += width - w + 1;
			xpos = 0;
		}
	}
	m_framebuffer.update_texture();
}
void
NkVt100FramebufferDisplay::draw_pixels(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t *pixels){
	int width = m_framebuffer.get_width();
	uint32_t* fbpos = ((uint32_t*)m_framebuffer.get_data());
	fbpos += x + (width * y);
	int xpos = 0;
	for (int i = 0; i < w * h; ++i){
		*fbpos = *pixels++;
		if (++xpos < w){
			fbpos++;
		} else {
			fbpos += width - w + 1;
			xpos = 0;
		}
	}
	m_framebuffer.update_texture();
}
