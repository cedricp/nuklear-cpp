#include <nuklear_lib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <time.h>
#include <sys/time.h>

#include <window.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#undef NK_IMPLEMENTATION
#undef NK_SDL_GLES2_IMPLEMENTATION
#include <nuklear.h>

/*
 * Widget base
 */

inline NkColor convert_color(nk_color c){
	NkColor nkc(c.r, c.g, c.b, c.a);
	return nkc;
}

inline nk_color to_nkcolor(NkColor& a){
	return nk_rgba(a.r, a.g, a.b, a.a);
}

inline struct nk_rect to_nkrect(NkRect& a){
	return nk_rect(a.x, a.y, a.w, a.h);
}

inline struct nk_vec2 to_nkvec2(NkVec2& a){
	return nk_vec2(a.x, a.y);
}

void
NkBase::push_text_attributes(struct nk_context* ctx)
{
	if (m_font){
		nk_style_push_font(ctx, &m_font->handle);
	}
}

void
NkBase::pop_text_attributes(struct nk_context* ctx)
{
	if (m_font)
		nk_style_pop_font(ctx);
}

void
NkBase::set_text_font(std::string font)
{
	m_font = NkWindow::get()->get_user_font(font);
}



NkWidget::NkWidget(std::string name, int posx, int posy, int width, int height){
	m_name = name;
	m_x = posx;
	m_y = posy;
	m_width = width;
	m_height = height;
	m_flags = NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
			  NK_WINDOW_CLOSABLE|NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE;
	is_hovered = false;
}

NkWidget::~NkWidget()
{
	for (int i = 0; i <  m_elements.size(); ++i){
		delete m_elements[i];
	}
}

void
NkWidget::add_element(NkBase* elem)
{
	m_elements.push_back(elem);
	elem->set_parent(this);
	elem->set_parent_window(m_parent_window);
	elem->set_parent(this);
}



void
NkWidget::draw(nk_context* ctx)
{
	if (nk_begin(ctx, m_name.c_str(), nk_rect(m_x, m_y, m_width, m_height), m_flags)){
		if (nk_window_is_hovered(ctx)){
			is_hovered = true;
		}
		for (int i = 0; i <  m_elements.size(); ++i){
			m_elements[i]->draw(ctx);
		}
	}
	nk_end(ctx);
}

void
NkWidget::set_scalable(bool on)
{
	if (on)
		m_flags |= NK_WINDOW_SCALABLE;
	else
		m_flags &= ~NK_WINDOW_SCALABLE;
}

void
NkWidget::set_movable(bool on)
{
	if (on)
		m_flags |= NK_WINDOW_MOVABLE;
	else
		m_flags &= ~NK_WINDOW_MOVABLE;
}

void
NkWidget::set_borders(bool on)
{
	if (on)
		m_flags |= NK_WINDOW_BORDER;
	else
		m_flags &= ~NK_WINDOW_BORDER;
}
void
NkWidget::set_closable(bool on)
{
	if (on)
		m_flags |= NK_WINDOW_CLOSABLE;
	else
		m_flags ^= ~NK_WINDOW_CLOSABLE;
}

void
NkWidget::set_minimizable(bool on)
{
	if (on)
		m_flags |= NK_WINDOW_MINIMIZABLE;
	else
		m_flags &= ~NK_WINDOW_MINIMIZABLE;
}

void
NkWidget::set_title_on(bool on)
{
	if (on)
		m_flags |= NK_WINDOW_TITLE;
	else
		m_flags &= ~NK_WINDOW_TITLE;
}

void
NkWidget::set_title_name(std::string name)
{
	m_name = name;
}

/*
 * Label
 */


NkLabel::NkLabel(std::string text, Text_aligment align)
{
	m_text = text;
	m_text_alignment = align;
}

void NkLabel::draw(nk_context* ctx)
{
	push_text_attributes(ctx);
	nk_label(ctx, m_text.c_str(), m_text_alignment);
	pop_text_attributes(ctx);
}

/*
 * Button base
 */

NkButton::NkButton(std::string name, Symbol symbol)
{
	m_name = name;
	m_callback = NULL;
	m_symbol = symbol;
}

void
NkButton::draw(nk_context* ctx)
{
	if (m_symbol == NONE && !m_name.empty()){
		if (nk_button_label(ctx, m_name.c_str()) && m_callback)
			m_callback(this, 0L, m_callback_data);
	} else if (m_name.empty() && m_symbol != NONE){
		if (nk_button_symbol(ctx, (nk_symbol_type)m_symbol) && m_callback)
			m_callback(this, 0L, m_callback_data);
	} else {
		if (nk_button_symbol_label(ctx, (nk_symbol_type)m_symbol, m_name.c_str(), NK_TEXT_CENTERED) && m_callback)
			m_callback(this, 0L, m_callback_data);
	}
}


/*
 * Layout base
 */

NkLayoutRow::NkLayoutRow(int height, int item_width, int cols, bool dynamic)
{
	m_height = height;
	m_item_width = item_width;
	m_num_columns = cols;
	m_num_rows = 1;
	m_is_row_dynamic = dynamic;
}

NkLayoutRow::~NkLayoutRow()
{
	for (int i = 0; i < m_elements.size(); ++i){
		delete m_elements[i];
	}
}

void
NkLayoutRow::add_element(NkBase* elem)
{
	m_elements.push_back(elem);
	elem->set_parent(this);
	elem->set_parent_window(m_parent_window);
	elem->set_parent(this);
	m_num_rows = m_num_columns > 0 ? m_elements.size() / m_num_columns : 1;
}

void
NkLayoutRow::draw(nk_context* ctx)
{
	if ((m_elements.size() % m_num_columns) != 0){
		fprintf(stderr, "NkLayoutRow::draw problem, bad column number\n");
		return;
	}

	if (m_is_row_dynamic){
		nk_layout_row_begin(ctx, NK_STATIC, m_height, m_num_columns);
	} else {
		if (m_item_width > -1)
			nk_layout_row_static(ctx, m_height, m_item_width, m_num_columns);
		else
			nk_layout_row_dynamic(ctx, m_height, m_num_columns);
	}

	for (int i = 0; i < m_elements.size(); ++i){
		if (m_is_row_dynamic && m_elements[i]->get_row_width() > 0){
			nk_layout_row_push(ctx, m_elements[i]->get_row_width());
		}
		struct nk_rect bound = nk_widget_bounds(ctx);
		m_elements[i]->m_bound = *((struct bound*)&bound);
		m_elements[i]->draw(ctx);
	}

	if (m_is_row_dynamic){
		nk_layout_row_end(ctx);
	}
}


NkLayoutRowDynamic::NkLayoutRowDynamic(int height, int cols, bool dynamic) : NkLayoutRow(height, -1, cols, dynamic)
{

}

/*
 * Checkbox
 */

NkCheckbox::NkCheckbox(std::string name)
{
	m_name = name;
	m_checked = 0;
}

void
NkCheckbox::draw(nk_context* ctx)
{
	push_text_attributes(ctx);
	int check_test = m_checked;
	nk_checkbox_label(ctx, m_name.c_str(), &m_checked);
	if (m_checked != check_test && m_callback){
		m_callback(this, &m_checked, m_callback_data);
	}
	pop_text_attributes(ctx);
}

/*
 * CheckList
 */

NkCheckList::NkCheckList(std::vector<std::string> names)
{
	m_names = names;
	m_checkedval = 0;
}

void
NkCheckList::draw(nk_context* ctx)
{
	push_text_attributes(ctx);
	int current_checkedval = m_checkedval;
	for (int i = 0; i < m_names.size(); ++i){
		if(nk_option_label(ctx, m_names[i].c_str(), i == m_checkedval)){
			m_checkedval = i;
		}
	}
	if (current_checkedval != m_checkedval && m_callback){
		m_callback(this, &m_checkedval, m_callback_data);
	}
	pop_text_attributes(ctx);
}

/*
 * Tree
 */

NkTree::NkTree(bool node_type)
{
	m_node_type = node_type;
}

NkTree::~NkTree()
{

}

void
NkTree::add_element(NkBase* elem, std::string tree_name, bool minimized)
{
	m_elements.push_back(elem);
	elem->set_parent(this);
	m_names.push_back(tree_name);
	m_minimized.push_back(minimized);
	elem->set_parent_window(m_parent_window);
	elem->set_parent(this);
}

void NkTree::draw(nk_context* ctx)
{
	for (int i = 0; i < m_elements.size(); ++i){
		nk_collapse_states* state = (nk_collapse_states*)&m_minimized[i];
		push_text_attributes(ctx);
		if (nk_tree_state_push(ctx, m_node_type ? NK_TREE_NODE : NK_TREE_TAB, m_names[i].c_str(), state)){
			pop_text_attributes(ctx);
			m_elements[i]->draw(ctx);
			nk_tree_pop(ctx);
		} else {
			pop_text_attributes(ctx);
		}
	}
}


/*
 * Image
 */

struct image_impl {
	struct nk_image img;
	GLuint texid;
	NkTexture texture;
};

NkImage::NkImage(std::string filename)
{
	m_impl = new image_impl;
    int n;
    m_impl->texid = -1;
    m_impl->texture.pixel_data_type = NkTexture::TYPE_UCHAR;
    m_impl->texture.internal_format = NkTexture::RGBA_BYTE;
    m_impl->texture.data = stbi_load(filename.c_str(), &m_impl->texture.width, &m_impl->texture.height, &n, 0);
    if (!m_impl->texture.data){
    	fprintf(stderr, "NkImage::NkImage : Cannot load image %s\n", filename.c_str());
    	return;
    }

    generate_texture();

    stbi_image_free(m_impl->texture.data);
    m_impl->texture.data = NULL;
}

NkImage::NkImage(int size_x, int size_y)
{
	m_impl = new image_impl;
	int buffer_size = size_x * size_y * 4;
	m_impl->texture.width = size_x;
	m_impl->texture.height = size_y;
	m_impl->texture.data = (unsigned char*)malloc(buffer_size);
	m_impl->texture.internal_format = NkTexture::RGBA_BYTE;
	memset(m_impl->texture.data, 0, buffer_size);
	generate_texture();
	free(m_impl->texture.data);
}

NkImage::NkImage(const NkTexture& texture)
{
	m_impl = new image_impl;
	m_impl->texture = texture;
	generate_texture();
}

void*
NkImage::get_data()
{
	return m_impl->texture.data;
}

NkImage::~NkImage()
{
	if (m_impl->texture.data)
		free(m_impl->texture.data);
	if (m_impl->texid != -1)
		NkWindow::get()->delete_texture(m_impl->texid);
	delete m_impl;
}

int
NkImage::get_width()
{
	return m_impl->texture.width;
}

int
NkImage::get_height()
{
	return m_impl->texture.height;
}

void
NkImage::generate_texture()
{
	if (m_impl->texid != -1){
		NkWindow::get()->delete_texture(m_impl->texid);
	}

	m_impl->texture.internal_format = NkTexture::RGBA_BYTE;

	m_impl->texid = NkWindow::get()->create_texture(m_impl->texture);
	m_impl->img = nk_image_id((int)m_impl->texid);
}

void
NkImage::update_texture()
{
	generate_texture();
}

struct nk_image*
NkImage::get_image()
{
	if (m_impl->texid == -1){
		return NULL;
	}
	return &m_impl->img;
}

void
NkImage::draw(nk_context* ctx)
{
	if (m_impl->texid == -1)
		return;
	nk_image(ctx, m_impl->img);
}

/*
 * Canvas window
 */


struct cavas_impl{
    struct nk_vec2 item_spacing;
    struct nk_vec2 panel_padding;
    nk_style_item window_background;
    nk_color bg_color;
    int flags;
    int x, y, width, height;
};

NkCanvasWindow::NkCanvasWindow(std::string name, int x, int y, int width, int height) : NkWidget(name, x, y, width, height)
{
	m_impl = new cavas_impl;
	m_impl->flags = 0;
	m_impl->bg_color.r = 20;
	m_impl->bg_color.g = 20;
	m_impl->bg_color.g = 20;
	m_impl->bg_color.a = 255;
}

NkCanvasWindow::~NkCanvasWindow()
{
	delete m_impl;
}

void
NkCanvasWindow::draw(nk_context* ctx)
{
	nk_style_item tmp_bg = ctx->style.window.fixed_background;
    ctx->style.window.fixed_background = nk_style_item_color(m_impl->bg_color);

    if (nk_begin(ctx, m_name.c_str(), nk_rect(m_x, m_y, m_width, m_height), NK_WINDOW_NO_SCROLLBAR|m_flags)){
    	struct nk_rect region = nk_window_get_content_region(ctx);
    	NkRect bounds(region.x, region.y, region.w, region.h);
    	m_painter.set_bounds(bounds);
    	m_painter.set_context(ctx);
		/* allocate the complete window space for drawing */
		struct nk_rect total_space;
		total_space = nk_window_get_content_region(ctx);
		nk_layout_row_dynamic(ctx, total_space.h, 1);
		nk_widget(&total_space, ctx);

		draw_content();
    }

    nk_end(ctx);
    ctx->style.window.fixed_background = tmp_bg;
}

void NkCanvasWindow::draw_content()
{
	NkColor col(70,20,20,255);
    std::vector<NkVec2> points;
    points.resize(6);
    points[0].x = 100; points[0].y = 150;
    points[1].x = 150; points[1].y = 250;
    points[2].x = 125; points[2].y = 250;
    points[3].x = 100; points[3].y = 200;
    points[4].x = 75; points[4].y = 250;
    points[5].x = 50; points[5].y = 250;

	m_painter.draw_fill_polygon(points, 1, col);
	m_painter.draw_text("NO CONTENT YET", NkRect(10,0, 120, 20), NkColor(0,255,0,255));
}

NkPainter::NkPainter()
{
	m_ctx = NULL;
	m_painter = NULL;
}

void
NkPainter::set_context(nk_context* ctx){
	m_ctx = ctx;
	m_painter = nk_window_get_canvas(ctx);
}

void
NkPainter::draw_fill_polygon(std::vector<NkVec2>& points, int width, const NkColor& colors)
{
	for (int i = 0; i <  points.size(); ++i){
		points[i].x += m_bounds.x;
		points[i].y += m_bounds.y;
	}
	nk_fill_polygon(m_painter, (float*)points.data(), points.size(), nk_rgb(colors.r, colors.g, colors.b));
	for (int i = 0; i <  points.size(); ++i){
		points[i].x -= m_bounds.x;
		points[i].y -= m_bounds.y;
	}
}

void
NkPainter::draw_fill_polygon(std::vector<NkVec2>& points, int width, float thickness, const NkColor& colors)
{
	for (int i = 0; i <  points.size(); ++i){
		points[i].x += m_bounds.x;
		points[i].y += m_bounds.y;
	}
	nk_stroke_polygon(m_painter, (float*)points.data(), points.size(), thickness, nk_rgb(colors.r, colors.g, colors.b));
	for (int i = 0; i <  points.size(); ++i){
		points[i].x -= m_bounds.x;
		points[i].y -= m_bounds.y;
	}
}

void
NkPainter::draw_image(NkImage& img, NkRect rect, NkColor color)
{
	rect.x += m_bounds.x;
	rect.y += m_bounds.y;
	if (img.get_image())
		nk_draw_image(m_painter, to_nkrect(rect), img.get_image(), to_nkcolor(color));
}

void
NkPainter::draw_text(std::string text, NkRect r, NkColor color)
{
	r.x += m_bounds.x;
	r.y += m_bounds.y;
	struct nk_color blk = {0,0,0,0};
	nk_draw_text(m_painter, to_nkrect(r), text.c_str(), text.length(), m_ctx->style.font, blk, to_nkcolor(color));
}

void
NkPainter::draw_stroke_line(NkVec2 a, NkVec2 b, float thickness, NkColor& color)
{
	a.x += m_bounds.x;
	a.y += m_bounds.y;

	b.x += m_bounds.x;
	b.y += m_bounds.y;

	nk_stroke_line(m_painter, a.x, a.y, b.x, b.y, thickness, to_nkcolor(color));
}

void
NkPainter::draw_fill_rect(NkRect rect, float rounding, NkColor& color)
{
	rect.x += m_bounds.x;
	rect.y += m_bounds.y;

	nk_fill_rect(m_painter, to_nkrect(rect), rounding, to_nkcolor(color));
}

/*
 * Combos
 */


NkCombo::NkCombo(std::string name)
{
	m_name = name;
	m_selected = 0;
}

void
NkCombo::draw(nk_context* ctx)
{
	push_text_attributes(ctx);
	std::vector<char*> cstrings;
	cstrings.reserve(m_elems.size());

	for(size_t i = 0; i < m_elems.size(); ++i)
		cstrings.push_back(const_cast<char*>(m_elems[i].c_str()));
	int old_selected = m_selected;
	m_selected = nk_combo(ctx, (const char**)cstrings.data(), m_elems.size(), m_selected, 20,  nk_vec2(200,200));

	if (m_selected != old_selected && m_callback){
		m_callback(this, &m_selected, m_callback_data);
	}
	pop_text_attributes(ctx);
}

void
NkCombo::add_element(std::string elem_name)
{
	m_elems.push_back(elem_name);
}

/*
 * Combos images
 */


NkComboImage::NkComboImage(std::string name, int default_height)
{
	m_name = name;
	m_height = default_height;
	m_selected = 0;
}

void
NkComboImage::draw(nk_context* ctx)
{
	if (m_names.empty()){
		return;
	}
	push_text_attributes(ctx);
	int old_selected = m_selected;
	struct nk_image* current_img = m_elems[m_selected]->get_image();
	if ( nk_combo_begin_image_label(ctx, m_name.c_str(), *current_img, nk_vec2(nk_widget_width(ctx), m_height)) ){
		nk_layout_row_dynamic(ctx, m_height, 1);
        for (int i = 0; i < m_elems.size(); ++i){
        	struct nk_image* current_img = m_elems[i]->get_image();
            if (nk_combo_item_image_label(ctx, *current_img, "Text", NK_TEXT_RIGHT)){
            	m_selected = i;
            }
        }
        nk_combo_end(ctx);
	}

	if (m_selected != old_selected && m_callback){
		m_callback(this, &m_selected, m_callback_data);
	}
	pop_text_attributes(ctx);
}

void
NkComboImage::add_element(NkImage* image, std::string name)
{
	if (image->get_image()){
		m_elems.push_back(image);
		m_names.push_back(name);
	} else {
		fprintf(stderr, "Cannot insert NULL image %s\n", name.c_str());
	}
}

/*
 * Slider float
 */

NkSliderFloat::NkSliderFloat(std::string name, float min, float max, float val, float step)
{
	m_name = name;
	m_min = min;
	m_max = max;
	m_value = val;
	m_step = step;
}

void
NkSliderFloat::draw(nk_context* ctx)
{
	push_text_attributes(ctx);
	float old_value = m_value;
	nk_slider_float(ctx, m_min, &m_value, m_max, 1);

	if (old_value != m_value && m_callback){
		m_callback(this, &m_value, m_callback_data);
	}
	pop_text_attributes(ctx);
}

/*
 * Slider integer
 */

NkSliderInt::NkSliderInt(std::string name, int min, int max, int val, int step)
{
	m_name = name;
	m_min = min;
	m_max = max;
	m_value = val;
	m_step = step;
}

void
NkSliderInt::draw(nk_context* ctx)
{
	int old_value = m_value;
	nk_slider_int(ctx, m_min, &m_value, m_max, 1);

	if (old_value != m_value && m_callback){
		m_callback(this, &m_value, m_callback_data);
	}

}

/*
 * Progress
 */


NkProgress::NkProgress(std::string name, int max, int val, bool modifiable)
{
	m_name = name;
	m_max = max;
	m_value = val;
	m_modifiable = modifiable;
}

void NkProgress::draw(nk_context* ctx)
{
	int old_value = m_value;
	push_text_attributes(ctx);
	nk_progress(ctx, (nk_size*)&m_value, m_max, m_modifiable ? NK_MODIFIABLE : NK_FIXED);
	pop_text_attributes(ctx);

	if (old_value != m_value && m_callback){
		m_callback(this, &m_value, m_callback_data);
	}
}

void
NkProgress::set_value(int val, bool do_callback){
	bool value_modifed = m_value != val;
	m_value = val;
	if (value_modifed && do_callback && m_callback){
		m_callback(this, &m_value, m_callback_data);
	}
}

/*
 * Canvas
 */

NkCanvas::NkCanvas(std::string name)
{
	m_name = name;
}

NkCanvas::~NkCanvas()
{

}

void NkCanvas::draw_content()
{
	NkColor col(255,255,0,255);
	m_painter.draw_text("NO CONTENT YET", NkRect(10,10, 160, 20), col);
//	m_painter.draw_stroke_line(NkVec2(20,20), NkVec2(100,100), 2.0f, col);
//	m_painter.draw_fill_rect(NkRect(20,20, 40, 10), 1.f,  col);
}

void
NkCanvas::draw(nk_context* ctx)
{
    struct nk_window *win;
    struct nk_panel *layout;
    const struct nk_style *style;
    struct nk_rect bounds;

    win = ctx->current;
    style = &ctx->style;
    layout = win->layout;

    int state = nk_widget(&bounds, ctx);
    if (!state) return;
	m_painter.set_context(ctx);
	m_painter.set_bounds(NkRect(bounds.x, bounds.y, bounds.w, bounds.h));

	if (nk_input_has_mouse_click_down_in_rect(&ctx->input, NK_BUTTON_LEFT, bounds, 1)){
		int x = ctx->input.mouse.pos.x - bounds.x;
		int y = ctx->input.mouse.pos.y - bounds.y;
		printf("Click %i %i!\n", x, y);
	}

	draw_content();
}

inline int min(int a, int b)
{
	return ((a) < (b) ? (a) : (b));
}

NkEditValue::NkEditValue(Text_filter fi, size_t buffer_size)
{
	m_filter = fi;
	m_buffer_size = buffer_size + 1;
	m_text = (char*)malloc(m_buffer_size);
	m_old_text = (char*)malloc(m_buffer_size);
	memset(m_text, 0, sizeof(m_text));
	m_text_len = 0;
	m_old_state = NK_EDIT_INACTIVE;
	m_flags = NK_EDIT_FIELD | NK_EDIT_SIG_ENTER;
}

NkEditValue::~NkEditValue()
{
	free(m_text);
	free(m_old_text);
}

void NkEditValue::draw(nk_context* ctx)
{
	push_text_attributes(ctx);
	nk_flags ret_flags = 0;

	nk_plugin_filter filter = NULL;

	switch (m_filter){
	case FILTER_NONE:
		filter = nk_filter_default;
		break;
	case FILTER_ASCII:
		filter = nk_filter_ascii;
		break;
	case FILTER_FLT:
		filter = nk_filter_float;
		break;
	case FILTER_INT:
		filter = nk_filter_decimal;
		break;
	case FILTER_HEX:
		filter = nk_filter_hex;
		break;
	case FILTER_OCT:
		filter = nk_filter_oct;
		break;
	case FILTER_BIN:
		filter = nk_filter_binary;
		break;
	}

	bool autolayout = false;
	if (m_autolayout && get_row_height() > 0){
		nk_layout_row_dynamic(ctx, get_row_height(), 1);
		autolayout = true;
	} else if (m_autolayout && m_font){
		nk_layout_row_dynamic(ctx, m_font->info.height, 1);
		autolayout = true;
	}

	ret_flags = nk_edit_string(ctx, m_flags, m_text, &m_text_len, 64, filter);


	if((ret_flags & NK_EDIT_COMMITED) || (m_old_state != ret_flags && (ret_flags & NK_EDIT_DEACTIVATED))){
		m_text[m_text_len] = 0;
		bool text_changed = std::string(m_old_text) != std::string(m_text);
		if (text_changed){
			strcpy(m_old_text, m_text);
			if(m_callback){
				m_callback(this, m_text, m_callback_data);
			}
		}
	}

	m_old_state = ret_flags;
	pop_text_attributes(ctx);
}

void
NkEditValue::set_value(double val)
{
	snprintf(m_text, m_buffer_size, "%f", val);
	strcpy(m_old_text, m_text);
	m_text_len = strlen(m_text);
}

void
NkEditValue::set_multiline(bool m)
{
	if (m){
		m_flags |= NK_EDIT_MULTILINE;
		m_flags &= ~NK_EDIT_SIG_ENTER;
	} else {
		m_flags &= ~NK_EDIT_MULTILINE;
		m_flags |= NK_EDIT_SIG_ENTER;
	}
}

void
NkEditValue::set_readonly(bool m)
{
	if (m){
		m_flags |= NK_EDIT_READ_ONLY;
	} else {
		m_flags &= ~NK_EDIT_READ_ONLY;
	}
}

void
NkEditValue::set_value(long val)
{
	snprintf(m_text, m_buffer_size, "%i", val);
	strcpy(m_old_text, m_text);
	m_text_len = strlen(m_text);
}

void
NkEditValue::set_value(std::string txt)
{
	strncpy(m_text, txt.c_str(), m_buffer_size);
	strcpy(m_old_text, m_text);
	m_text_len = txt.length();
}

double NkEditValue::get_value_flt()
{
	m_text[m_text_len] = 0;
	double v;
	sscanf(m_text, "%d", &v);
	return v;
}

long NkEditValue::get_value_int()
{
	m_text[m_text_len] = 0;
	long v;
	sscanf(m_text, "%i", &v);
	return v;
}

std::string NkEditValue::get_value_str()
{
	m_text[m_text_len] = 0;
	return std::string(m_text);
}

NkPropertyInt::NkPropertyInt(std::string label, int min, int max, int default_value, int step) : m_min(min), m_max(max),
		m_value(default_value), m_step(step), m_label(label)
{
}

NkPropertyInt::~NkPropertyInt() {
}

void
NkPropertyInt::draw(nk_context* ctx)
{
	push_text_attributes(ctx);
	nk_property_int(ctx, m_label.c_str(), m_min, &m_value, m_max, 1, 1);
	pop_text_attributes(ctx);
}

NkPropertyFloat::NkPropertyFloat(std::string label, float min, float max, float default_value, int step) : m_min(min), m_max(max),
		m_value(default_value), m_step(step), m_label(label)
{
}

NkPropertyFloat::~NkPropertyFloat() {
}

void
NkPropertyFloat::draw(nk_context* ctx)
{
	push_text_attributes(ctx);
	nk_property_float(ctx, m_label.c_str(), m_min, &m_value, m_max, 1, 1);
	pop_text_attributes(ctx);
}

NkChartSlot::NkChartSlot(unsigned int size, double min, double max, NkColor color,
		NkColor color_hi) : m_color(color), m_color_hi(color_hi), m_size(size), m_min(min), m_max(max)
{
	m_buffer = (double*)malloc(m_size*sizeof(double));
	memset((void*)m_buffer, 0,m_size*sizeof(double));
	m_clicked.reserve(10);
	m_hovered.reserve(10);
}

NkChartSlot::~NkChartSlot()
{
	free(m_buffer);
}

NkChart::NkChart(Chart_type type, unsigned int size, double min, double max) : m_type(type),
		m_size(size), m_color(NkColor(128, 128, 128, 255)), m_color_hi(NkColor(255, 255, 255, 255)),
		m_minval(min), m_maxval(max)
{
	add_slot(min, max, m_color, m_color_hi);
}

NkChart::~NkChart()
{
}

void
NkChart::draw(nk_context* ctx)
{
	if (nk_chart_begin_colored(ctx, (nk_chart_type)m_type, *m_color.get_nk_color(), *m_color_hi.get_nk_color(), m_size, m_minval, m_maxval)){
		for(int i = 1; i < m_slots.size(); ++i){
			NkChartSlot* slot = m_slots[i];
			nk_chart_add_slot_colored(ctx, (nk_chart_type)m_type, *slot->m_color.get_nk_color(), *slot->m_color_hi.get_nk_color(), slot->m_size, slot->m_min, slot->m_max);
		}
		for(int i = 0; i < m_slots.size(); ++i){
			NkChartSlot* slot = m_slots[i];
			slot->m_clicked.clear();
			slot->m_hovered.clear();
			const double *buffer = slot->get_buffer();
			for (int id = 0; id < m_size; ++id) {
				nk_flags flags = nk_chart_push_slot(ctx, buffer[id], i);
				if (flags & NK_CHART_CLICKED){
					slot->m_clicked.push_back(id);
				}
				if (flags & NK_CHART_HOVERING){
					slot->m_hovered.push_back(id);
				}
			}
		}
	}
	nk_chart_end(ctx);
}

NkChartSlot*
NkChart::add_slot(double min, double max, NkColor color, NkColor color_hi)
{
	NkChartSlot* slot = new NkChartSlot(m_size, min, max, color, color_hi);
	m_slots.push_back(slot);
	return slot;
}

NkColorPicker::NkColorPicker(NkColor col) :  m_color_mode(COL_RGB), m_color(col)
{
	set_color(col);
}

NkColorPicker::~NkColorPicker()
{
}

void
NkColorPicker::draw(nk_context* ctx)
{
	if (nk_combo_begin_color(ctx, nk_rgb_cf(*m_colorf.get_nk_colorf()), nk_vec2(200,500))) {
		NkColor old_color = m_color;
		nk_layout_row_dynamic(ctx, 120, 1);
		struct nk_colorf colf = nk_color_picker(ctx, *m_colorf.get_nk_colorf(), NK_RGBA);
		m_colorf.r = colf.r;
		m_colorf.g = colf.g;
		m_colorf.b = colf.b;
		m_colorf.a = colf.a;

		nk_layout_row_dynamic(ctx, 25, 2);
		m_color_mode = nk_option_label(ctx, "RGB", m_color_mode == COL_RGB) ? COL_RGB : m_color_mode;
		m_color_mode = nk_option_label(ctx, "HSV", m_color_mode == COL_HSV) ? COL_HSV : m_color_mode;

		nk_layout_row_dynamic(ctx, 25, 1);
		if (m_color_mode == COL_RGB) {
			m_colorf.r = nk_propertyf(ctx, "#R:", 0, m_colorf.r, 1.0f, 0.01f,0.005f);
			m_colorf.g = nk_propertyf(ctx, "#G:", 0, m_colorf.g, 1.0f, 0.01f,0.005f);
			m_colorf.b = nk_propertyf(ctx, "#B:", 0, m_colorf.b, 1.0f, 0.01f,0.005f);
			m_colorf.a = nk_propertyf(ctx, "#A:", 0, m_colorf.a, 1.0f, 0.01f,0.005f);
		} else {
			float hsva[4];
			nk_colorf_hsva_fv(hsva, *m_colorf.get_nk_colorf());
			hsva[0] = nk_propertyf(ctx, "#H:", 0, hsva[0], 1.0f, 0.01f,0.05f);
			hsva[1] = nk_propertyf(ctx, "#S:", 0, hsva[1], 1.0f, 0.01f,0.05f);
			hsva[2] = nk_propertyf(ctx, "#V:", 0, hsva[2], 1.0f, 0.01f,0.05f);
			hsva[3] = nk_propertyf(ctx, "#A:", 0, hsva[3], 1.0f, 0.01f,0.05f);
			colf = nk_hsva_colorfv(hsva);
			m_colorf.r = colf.r;
			m_colorf.g = colf.g;
			m_colorf.b = colf.b;
			m_colorf.a = colf.a;
		}
		nk_combo_end(ctx);
		m_color = m_colorf.get_color_bytes();

		if ((m_color != old_color) && m_callback){
			m_callback(this, &m_color, m_callback_data);
		}
	}
}

void
NkColorPicker::set_color(NkColor col)
{
	m_colorf.r = col.r / 255.0f;
	m_colorf.g = col.g / 255.0f;
	m_colorf.b = col.b / 255.0f;
	m_colorf.a = col.a / 255.0f;
	m_color = col;
}

