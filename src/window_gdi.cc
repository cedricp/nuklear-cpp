#include <window_gdi.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#include <stdlib.h>
#include <malloc.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_BUTTON_TRIGGER_ON_RELEASE

#include <nuklear.h>
#include <nuklear_lib.h>
#include <thread.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 480

#define FONT_DEBUG 1

struct gdi_font {
	HFONT handle;
	HDC dc;
	LONG height;
	struct nk_font font;
	std::string font_filename;
};

struct ImageData {
	ImageData(HBITMAP h) {
		alive = true;
		bm = h;
	}
	HBITMAP bm;
	bool alive;
};

struct gdi_impl {
	std::map<std::string, gdi_font*> font;
	HWND wnd;
	WNDCLASSW wc;
	HBITMAP bitmap;
	HDC window_dc;
	HDC memory_dc;
	unsigned int width;
	unsigned int height;
	std::vector<ImageData> image_data;
	Thread_mutex event_mutex;
	std::vector<UserEvent> event_queue;
};

extern NkWindow* global_ptr;

int nk_gdi_handle_event(gdi_impl* gdi, nk_context* ctx, HWND wnd, UINT msg,
		WPARAM wparam, LPARAM lparam) {
	switch (msg) {
	case WM_SIZE: {
		unsigned width = LOWORD(lparam);
		unsigned height = HIWORD(lparam);
		if (width != gdi->width || height != gdi->height) {
			DeleteObject(gdi->bitmap);
			gdi->bitmap = CreateCompatibleBitmap(gdi->window_dc, width, height);
			gdi->width = width;
			gdi->height = height;
			SelectObject(gdi->memory_dc, gdi->bitmap);
		}
		break;
	}

	case WM_PAINT: {
		PAINTSTRUCT paint;
		HDC dc = BeginPaint(wnd, &paint);
		BitBlt(dc, 0, 0, gdi->width, gdi->height, gdi->memory_dc, 0, 0,
				SRCCOPY);
		EndPaint(wnd, &paint);
		return 1;
	}

	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP: {
		int down = !((lparam >> 31) & 1);
		int ctrl = GetKeyState(VK_CONTROL) & (1 << 15);

		switch (wparam) {
		case VK_SHIFT:
		case VK_LSHIFT:
		case VK_RSHIFT:
			nk_input_key(ctx, NK_KEY_SHIFT, down);
			return 1;

		case VK_DELETE:
			nk_input_key(ctx, NK_KEY_DEL, down);
			return 1;

		case VK_RETURN:
			nk_input_key(ctx, NK_KEY_ENTER, down);
			return 1;

		case VK_TAB:
			nk_input_key(ctx, NK_KEY_TAB, down);
			return 1;

		case VK_LEFT:
			if (ctrl)
				nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, down);
			else
				nk_input_key(ctx, NK_KEY_LEFT, down);
			return 1;

		case VK_RIGHT:
			if (ctrl)
				nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, down);
			else
				nk_input_key(ctx, NK_KEY_RIGHT, down);
			return 1;

		case VK_BACK:
			nk_input_key(ctx, NK_KEY_BACKSPACE, down);
			return 1;

		case VK_HOME:
			nk_input_key(ctx, NK_KEY_TEXT_START, down);
			nk_input_key(ctx, NK_KEY_SCROLL_START, down);
			return 1;

		case VK_END:
			nk_input_key(ctx, NK_KEY_TEXT_END, down);
			nk_input_key(ctx, NK_KEY_SCROLL_END, down);
			return 1;

		case VK_NEXT:
			nk_input_key(ctx, NK_KEY_SCROLL_DOWN, down);
			return 1;

		case VK_PRIOR:
			nk_input_key(ctx, NK_KEY_SCROLL_UP, down);
			return 1;

		case 'C':
			if (ctrl) {
				nk_input_key(ctx, NK_KEY_COPY, down);
				return 1;
			}
			break;

		case 'V':
			if (ctrl) {
				nk_input_key(ctx, NK_KEY_PASTE, down);
				return 1;
			}
			break;

		case 'X':
			if (ctrl) {
				nk_input_key(ctx, NK_KEY_CUT, down);
				return 1;
			}
			break;

		case 'Z':
			if (ctrl) {
				nk_input_key(ctx, NK_KEY_TEXT_UNDO, down);
				return 1;
			}
			break;

		case 'R':
			if (ctrl) {
				nk_input_key(ctx, NK_KEY_TEXT_REDO, down);
				return 1;
			}
			break;
		}
		return 0;
	}

	case WM_CHAR:
		if (wparam >= 32) {
			nk_input_unicode(ctx, (nk_rune) wparam);
			return 1;
		}
		break;

	case WM_LBUTTONDOWN:
		nk_input_button(ctx, NK_BUTTON_LEFT, (short) LOWORD(lparam),
				(short) HIWORD(lparam), 1);
		SetCapture(wnd);
		return 1;

	case WM_LBUTTONUP:
		nk_input_button(ctx, NK_BUTTON_DOUBLE, (short) LOWORD(lparam),
				(short) HIWORD(lparam), 0);
		nk_input_button(ctx, NK_BUTTON_LEFT, (short) LOWORD(lparam),
				(short) HIWORD(lparam), 0);
		ReleaseCapture();
		return 1;

	case WM_RBUTTONDOWN:
		nk_input_button(ctx, NK_BUTTON_RIGHT, (short) LOWORD(lparam),
				(short) HIWORD(lparam), 1);
		SetCapture(wnd);
		return 1;

	case WM_RBUTTONUP:
		nk_input_button(ctx, NK_BUTTON_RIGHT, (short) LOWORD(lparam),
				(short) HIWORD(lparam), 0);
		ReleaseCapture();
		return 1;

	case WM_MBUTTONDOWN:
		nk_input_button(ctx, NK_BUTTON_MIDDLE, (short) LOWORD(lparam),
				(short) HIWORD(lparam), 1);
		SetCapture(wnd);
		return 1;

	case WM_MBUTTONUP:
		nk_input_button(ctx, NK_BUTTON_MIDDLE, (short) LOWORD(lparam),
				(short) HIWORD(lparam), 0);
		ReleaseCapture();
		return 1;

	case WM_MOUSEWHEEL:
		nk_input_scroll(ctx,
				nk_vec2(0, (float) (short) HIWORD(wparam) / WHEEL_DELTA));
		return 1;

	case WM_MOUSEMOVE:
		nk_input_motion(ctx, (short) LOWORD(lparam), (short) HIWORD(lparam));
		return 1;

	case WM_LBUTTONDBLCLK:
		nk_input_button(ctx, NK_BUTTON_DOUBLE, (short) LOWORD(lparam),
				(short) HIWORD(lparam), 1);
		return 1;
	}

	return 0;
}

static LRESULT CALLBACK
WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}


	if (global_ptr){
		NkWindowGDI* gdiwin = (NkWindowGDI*) global_ptr;
		if (nk_gdi_handle_event(gdiwin->get_gdi_impl(), gdiwin->get_ctx(), wnd, msg,
				wparam, lparam)){
			return 0;
		}
	}

	return DefWindowProcW(wnd, msg, wparam, lparam);
}

static inline float nk_gdifont_get_text_width(nk_handle handle, float height,
		const char *text, int len) {
	gdi_font *font = (gdi_font*) handle.ptr;
	SIZE size;
	int wsize;
	WCHAR* wstr;
	if (!font || !text)
		return 0;

	wsize = MultiByteToWideChar(CP_UTF8, 0, text, len, NULL, 0);
	wstr = (WCHAR*) alloca(wsize * sizeof(wchar_t));
	MultiByteToWideChar(CP_UTF8, 0, text, len, wstr, wsize);
	if (GetTextExtentPoint32W(font->dc, wstr, wsize, &size))
		return (float) size.cx;
	return -1.0f;
}

static inline void nk_gdi_clipboard_paste(nk_handle usr,
		struct nk_text_edit *edit) {
	(void) usr;
	if (IsClipboardFormatAvailable(CF_UNICODETEXT) && OpenClipboard(NULL)) {
		HGLOBAL mem = GetClipboardData(CF_UNICODETEXT);
		if (mem) {
			SIZE_T size = GlobalSize(mem) - 1;
			if (size) {
				LPCWSTR wstr = (LPCWSTR) GlobalLock(mem);
				if (wstr) {
					int utf8size = WideCharToMultiByte(CP_UTF8, 0, wstr,
							(int) (size / sizeof(wchar_t)), NULL, 0, NULL,
							NULL);
					if (utf8size) {
						char* utf8 = (char*) malloc(utf8size);
						if (utf8) {
							WideCharToMultiByte(CP_UTF8, 0, wstr,
									(int) (size / sizeof(wchar_t)), utf8,
									utf8size, NULL, NULL);
							nk_textedit_paste(edit, utf8, utf8size);
							free(utf8);
						}
					}
					GlobalUnlock(mem);
				}
			}
		}
		CloseClipboard();
	}
}

static inline void nk_gdi_clipboard_copy(nk_handle usr, const char *text,
		int len) {
	if (OpenClipboard(NULL)) {
		int wsize = MultiByteToWideChar(CP_UTF8, 0, text, len, NULL, 0);
		if (wsize) {
			HGLOBAL mem = (HGLOBAL) GlobalAlloc(GMEM_MOVEABLE,
					(wsize + 1) * sizeof(wchar_t));
			if (mem) {
				wchar_t* wstr = (wchar_t*) GlobalLock(mem);
				if (wstr) {
					MultiByteToWideChar(CP_UTF8, 0, text, len, wstr, wsize);
					wstr[wsize] = 0;
					GlobalUnlock(mem);

					SetClipboardData(CF_UNICODETEXT, mem);
				}
			}
		}
		CloseClipboard();
	}
}

static inline void nk_gdi_draw_image(HBITMAP hbm, HDC memory_dc, short x, short y,
		unsigned short w, unsigned short h, struct nk_image img,
		struct nk_color col) {
	HDC hDCBits;
	BITMAP bitmap;

	if (!memory_dc || !hbm)
		return;

	hDCBits = CreateCompatibleDC(memory_dc);
	GetObject(hbm, sizeof(BITMAP), (LPSTR) &bitmap);
	SelectObject(hDCBits, hbm);
	StretchBlt(memory_dc, x, y, w, h, hDCBits, 0, 0, bitmap.bmWidth,
			bitmap.bmHeight, SRCCOPY);
	DeleteDC(hDCBits);
}

static inline void nk_gdi_scissor(HDC dc, float x, float y, float w, float h) {
	SelectClipRgn(dc, NULL);
	IntersectClipRect(dc, (int) x, (int) y, (int) (x + w + 1),
			(int) (y + h + 1));
}

static COLORREF convert_color(struct nk_color c) {
	return c.r | (c.g << 8) | (c.b << 16);
}

static inline void nk_gdi_stroke_line(HDC dc, short x0, short y0, short x1,
		short y1, unsigned int line_thickness, struct nk_color col) {
	COLORREF color = convert_color(col);

	HPEN pen = NULL;
	pen = CreatePen(PS_SOLID, line_thickness, color);
	SelectObject(dc, pen);

	MoveToEx(dc, x0, y0, NULL);
	LineTo(dc, x1, y1);

	if (pen) {
		SelectObject(dc, pen);
		DeleteObject(pen);
	}
}

static inline void nk_gdi_stroke_rect(HDC dc, short x, short y,
		unsigned short w, unsigned short h, unsigned short r,
		unsigned short line_thickness, struct nk_color col) {
	COLORREF color = convert_color(col);

	HPEN pen = NULL;
	pen = CreatePen(PS_SOLID, line_thickness, color);
	SelectObject(dc, pen);

	HGDIOBJ br = SelectObject(dc, GetStockObject(NULL_BRUSH));
	if (r == 0) {
		Rectangle(dc, x, y, x + w, y + h);
	} else {
		RoundRect(dc, x, y, x + w, y + h, r, r);
	}
	SelectObject(dc, br);

	if (pen) {
		SelectObject(dc, pen);
		DeleteObject(pen);
	}
}

static inline void nk_gdi_fill_rect(HDC dc, short x, short y, unsigned short w,
		unsigned short h, unsigned short r, struct nk_color col) {
	COLORREF color = convert_color(col);

	if (r == 0) {
		RECT rect = { x, y, x + w, y + h };
		SetBkColor(dc, color);
		ExtTextOutW(dc, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);
	} else {
		HBRUSH brush = CreateSolidBrush(color);
		SelectObject(dc, brush);
		SetBkColor(dc, color);
		RoundRect(dc, x, y, x + w, y + h, r, r);
		SelectObject(dc, brush);
		DeleteObject(brush);
	}
}

static inline void nk_gdi_fill_triangle(HDC dc, short x0, short y0, short x1,
		short y1, short x2, short y2, struct nk_color col) {
	COLORREF color = convert_color(col);
	POINT points[] = { { x0, y0 }, { x1, y1 }, { x2, y2 }, };

	HBRUSH brush = CreateSolidBrush(color);
	SelectObject(dc, brush);
	SetBkColor(dc, color);
	Polygon(dc, points, 3);
	SelectObject(dc, brush);
	DeleteObject(brush);
}

static inline void nk_gdi_stroke_triangle(HDC dc, short x0, short y0, short x1,
		short y1, short x2, short y2, unsigned short line_thickness,
		struct nk_color col) {
	COLORREF color = convert_color(col);
	POINT points[] = { { x0, y0 }, { x1, y1 }, { x2, y2 }, { x0, y0 }, };

	HPEN pen = NULL;
	pen = CreatePen(PS_SOLID, line_thickness, color);
	SelectObject(dc, pen);

	Polyline(dc, points, 4);

	if (pen) {
		DeleteObject(pen);
	}
}

static inline void nk_gdi_fill_polygon(HDC dc, const struct nk_vec2i *pnts,
		int count, struct nk_color col) {
	int i = 0;
#define MAX_POINTS 64
	POINT points[MAX_POINTS];
	COLORREF color = convert_color(col);
//    SetDCBrushColor(dc, color);
//    SetDCPenColor(dc, color);
//	SetBkColor(dc, color);
	HBRUSH brush = CreateSolidBrush(color);
	SelectObject(dc, brush);
	for (i = 0; i < count && i < MAX_POINTS; ++i) {
		points[i].x = pnts[i].x;
		points[i].y = pnts[i].y;
	}
	Polygon(dc, points, i);
	SelectObject(dc, brush);
	DeleteObject(brush);
#undef MAX_POINTS
}

static inline void nk_gdi_stroke_polygon(HDC dc, const struct nk_vec2i *pnts,
		int count, unsigned short line_thickness, struct nk_color col) {
	COLORREF color = convert_color(col);
	HPEN pen = CreatePen(PS_SOLID, line_thickness, color);
	SelectObject(dc, pen);

	if (count > 0) {
		int i;
		MoveToEx(dc, pnts[0].x, pnts[0].y, NULL);
		for (i = 1; i < count; ++i)
			LineTo(dc, pnts[i].x, pnts[i].y);
		LineTo(dc, pnts[0].x, pnts[0].y);
	}

	if (pen) {
		SelectObject(dc, pen);
		DeleteObject(pen);
	}
}

static inline void nk_gdi_stroke_polyline(HDC dc, const struct nk_vec2i *pnts,
		int count, unsigned short line_thickness, struct nk_color col) {
	COLORREF color = convert_color(col);
	HPEN pen = NULL;
	pen = CreatePen(PS_SOLID, line_thickness, color);
	SelectObject(dc, pen);

	if (count > 0) {
		int i;
		MoveToEx(dc, pnts[0].x, pnts[0].y, NULL);
		for (i = 1; i < count; ++i)
			LineTo(dc, pnts[i].x, pnts[i].y);
	}

	if (pen) {
		SelectObject(dc, pen);
		DeleteObject(pen);
	}
}

static inline void nk_gdi_fill_circle(HDC dc, short x, short y,
		unsigned short w, unsigned short h, struct nk_color col) {
	COLORREF color = convert_color(col);
	HPEN pen = CreatePen(PS_SOLID, 1, color);
	SelectObject(dc, pen);

	HBRUSH brush = CreateSolidBrush(color);
	SelectObject(dc, brush);

	Ellipse(dc, x, y, x + w, y + h);

	if (pen) {
		SelectObject(dc, pen);
		DeleteObject(pen);
	}

	SelectObject(dc, brush);
	DeleteObject(brush);
}

static inline void nk_gdi_stroke_circle(HDC dc, short x, short y,
		unsigned short w, unsigned short h, unsigned short line_thickness,
		struct nk_color col) {
	COLORREF color = convert_color(col);
	HPEN pen = NULL;
	pen = CreatePen(PS_SOLID, line_thickness, color);
	SelectObject(dc, pen);

//    SetDCBrushColor(dc, OPAQUE);
	HBRUSH brush = CreateSolidBrush(color);
	SelectObject(dc, brush);
	Ellipse(dc, x, y, x + w, y + h);

	if (pen) {
        SelectObject(dc, pen);
		DeleteObject(pen);
	}

	SelectObject(dc, brush);
	DeleteObject(brush);
}

//static inline void
//nk_gdi_stroke_curve(HDC dc, struct nk_vec2i p1,
//    struct nk_vec2i p2, struct nk_vec2i p3, struct nk_vec2i p4,
//    unsigned short line_thickness, struct nk_color col)
//{
//    COLORREF color = convert_color(col);
//    POINT p[] = {
//        { p1.x, p1.y },
//        { p2.x, p2.y },
//        { p3.x, p3.y },
//        { p4.x, p4.y },
//    };
//
//    HPEN pen = NULL;
//	pen = CreatePen(PS_SOLID, line_thickness, color);
//	SelectObject(dc, pen);
//
////    SetDCBrushColor(dc, OPAQUE);
//    SetBkColor(dc, color);
//    PolyBezier(dc, p, 4);
//
//    if (pen) {
////        SelectObject(dc, GetStockObject(DC_PEN));
//        DeleteObject(pen);
//    }
//}

static inline void nk_gdi_draw_text(HDC dc, short x, short y, unsigned short w,
		unsigned short h, const char *text, int len, gdi_font *font,
		struct nk_color cbg, struct nk_color cfg) {
	int wsize;
	WCHAR* wstr;

	if (!text || !font || !len)
		return;

	wsize = MultiByteToWideChar(CP_UTF8, 0, text, len, NULL, 0);
	wstr = (WCHAR*) alloca(wsize * sizeof(wchar_t));
	MultiByteToWideChar(CP_UTF8, 0, text, len, wstr, wsize);

	SetBkColor(dc, convert_color(cbg));
	SetTextColor(dc, convert_color(cfg));

	SelectObject(dc, font->handle);
	ExtTextOutW(dc, x, y, ETO_OPAQUE, NULL, wstr, wsize, NULL);
}

static inline void nk_gdi_clear(gdi_impl* gdi, struct nk_color col) {
	COLORREF color = convert_color(col);
	RECT rect = { 0, 0, gdi->width, gdi->height };
	SetBkColor(gdi->memory_dc, color);

	ExtTextOutW(gdi->memory_dc, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);
}

NK_API inline void nk_gdi_set_font(struct nk_context* ctx, gdi_font *gdifont) {
	struct nk_user_font *font = &gdifont->font.handle;
	font->userdata = nk_handle_ptr(gdifont);
	font->height = (float) gdifont->height;
	font->width = nk_gdifont_get_text_width;
	nk_style_set_font(ctx, font);
}

NkWindowGDI::NkWindowGDI(int w, int h, bool aa) :
		NkWindow(w, h, aa) {
	m_gdi_impl = new gdi_impl;
	init_GDI(w, h);
	m_needs_refresh = false;
}

NkWindowGDI::~NkWindowGDI() {
}

void NkWindowGDI::run() {
	do_ui();
	render();
	nk_clear(get_ctx());

	while (is_running()) {
		bool evt = handle_events();
		if (!evt) {
			/*
			 * calm down the processor
			 */
			Sleep(100);
		} else {
			do_ui();
			nk_clear(get_ctx());
			nk_input_begin(get_ctx());
			nk_input_end(get_ctx());
			do_ui();
			render();
			nk_clear(get_ctx());
		}
	}
}

NkWindow*
NkWindowGDI::create(int w, int h, bool aa) {
	if (global_ptr != NULL)
		return global_ptr;

	global_ptr = (NkWindow*) new NkWindowGDI(w, h, aa);
	return global_ptr;
}

void NkWindowGDI::delete_texture(unsigned int id) {
	if (!m_gdi_impl->image_data[id].alive) {
		return;
	}

	HBITMAP hbm = m_gdi_impl->image_data[id].bm;
	DeleteObject(hbm);
	m_gdi_impl->image_data[id].alive = false;
}

unsigned int
NkWindowGDI::create_texture(const NkTexture& texture) {
	INT row = ((texture.width * 3 + 3) & ~3);
	BITMAPINFO bi = { 0 };
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = texture.width;
	bi.bmiHeader.biHeight = texture.height;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 24;
	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biSizeImage = row * texture.height;

	LPBYTE lpBuf, pb = NULL;
	HBITMAP hbm = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, (void**) &lpBuf,
			NULL, 0);

	pb = lpBuf + row * texture.height;
	unsigned char * src = (unsigned char *) texture.data;
	for (int v = 0; v < texture.height; v++) {
		pb -= row;
		for (int i = 0; i < row; i += 3) {
			pb[i + 0] = src[2];
			pb[i + 1] = src[1];
			pb[i + 2] = src[0];
			if (texture.internal_format == NkTexture::RGBA_BYTE){
				src += 4;
			} else {
				src += 3;
			}
		}
	}
	// No library link on WINCE
#ifndef WINCE
	//SetDIBits(NULL, hbm, 0, texture.height, lpBuf, &bi, DIB_RGB_COLORS);
#endif
	ImageData imgdata(hbm);

	for (int i = 0; i < m_gdi_impl->image_data.size(); ++i) {
		if (!m_gdi_impl->image_data[i].alive) {
			m_gdi_impl->image_data[i] = imgdata;
			return i;
		}
	}
	m_gdi_impl->image_data.push_back(imgdata);
	return m_gdi_impl->image_data.size() - 1;
}

bool
NkWindowGDI::handle_events() {
	handle_fd_events();
	handle_timer_events();

	/* Input */
	//bool needs_refresh = false;

	MSG msg;
	nk_input_begin(get_ctx());

	while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT)
			set_running(false);
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
		m_needs_refresh = true;
	}

	nk_input_end(get_ctx());

	while (!m_gdi_impl->event_queue.empty()){
		m_gdi_impl->event_mutex.lock();
		UserEvent& ev = m_gdi_impl->event_queue.front();
		m_needs_refresh |= true;
		m_gdi_impl->event_queue.erase(m_gdi_impl->event_queue.begin());
		m_gdi_impl->event_mutex.unlock();
		ev.on_callback();
	}

	return m_needs_refresh;
}

void
NkWindowGDI::render() {
	const struct nk_command *cmd;
	struct nk_color clear_color;
	HDC memory_dc = m_gdi_impl->memory_dc;
//    SelectObject(memory_dc, GetStockObject(DC_PEN));
//    SelectObject(memory_dc, GetStockObject(DC_BRUSH));
	nk_gdi_clear(m_gdi_impl, clear_color);

	nk_foreach(cmd, get_ctx())
	{
		switch (cmd->type) {
		case NK_COMMAND_NOP:
			break;
		case NK_COMMAND_SCISSOR: {
			const struct nk_command_scissor *s =
					(const struct nk_command_scissor*) cmd;
			nk_gdi_scissor(memory_dc, s->x, s->y, s->w, s->h);
		}
			break;
		case NK_COMMAND_LINE: {
			const struct nk_command_line *l =
					(const struct nk_command_line *) cmd;
			nk_gdi_stroke_line(memory_dc, l->begin.x, l->begin.y, l->end.x,
					l->end.y, l->line_thickness, l->color);
		}
			break;
		case NK_COMMAND_RECT: {
			const struct nk_command_rect *r =
					(const struct nk_command_rect *) cmd;
			nk_gdi_stroke_rect(memory_dc, r->x, r->y, r->w, r->h,
					(unsigned short) r->rounding, r->line_thickness, r->color);
		}
			break;
		case NK_COMMAND_RECT_FILLED: {
			const struct nk_command_rect_filled *r =
					(const struct nk_command_rect_filled *) cmd;
			nk_gdi_fill_rect(memory_dc, r->x, r->y, r->w, r->h,
					(unsigned short) r->rounding, r->color);
		}
			break;
		case NK_COMMAND_CIRCLE: {
			const struct nk_command_circle *c =
					(const struct nk_command_circle *) cmd;
			nk_gdi_stroke_circle(memory_dc, c->x, c->y, c->w, c->h,
					c->line_thickness, c->color);
		}
			break;
		case NK_COMMAND_CIRCLE_FILLED: {
			const struct nk_command_circle_filled *c =
					(const struct nk_command_circle_filled *) cmd;
			nk_gdi_fill_circle(memory_dc, c->x, c->y, c->w, c->h, c->color);
		}
			break;
		case NK_COMMAND_TRIANGLE: {
			const struct nk_command_triangle*t =
					(const struct nk_command_triangle*) cmd;
			nk_gdi_stroke_triangle(memory_dc, t->a.x, t->a.y, t->b.x, t->b.y,
					t->c.x, t->c.y, t->line_thickness, t->color);
		}
			break;
		case NK_COMMAND_TRIANGLE_FILLED: {
			const struct nk_command_triangle_filled *t =
					(const struct nk_command_triangle_filled *) cmd;
			nk_gdi_fill_triangle(memory_dc, t->a.x, t->a.y, t->b.x, t->b.y,
					t->c.x, t->c.y, t->color);
		}
			break;
		case NK_COMMAND_POLYGON: {
			const struct nk_command_polygon *p =
					(const struct nk_command_polygon*) cmd;
			nk_gdi_stroke_polygon(memory_dc, p->points, p->point_count,
					p->line_thickness, p->color);
		}
			break;
		case NK_COMMAND_POLYGON_FILLED: {
			const struct nk_command_polygon_filled *p =
					(const struct nk_command_polygon_filled *) cmd;
			nk_gdi_fill_polygon(memory_dc, p->points, p->point_count, p->color);
		}
			break;
		case NK_COMMAND_POLYLINE: {
			const struct nk_command_polyline *p =
					(const struct nk_command_polyline *) cmd;
			nk_gdi_stroke_polyline(memory_dc, p->points, p->point_count,
					p->line_thickness, p->color);
		}
			break;
		case NK_COMMAND_TEXT: {
			const struct nk_command_text *t =
					(const struct nk_command_text*) cmd;
			nk_gdi_draw_text(memory_dc, t->x, t->y, t->w, t->h,
					(const char*) t->string, t->length,
					(gdi_font*) t->font->userdata.ptr, t->background,
					t->foreground);
		}
			break;
		case NK_COMMAND_IMAGE: {
			const struct nk_command_image *i =
					(const struct nk_command_image *) cmd;
			ImageData& img_data = m_gdi_impl->image_data[i->img.handle.id];
			nk_gdi_draw_image(img_data.bm, memory_dc, i->x, i->y, i->w, i->h, i->img,
					i->col);
		}
			break;
		case NK_COMMAND_CURVE:
//		{
//            const struct nk_command_curve *q = (const struct nk_command_curve *)cmd;
//            nk_gdi_stroke_curve(memory_dc, q->begin, q->ctrl[0], q->ctrl[1],
//                q->end, q->line_thickness, q->color);
//		}
//			break;
		case NK_COMMAND_RECT_MULTI_COLOR:
//		{
//			const struct nk_command_rect_multi_color *r =
//					(const struct nk_command_rect_multi_color *) cmd;
//            nk_gdi_rect_multi_color(memory_dc, r->x, r->y,r->w, r->h, r->left, r->top, r->right, r->bottom);
//		}
//			break;
		case NK_COMMAND_ARC:
		case NK_COMMAND_ARC_FILLED:
		default:
			break;
		}
	}
	BitBlt(m_gdi_impl->window_dc, 0, 0, m_gdi_impl->width, m_gdi_impl->height,
			m_gdi_impl->memory_dc, 0, 0, SRCCOPY);
}

void
NkWindowGDI::do_ui() {
	/* GUI */
	for (int i = 0; i < get_widgets().size(); ++i) {
		get_widgets()[i]->draw(get_ctx());
	}
}

bool
NkWindowGDI::init_GDI(int width, int height) {
	ATOM atom;
	RECT rect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
	DWORD style = WS_DLGFRAME;//WS_OVERLAPPEDWINDOW;
	DWORD exstyle = WS_EX_APPWINDOW;
	int running = 1;
	int needs_refresh = 1;

	/* Win32 */
	memset(&m_gdi_impl->wc, 0, sizeof(m_gdi_impl->wc));
	m_gdi_impl->wc.style = CS_DBLCLKS;
	m_gdi_impl->wc.lpfnWndProc = WindowProc;
	m_gdi_impl->wc.hInstance = GetModuleHandleW(0);
	m_gdi_impl->wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	m_gdi_impl->wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	m_gdi_impl->wc.lpszClassName = L"NuklearWindowClass";
	atom = RegisterClassW(&m_gdi_impl->wc);

	AdjustWindowRectEx(&rect, style, FALSE, exstyle);
	m_gdi_impl->wnd = CreateWindowExW(exstyle, m_gdi_impl->wc.lpszClassName,
			L"Nuklear Demo", style | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
			rect.right - rect.left, rect.bottom - rect.top,
			NULL, NULL, m_gdi_impl->wc.hInstance, NULL);
	m_gdi_impl->window_dc = GetDC(m_gdi_impl->wnd);
	add_font("native", "Arial", 12);
	nk_gdi_set_font(get_ctx(), (gdi_font*)get_user_font("native")->handle.userdata.ptr);

	m_gdi_impl->bitmap = CreateCompatibleBitmap(m_gdi_impl->window_dc, width,
			height);
	m_gdi_impl->memory_dc = CreateCompatibleDC(m_gdi_impl->window_dc);
	m_gdi_impl->width = width;
	m_gdi_impl->height = height;
	SelectObject(m_gdi_impl->memory_dc, m_gdi_impl->bitmap);

	nk_init_default(get_ctx(), &get_user_font("native")->handle);
	get_ctx()->clip.copy = nk_gdi_clipboard_copy;
	get_ctx()->clip.paste = nk_gdi_clipboard_paste;
	return true;
}

void
NkWindowGDI::add_font(std::string font_name, std::string filename, float size)
{
	TEXTMETRICW metric;
	LOGFONTW lf;
	gdi_font *gdifont = new gdi_font;
	gdifont->dc = CreateCompatibleDC(0);
	lf.lfHeight = -size;
	lf.lfWidth = 0;
	lf.lfEscapement = 0;
	lf.lfOrientation = 0;
	lf.lfWeight = 0;
	lf.lfItalic = 0;
	lf.lfUnderline = 0;
	lf.lfStrikeOut = 0;
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfOutPrecision = 0;
	lf.lfClipPrecision = 0;
	lf.lfQuality = 0;
	lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
	wcscpy(lf.lfFaceName, (const wchar_t*) filename.c_str());
	//SetMapMode(gdifont->dc, MM_TEXT);
	gdifont->handle = CreateFontIndirectW(&lf);
	SelectObject(gdifont->dc, gdifont->handle);
	GetTextMetricsW(gdifont->dc, &metric);
	gdifont->height = metric.tmHeight;
	gdifont->font_filename = filename;
	gdifont->font.handle.width = nk_gdifont_get_text_width;
	gdifont->font.handle.height = metric.tmHeight;
	gdifont->font.handle.userdata = nk_handle_ptr(gdifont);
	m_gdi_impl->font[font_name] = gdifont;
}

struct nk_font*
NkWindowGDI::get_user_font(std::string font_name)
{
	if (font_name == "native" || m_gdi_impl->font.find(font_name) == m_gdi_impl->font.end()){
		return &m_gdi_impl->font["native"]->font;
	}
	return &m_gdi_impl->font[font_name]->font;
}

void
NkWindowGDI::load_fonts()
{
  // Useless
}

void
NkWindowGDI::use_font(std::string font_name)
{
	if (m_gdi_impl->font.find(font_name) == m_gdi_impl->font.end()){
		fprintf(stderr, "SDLWindow::use_font : Cannot use font [%s]\n", font_name.c_str());
		return;
	}
	struct nk_user_font* font_handler = &m_gdi_impl->font[font_name]->font.handle;
	if (!font_handler){
		fprintf(stderr, "WindowGDI::use_font : font [%s] not loaded\n", font_name.c_str());
		return;
	}
#ifdef FONT_DEBUG
	printf("Using fonts %x\n", font_handler);
#endif
	nk_style_set_font(get_ctx(), font_handler);
}

/*
 * Thread safe event registration
 */
void
NkWindowGDI::push_user_event(UserEvent* ev)
{
	m_gdi_impl->event_mutex.lock();
	m_gdi_impl->event_queue.push_back(*ev);
	m_gdi_impl->event_mutex.unlock();
}

void NkWindowGDI::shutdown() {
	std::map<std::string, gdi_font*>::iterator it = m_gdi_impl->font.begin();
	for (; it != m_gdi_impl->font.end(); ++it){
		DeleteObject(it->second->handle);
		DeleteDC(it->second->dc);
		delete it->second;
	}

	ReleaseDC(m_gdi_impl->wnd, m_gdi_impl->window_dc);
	UnregisterClassW(m_gdi_impl->wc.lpszClassName, m_gdi_impl->wc.hInstance);
}

UserEvent::UserEvent()
{
	NkWindowGDI* wingdi = (NkWindowGDI*)NkWindow::get();
	m_event_idx = 0;
	wingdi->register_user_event(this);
}

UserEvent::~UserEvent()
{
	NkWindowGDI* wingdi = (NkWindowGDI*)NkWindow::get();
	if (m_event_idx != -1)
		wingdi->unregister_user_event(this);
}

void UserEvent::push(int code, void* data1, void* data2)
{
	NkWindowGDI* wingdi = (NkWindowGDI*)NkWindow::get();
	m_event_idx = code;
	m_userdata1 = data1;
	m_userdata2 = data2;
	wingdi->push_user_event(this);
}

void
UserEvent::on_callback(void* data1, void* data2)
{
	if (m_callback)
		m_callback(this, (void*)this, m_callback_data);
}

