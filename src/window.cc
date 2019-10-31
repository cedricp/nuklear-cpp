#include <vector>

#include <nuklear_lib.h>
#include <socket_notifier.h>
#include <algorithm>
#include <time.h>
#include <sys/time.h>
#include <window.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <timer.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT

#include <nuklear.h>

NkWindow* global_ptr = NULL;

struct font_info{
	std::string filename;
	nk_font* font;
	float size;
};

struct win_impl {
	win_impl(int w, int h){
		running = true;
		height = w;
		width = h;
		nk_init_default(&ctx, NULL);
		font_tex = -1;
	}
	struct nk_context ctx;
	int width, height;
	bool running;
	std::vector<NkWidget*> widgets;
	std::vector<Socket_notifier*> sockets_notifiers;
	std::vector<UserEvent*> m_user_events;
	std::vector<Timer*> m_timers;
    struct nk_font_atlas atlas;
    struct nk_draw_null_texture null;
    std::map<std::string, font_info> fonts;
    GLuint font_tex;
};

NkWindow::NkWindow(int w, int h, bool aa) {
	m_impl = new win_impl(w, h);
	m_anti_aliasing = aa;
}

NkWindow::~NkWindow()
{
	for (NkWidget* widget : m_impl->widgets){
		delete widget;
	}
	delete m_impl;
}

struct nk_draw_null_texture*
NkWindow::get_draw_null_texture()
{
	return &m_impl->null;
}

void
NkWindow::cleanup()
{
	nk_font_atlas_clear(&m_impl->atlas);
	delete_texture(m_impl->font_tex);
}

NkWindow*
NkWindow::get()
{
	return global_ptr;
}

void
NkWindow::add_widget(NkWidget* w)
{
	m_impl->widgets.push_back(w);
	w->set_parent_window(this);
}


void
NkWindow::register_fd_event(Socket_notifier* sn)
{
	if (std::find(m_impl->sockets_notifiers.begin(), m_impl->sockets_notifiers.end(), sn) == m_impl->sockets_notifiers.end())
		m_impl->sockets_notifiers.push_back(sn);
}

void
NkWindow::unregister_fd_event(Socket_notifier* sn)
{
	std::vector<Socket_notifier*>::iterator it = std::find(m_impl->sockets_notifiers.begin(), m_impl->sockets_notifiers.end(), sn);
	if (it != m_impl->sockets_notifiers.end())
		m_impl->sockets_notifiers.erase(it);

}


void NkWindow::register_user_event(UserEvent* ev) {
	std::vector<UserEvent*>::iterator it = std::find(m_impl->m_user_events.begin(), m_impl->m_user_events.end(), ev);
	if (it != m_impl->m_user_events.end())
		return;
	m_impl->m_user_events.push_back(ev);
}

void NkWindow::unregister_user_event(UserEvent* ev) {
	std::vector<UserEvent*>::iterator it = std::find(m_impl->m_user_events.begin(), m_impl->m_user_events.end(), ev);
	if (it == m_impl->m_user_events.end())
		return;
	m_impl->m_user_events.erase(it);
}

void NkWindow::register_timer(Timer* t)
{
	std::vector<Timer*>::iterator it = std::find(m_impl->m_timers.begin(), m_impl->m_timers.end(), t);
	if (it != m_impl->m_timers.end())
		return;
	m_impl->m_timers.push_back(t);
}

void NkWindow::unregister_timer(Timer* t)
{
	std::vector<Timer*>::iterator it = std::find(m_impl->m_timers.begin(), m_impl->m_timers.end(), t);
	if (it == m_impl->m_timers.end())
		return;
	m_impl->m_timers.erase(it);
}

int NkWindow::get_width() {
	return m_impl->width;
}

int NkWindow::get_height() {
	return m_impl->height;
}

nk_context* NkWindow::get_ctx() {
	return &m_impl->ctx;
}

std::vector<NkWidget*> NkWindow::get_widgets() {
	return m_impl->widgets;
}

bool NkWindow::is_running() {
	return m_impl->running;
}

void NkWindow::set_running(bool r) {
	m_impl->running = r;
}

std::vector<UserEvent*>& NkWindow::get_user_events() {
	return m_impl->m_user_events;
}


unsigned long
NkWindow::timestamp(void)
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) < 0) return 0;
    return (unsigned long)((unsigned long)tv.tv_sec * 1000 + (unsigned long)tv.tv_usec/1000);
}

void
NkWindow::set_stye(StyleTheme theme)
{
    struct nk_color table[NK_COLOR_COUNT];
    if (theme == THEME_WHITE) {
        table[NK_COLOR_TEXT] = nk_rgba(70, 70, 70, 255);
        table[NK_COLOR_WINDOW] = nk_rgba(175, 175, 175, 255);
        table[NK_COLOR_HEADER] = nk_rgba(175, 175, 175, 255);
        table[NK_COLOR_BORDER] = nk_rgba(0, 0, 0, 255);
        table[NK_COLOR_BUTTON] = nk_rgba(185, 185, 185, 255);
        table[NK_COLOR_BUTTON_HOVER] = nk_rgba(170, 170, 170, 255);
        table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(160, 160, 160, 255);
        table[NK_COLOR_TOGGLE] = nk_rgba(150, 150, 150, 255);
        table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(120, 120, 120, 255);
        table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(175, 175, 175, 255);
        table[NK_COLOR_SELECT] = nk_rgba(190, 190, 190, 255);
        table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(175, 175, 175, 255);
        table[NK_COLOR_SLIDER] = nk_rgba(190, 190, 190, 255);
        table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(80, 80, 80, 255);
        table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(70, 70, 70, 255);
        table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(60, 60, 60, 255);
        table[NK_COLOR_PROPERTY] = nk_rgba(175, 175, 175, 255);
        table[NK_COLOR_EDIT] = nk_rgba(150, 150, 150, 255);
        table[NK_COLOR_EDIT_CURSOR] = nk_rgba(0, 0, 0, 255);
        table[NK_COLOR_COMBO] = nk_rgba(175, 175, 175, 255);
        table[NK_COLOR_CHART] = nk_rgba(160, 160, 160, 255);
        table[NK_COLOR_CHART_COLOR] = nk_rgba(45, 45, 45, 255);
        table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba( 255, 0, 0, 255);
        table[NK_COLOR_SCROLLBAR] = nk_rgba(180, 180, 180, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(140, 140, 140, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(150, 150, 150, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(160, 160, 160, 255);
        table[NK_COLOR_TAB_HEADER] = nk_rgba(180, 180, 180, 255);
        nk_style_from_table(&m_impl->ctx, table);
    } else if (theme == THEME_RED) {
        table[NK_COLOR_TEXT] = nk_rgba(190, 190, 190, 255);
        table[NK_COLOR_WINDOW] = nk_rgba(30, 33, 40, 215);
        table[NK_COLOR_HEADER] = nk_rgba(181, 45, 69, 220);
        table[NK_COLOR_BORDER] = nk_rgba(51, 55, 67, 255);
        table[NK_COLOR_BUTTON] = nk_rgba(181, 45, 69, 255);
        table[NK_COLOR_BUTTON_HOVER] = nk_rgba(190, 50, 70, 255);
        table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(195, 55, 75, 255);
        table[NK_COLOR_TOGGLE] = nk_rgba(51, 55, 67, 255);
        table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(45, 60, 60, 255);
        table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(181, 45, 69, 255);
        table[NK_COLOR_SELECT] = nk_rgba(51, 55, 67, 255);
        table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(181, 45, 69, 255);
        table[NK_COLOR_SLIDER] = nk_rgba(51, 55, 67, 255);
        table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(181, 45, 69, 255);
        table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(186, 50, 74, 255);
        table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(191, 55, 79, 255);
        table[NK_COLOR_PROPERTY] = nk_rgba(51, 55, 67, 255);
        table[NK_COLOR_EDIT] = nk_rgba(51, 55, 67, 225);
        table[NK_COLOR_EDIT_CURSOR] = nk_rgba(190, 190, 190, 255);
        table[NK_COLOR_COMBO] = nk_rgba(51, 55, 67, 255);
        table[NK_COLOR_CHART] = nk_rgba(51, 55, 67, 255);
        table[NK_COLOR_CHART_COLOR] = nk_rgba(170, 40, 60, 255);
        table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba( 255, 0, 0, 255);
        table[NK_COLOR_SCROLLBAR] = nk_rgba(30, 33, 40, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(64, 84, 95, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(70, 90, 100, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(75, 95, 105, 255);
        table[NK_COLOR_TAB_HEADER] = nk_rgba(181, 45, 69, 220);
        nk_style_from_table(&m_impl->ctx, table);
    } else if (theme == THEME_BLUE) {
        table[NK_COLOR_TEXT] = nk_rgba(20, 20, 20, 255);
        table[NK_COLOR_WINDOW] = nk_rgba(202, 212, 214, 215);
        table[NK_COLOR_HEADER] = nk_rgba(137, 182, 224, 220);
        table[NK_COLOR_BORDER] = nk_rgba(140, 159, 173, 255);
        table[NK_COLOR_BUTTON] = nk_rgba(137, 182, 224, 255);
        table[NK_COLOR_BUTTON_HOVER] = nk_rgba(142, 187, 229, 255);
        table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(147, 192, 234, 255);
        table[NK_COLOR_TOGGLE] = nk_rgba(177, 210, 210, 255);
        table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(182, 215, 215, 255);
        table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(137, 182, 224, 255);
        table[NK_COLOR_SELECT] = nk_rgba(177, 210, 210, 255);
        table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(137, 182, 224, 255);
        table[NK_COLOR_SLIDER] = nk_rgba(177, 210, 210, 255);
        table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(137, 182, 224, 245);
        table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(142, 188, 229, 255);
        table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(147, 193, 234, 255);
        table[NK_COLOR_PROPERTY] = nk_rgba(210, 210, 210, 255);
        table[NK_COLOR_EDIT] = nk_rgba(210, 210, 210, 225);
        table[NK_COLOR_EDIT_CURSOR] = nk_rgba(20, 20, 20, 255);
        table[NK_COLOR_COMBO] = nk_rgba(210, 210, 210, 255);
        table[NK_COLOR_CHART] = nk_rgba(210, 210, 210, 255);
        table[NK_COLOR_CHART_COLOR] = nk_rgba(137, 182, 224, 255);
        table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba( 255, 0, 0, 255);
        table[NK_COLOR_SCROLLBAR] = nk_rgba(190, 200, 200, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(64, 84, 95, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(70, 90, 100, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(75, 95, 105, 255);
        table[NK_COLOR_TAB_HEADER] = nk_rgba(156, 193, 220, 255);
        nk_style_from_table(&m_impl->ctx, table);
    } else if (theme == THEME_DARK) {
        table[NK_COLOR_TEXT] = nk_rgba(210, 210, 210, 255);
        table[NK_COLOR_WINDOW] = nk_rgba(57, 67, 71, 215);
        table[NK_COLOR_HEADER] = nk_rgba(51, 51, 56, 220);
        table[NK_COLOR_BORDER] = nk_rgba(46, 46, 46, 255);
        table[NK_COLOR_BUTTON] = nk_rgba(48, 83, 111, 255);
        table[NK_COLOR_BUTTON_HOVER] = nk_rgba(58, 93, 121, 255);
        table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(63, 98, 126, 255);
        table[NK_COLOR_TOGGLE] = nk_rgba(50, 58, 61, 255);
        table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(45, 53, 56, 255);
        table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(48, 83, 111, 255);
        table[NK_COLOR_SELECT] = nk_rgba(57, 67, 61, 255);
        table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(48, 83, 111, 255);
        table[NK_COLOR_SLIDER] = nk_rgba(50, 58, 61, 255);
        table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(48, 83, 111, 245);
        table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(53, 88, 116, 255);
        table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(58, 93, 121, 255);
        table[NK_COLOR_PROPERTY] = nk_rgba(50, 58, 61, 255);
        table[NK_COLOR_EDIT] = nk_rgba(50, 58, 61, 225);
        table[NK_COLOR_EDIT_CURSOR] = nk_rgba(210, 210, 210, 255);
        table[NK_COLOR_COMBO] = nk_rgba(50, 58, 61, 255);
        table[NK_COLOR_CHART] = nk_rgba(50, 58, 61, 255);
        table[NK_COLOR_CHART_COLOR] = nk_rgba(48, 83, 111, 255);
        table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba(255, 0, 0, 255);
        table[NK_COLOR_SCROLLBAR] = nk_rgba(50, 58, 61, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(48, 83, 111, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(53, 88, 116, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(58, 93, 121, 255);
        table[NK_COLOR_TAB_HEADER] = nk_rgba(48, 83, 111, 255);
        nk_style_from_table(&m_impl->ctx, table);
    } else {
        nk_style_default(&m_impl->ctx);
    }
}

void
NkWindow::handle_fd_events()
{
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	for(Socket_notifier* sn : m_impl->sockets_notifiers){
		std::vector<int> fds = sn->get_fds();
		for (int fd : fds){
			fd_set set;
			FD_ZERO (&set);
			FD_SET (fd, &set);
			if (select(fd+1, &set, NULL, NULL, &tv) > 0){
				sn->data_available(fd);
			}
		}
	}
}

void
NkWindow::handle_timer_events()
{
	std::vector<Timer*>::iterator it_timer = m_impl->m_timers.begin();
	long timestamp = this->timestamp();
	for (; it_timer < m_impl->m_timers.end(); ++it_timer){
		if ((*it_timer)->is_active()){
			if (timestamp - (*it_timer)->get_start_time()  >= (*it_timer)->get_time()){
				(*it_timer)->on_timer_event();
			}
		}
	}
}



void
NkWindow::use_font(std::string font_name)
{
	if (m_impl->fonts.find(font_name) == m_impl->fonts.end()){
		fprintf(stderr, "SDLWindow::use_font : Cannot use font %s\n", font_name.c_str());
		return;
	}
	nk_style_set_font(get_ctx(), &m_impl->fonts[font_name].font->handle);
	printf("Use [%s] [%x]\n", font_name.c_str(), m_impl->fonts[font_name].font);
}

void
NkWindow::add_font(std::string font_name, std::string filename, float size)
{
	font_info fi;
	fi.filename = filename;
	fi.size = size;
	fi.font = NULL;
	m_impl->fonts[font_name] = fi;
}

struct nk_font*
NkWindow::get_user_font(std::string font_name)
{
	if (font_name == "native" || m_impl->fonts.find(font_name) == m_impl->fonts.end()){
		return m_impl->atlas.default_font;
	}
	return m_impl->fonts[font_name].font;
}

void
NkWindow::load_fonts()
{
	struct nk_font_atlas* atlas = &m_impl->atlas;
    nk_font_atlas_init_default(atlas);
    nk_font_atlas_begin(atlas);

    // Add user fonts
    for (auto& keyval: m_impl->fonts){
    	keyval.second.font = nk_font_atlas_add_from_file(atlas, keyval.second.filename.c_str(), keyval.second.size, 0);
    }

    // Add native font
	font_info fi;
    fi.font = nk_font_atlas_add_default(atlas, 13.0f, 0);
    m_impl->fonts["native"] = fi;

	const void *image; int w, h;
	image = nk_font_atlas_bake(atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
	m_impl->font_tex = device_upload_atlas((void*)image, w, h);
	nk_font_atlas_end(&m_impl->atlas,
			nk_handle_id((int)m_impl->font_tex),
			get_draw_null_texture());

	// Set default font
	atlas->default_font = fi.font;
}

GLuint
NkWindow::device_upload_atlas(void *image, int width, int height)
{
    NkTexture font_texture(image, NkTexture::TYPE_UCHAR, width, height, NkTexture::RGBA_BYTE);
    return create_texture(font_texture);
}
