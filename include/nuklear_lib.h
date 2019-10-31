#ifndef NUKLEARLIB_H
#define NUKLEARLIB_H

#include <window.h>
#include <string>
#include <vector>
#include <map>

struct nk_context;
struct nk_image;
struct nk_font;
struct nk_command_buffer;
class NkEvent;
class NkWindow;
struct cavas_impl;
struct image_impl;

typedef void (*event_cb_t)(NkEvent*, void*, void*);

#define STATIC_CALLBACK_METHOD(methname, ClassType) \
	static void static_method_##methname(NkEvent* base, void* data, void* class_instance) \
	{ \
		((ClassType *)class_instance)->methname(base, data); \
	}

#define DECLARE_STATIC_CALLBACK_METHOD(methname) \
	static void static_method_##methname(NkEvent* base, void* data, void* class_instance);

#define IMPLEMENT_STATIC_CALLBACK_METHOD(methname, ClassType) \
	void ClassType::static_method_##methname(NkEvent* base, void* data, void* class_instance) \
	{ \
		((ClassType *)class_instance)->methname(base, data); \
	}
#define DECLARE_CALLBACK_METHOD(methname) void methname(NkEvent* sender_object, void* data);
#define DECLARE_VIRTUAL_CALLBACK_METHOD(methname) virtual void methname(NkEvent* sender_object, void* data);
#define DECLARE_METHODS(methname) DECLARE_STATIC_CALLBACK_METHOD(methname) DECLARE_CALLBACK_METHOD(methname)

#define IMPLEMENT_CALLBACK_METHOD(methname, classname) IMPLEMENT_STATIC_CALLBACK_METHOD(methname, classname) void classname::methname(NkEvent* sender_object, void* data)

/*
 * Callback method macro
 */
#define CALLBACK_METHOD(methname) void methname(NkEvent* sender_object, void* data)


/*
 * Callback connection macros
 */
#define CONNECT_CALLBACK(base, methname) base->set_callback(static_method_##methname, (void*)this);
#define CONNECT_CALLBACK2(base, methname, class_instance) base->set_callback(static_method_##methname, (void*)class_instance);
#define RESET_CALLBACK(base) base->set_callback(NULL, NULL);

struct NkColor {
	NkColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a){
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = a;
	}
	NkColor(){
		this->r = 0;
		this->g = 0;
		this->b = 0;
		this->a = 0;
	}
	unsigned char r, g, b, a;

	struct nk_color* get_nk_color(){
		return (struct nk_color*)this;
	}

	bool operator != (const NkColor&o){
		if (o.r != r)
			return true;
		if (o.g != g)
			return true;
		if (o.b != b)
			return true;
		if (o.a != a)
			return true;
		return false;
	}
};

struct NkColorf {
	NkColorf(float r, float g, float b, float a){
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = a;
	}
	NkColorf(){
		this->r = 0;
		this->g = 0;
		this->b = 0;
		this->a = 0;
	}

	float r, g, b, a;

	struct nk_colorf* get_nk_colorf(){
		return (struct nk_colorf*)this;
	}

	NkColor get_color_bytes(){
		return NkColor((r + 0.5) *255., (g + 0.5) *255., (b + 0.5) *255., (a + 0.5) *255.);
	}
};

#define NKCOLOR_RED   NkColor(255, 0, 0, 255)
#define NKCOLOR_GREEN NkColor(0, 255, 0, 255)
#define NKCOLOR_BLUE  NkColor(0, 0, 255, 255)
#define NKCOLOR_BLACK NkColor(0, 0, 0, 255)
#define NKCOLOR_WHITE NkColor(255, 255, 255, 255)

struct NkRect
{
	NkRect(){
		x = y = w = h = 0.;
	}
	NkRect(int x, int y, int w, int h){
		this->x = x;
		this->y = y;
		this->w = w;
		this->h = h;
	}
	float x,y,w,h;
};

struct NkVec2 {
	NkVec2(){
		x = y = 0;
	}
	NkVec2(float x, float y){
		this->x = x;
		this->y = y;
	}
	float x, y;
};

enum Text_align {
    TEXT_ALIGN_LEFT        = 0x01,
    TEXT_ALIGN_CENTERED    = 0x02,
    TEXT_ALIGN_RIGHT       = 0x04,
    TEXT_ALIGN_TOP         = 0x08,
    TEXT_ALIGN_MIDDLE      = 0x10,
    TEXT_ALIGN_BOTTOM      = 0x20
};


enum Text_aligment {
    TEXT_LEFT        = TEXT_ALIGN_MIDDLE|TEXT_ALIGN_LEFT,
    TEXT_CENTERED    = TEXT_ALIGN_MIDDLE|TEXT_ALIGN_CENTERED,
    TEXT_RIGHT       = TEXT_ALIGN_MIDDLE|TEXT_ALIGN_RIGHT
};


class NkEvent {
protected:
	event_cb_t m_callback;
	void *m_callback_data, *m_userdata1, *m_userdata2;
public:
	NkEvent(){
		m_callback = NULL;
		m_callback_data = m_userdata1 = m_userdata2 = NULL;
	}

	void set_callback(event_cb_t cb, void* callback_data){
		m_callback = cb;
		m_callback_data = callback_data;
	}

	void* get_data1(){
		return m_userdata1;
	}

	void* get_data2(){
		return m_userdata2;
	}
};

class UserEvent : public NkEvent
{
	int m_event_idx;
public:
	UserEvent();
	~UserEvent();

	void push(int code = 0, void* data1 = 0L, void* data2 = 0L);
	void on_callback(void* data1 = NULL, void* data2 = NULL);
	friend class NkWindowGLES2;
	friend class NkWindowGLX;
};

class NkBase : public NkEvent{
protected:
	NkWindow* m_parent_window;
	NkBase* m_parent;
	int m_row_width;
	int m_row_height;
	bool m_autolayout;
	std::string m_name;
	struct nk_font* m_font;
	struct bound {
		float x,y, w, h;
	} m_bound;

	void push_text_attributes(struct nk_context* ctx);
	void pop_text_attributes(struct nk_context* ctx);
public:
	NkBase(NkBase* parent = NULL){
		m_parent_window = NULL;
		m_parent = NULL;
		m_row_width = 0;
		m_row_height = 0;
		m_font = NULL;
		m_autolayout = false;
	}
	virtual ~NkBase(){}
	void set_parent_window(NkWindow* parent){
		m_parent_window = parent;
	}
	void set_parent(NkBase* parent){
		m_parent = parent;
	}
	virtual void draw(nk_context* ctx){}

	NkBase* get_parent(){
		return m_parent;
	}
	NkWindow* get_parent_window(){
		return m_parent_window;
	}
	void set_row_width(int w){
		m_row_width = w;
	}
	int get_row_width(){
		return m_row_width;
	}
	void set_row_height(int h){
		m_row_height = h;
	}
	int get_row_height(){
		return m_row_height;
	}
	void set_layout_size(int w, int h){
		m_row_width = w;
		m_row_height = h;
	}
	struct bound get_bound(){
		return m_bound;
	}
	void set_text_font(std::string font);
	void set_autolayout(bool a){
		m_autolayout = a;
	}
	friend class NkLayoutRow;
	friend class NkLayoutRowDynamic;
};

class NkWidget : public NkBase {
	std::vector<NkBase*> m_elements;
protected:
	int m_x,m_y, m_width, m_height;
	int m_flags;
	bool is_hovered;
	virtual void draw(nk_context* ctx);
public:
	NkWidget(std::string name, int posx, int posy, int w, int h);
	virtual ~NkWidget();
	void add_element(NkBase* elem);
	void set_scalable(bool);
	void set_movable(bool);
	void set_borders(bool);
	void set_closable(bool);
	void set_title_on(bool);
	void set_minimizable(bool);
	void set_title_name(std::string name);

	friend class NkWindowGLES2;
	friend class NkWindowGLX;
};


class NkLabel : public NkBase {
public:
	NkLabel(std::string text, Text_aligment align);
	virtual void draw(nk_context* ctx);
private:
	std::string m_text;
	int m_text_alignment;
};


class NkButton : public NkBase {
public:
	enum Symbol{
		NONE,
		X,
		UNDERSCORE,
		CIRCLE_SOLID,
		CIRCLE_OUTLINE,
		RECT_SOLID,
		RECT_OUTLINE,
		TRIANGLE_UP,
		TRIANGLE_DOWN,
		TRIANGLE_LEFT,
		TRIANGLE_RIGHT,
		PLUS,
		MINUS,
	};
	NkButton(std::string name, Symbol symbol = Symbol::NONE);
	virtual void draw(nk_context* ctx);
private:
	std::string m_name;
	Symbol m_symbol;
};



class NkCheckbox : public NkBase {
protected:
	virtual void draw(nk_context* ctx);
private:
	int m_checked;
public:
	NkCheckbox(std::string name);
	void set_checked(bool c){
		m_checked = c;
	}
	int get_checked(){
		return m_checked;
	}
};

class NkCheckList : public NkBase {
protected:
	virtual void draw(nk_context* ctx);
private:
	int m_checkedval;
	std::vector<std::string> m_names;
public:
	NkCheckList(std::vector<std::string> names);

	int get_checked_value(){
		return m_checkedval;
	}

	std::string get_checked_name(){
		return m_names[m_checkedval];
	}
};

class NkCombo : public NkBase {
protected:
	virtual void draw(nk_context* ctx);
private:
	std::vector<std::string> m_elems;
	int m_selected;
public:
	void add_element(std::string elem_name);
	NkCombo(std::string name);

	int get_selected_index(){
		return m_selected;
	}

	std::string get_selected_string(){
		return m_elems[m_selected];
	}
};

class NkImage : public NkBase {
protected:
	void  generate_texture();
	virtual void draw(nk_context* ctx);
	image_impl* m_impl;
public:
	NkImage(std::string filename);
	NkImage(int size_x, int size_y);
	NkImage(const NkTexture& texture);
	~NkImage();

	struct nk_image* get_image();
	void* get_data();
	void update_texture();
	int get_width();
	int get_height();
};


class NkComboImage : public NkBase {
private:
	std::vector<NkImage*> m_elems;
	std::vector<std::string> m_names;
	int m_selected, m_height;
protected:
	virtual void draw(nk_context* ctx);
public:
	void add_element(NkImage* elem, std::string name);
	NkComboImage(std::string name, int default_height);

	int get_selected_index(){
		return m_selected;
	}

	NkImage* get_selected_image(){
		return m_elems[m_selected];
	}
};


class NkSliderFloat : public NkBase
{
private:
	float m_value, m_min, m_max, m_step;
protected:
	virtual void draw(nk_context* ctx);
public:
	NkSliderFloat(std::string name, float min, float max, float val, float step = 1.0);

};

class NkSliderInt : public NkBase
{
private:
	int m_value, m_min, m_max, m_step;
protected:
	virtual void draw(nk_context* ctx);
public:
	NkSliderInt(std::string name, int min, int max, int val, int step = 1);

};

class NkProgress : public NkBase
{
private:
	unsigned long m_value, m_max;
	bool m_modifiable;
protected:
	virtual void draw(nk_context* ctx);
public:
	NkProgress(std::string name, int max, int val, bool modifiable);
	void set_value(int val, bool do_callback = false);
};

class NkPropertyInt : public NkBase {
	std::string m_label;
	int m_value;
	int m_min, m_max, m_step;
protected:
	virtual void draw(nk_context* ctx);
public:
	NkPropertyInt(std::string label, int min, int max, int default_value, int step = 1);
	~NkPropertyInt();
};

class NkPropertyFloat : public NkBase {
	std::string m_label;
	float m_value;
	float m_min, m_max, m_step;
protected:
	virtual void draw(nk_context* ctx);
public:
	NkPropertyFloat(std::string label, float min, float max, float default_value, int step = 1);
	~NkPropertyFloat();
};

class NkEditValue : public NkBase {
protected:
	virtual void draw(nk_context* ctx);
public:
	enum Text_filter {
		FILTER_NONE,
		FILTER_ASCII,
		FILTER_FLT,
		FILTER_INT,
		FILTER_OCT,
		FILTER_HEX,
		FILTER_BIN
	} ;
	NkEditValue(Text_filter filter, size_t buffer_size=64);
	~NkEditValue();

	void set_value(double val);
	void set_value(long val);
	void set_value(std::string txt);
	double get_value_flt();
	long get_value_int();
	std::string get_value_str();

	void set_multiline(bool m);
	void set_readonly(bool r);
private:
	Text_filter m_filter;
    char *m_text;
    char *m_old_text;
    int m_text_len;
    int m_old_state;
    int m_flags;
    int m_buffer_size;
};

class NkLayoutRow :  public NkBase {
protected:
	virtual void draw(nk_context* ctx);
	std::vector<NkBase*> m_elements;
	int m_height, m_item_width, m_num_columns;
	int m_num_rows;
	bool m_is_row_dynamic;
public:
	NkLayoutRow(int height, int item_width, int cols, bool dynamic = false);
	virtual ~NkLayoutRow();
	void add_element(NkBase* elem);
};



class NkLayoutRowDynamic :  public NkLayoutRow {
public:
	NkLayoutRowDynamic(int height, int col_num, bool dynamic = false);
};



class NkTree : public NkBase {
protected:
	virtual void draw(nk_context* ctx);
private:
	std::vector<NkBase*> m_elements;
	std::vector<std::string> m_names;
	std::vector<int> m_minimized;
	bool m_node_type;
public:
	NkTree(bool node_type = false);
	~NkTree();
	void add_element(NkBase* elem, std::string name, bool minimized = true);
};

class NkPainter {
	nk_command_buffer* m_painter;
	nk_context* m_ctx;
	NkRect m_bounds;
public:
	NkPainter();
	void set_context(nk_context* ctx);
	void set_bounds(const NkRect& bounds){
		m_bounds = bounds;
	}

	void draw_fill_polygon(std::vector<NkVec2>& points, int width, const NkColor& colors);
	void draw_fill_polygon(std::vector<NkVec2>& points, int width, float thickness, const NkColor& colors);
	void draw_image(NkImage& img, NkRect rect, NkColor color);
	void draw_text(std::string text, NkRect r, NkColor color);
	void draw_stroke_line(NkVec2 a, NkVec2 b, float thickness, NkColor& color);
	void draw_fill_rect(NkRect rect, float rounding, NkColor& color);
};


class NkCanvasWindow : public NkWidget {
protected:
	cavas_impl* m_impl;
	NkPainter m_painter;
	virtual void draw(nk_context* ctx);
public:
	NkCanvasWindow(std::string name, int x, int y, int width, int height);
	~NkCanvasWindow();
	NkPainter* get_painter(){
		return &m_painter;
	}

	virtual void draw_content();
};

class NkCanvas : public NkBase {
protected:
	NkPainter m_painter;
	virtual void draw(nk_context* ctx);
public:
	NkCanvas(std::string name);
	~NkCanvas();
	NkPainter* get_painter(){
		return &m_painter;
	}

	virtual void draw_content();
};

class NkChartSlot {
	double* m_buffer;
	double m_min, m_max;
	unsigned long m_size;
	NkColor m_color, m_color_hi;
	std::vector<unsigned int> m_clicked;
	std::vector<unsigned int> m_hovered;
public:
	NkChartSlot(unsigned int size, double min, double max, NkColor color, NkColor color_hi);
	~NkChartSlot();
	double* get_buffer(){return m_buffer;}
	unsigned int get_buffer_len(){return m_size;}

	std::vector<unsigned int>& get_clicked_indexes(){return m_clicked;}
	std::vector<unsigned int>& get_hovered_indexes(){return m_hovered;}

	friend class NkChart;
};

class NkChart : public NkBase {
protected:
	virtual void draw(nk_context* ctx);
public:
	enum Chart_type {
		CHART_LINES,
		CHART_COLUMN,
		CHART_MAX
	};
	NkChart(Chart_type type, unsigned int size, double min = 0.f, double max = 100.f);
	~NkChart();

	NkChartSlot* add_slot(double min, double max, NkColor color, NkColor color_hi);
	unsigned long get_slots_size(){return m_slots.size();}
	NkChartSlot* get_slot(unsigned long num){return m_slots[num];}
private:
	Chart_type m_type;
	std::vector<NkChartSlot*> m_slots;
	float m_minval, m_maxval;
	unsigned long m_size;
	NkColor m_color, m_color_hi;
};

class NkColorPicker : public NkBase {
	NkColorf m_colorf;
	NkColor m_color;
	int m_color_mode;
protected:
	virtual void draw(nk_context* ctx);
public:
	enum color_mode {COL_RGB, COL_HSV};
	NkColorPicker(NkColor col = NKCOLOR_WHITE);
	~NkColorPicker();

	void set_color(NkColor col);

	const NkColorf& get_colorf() const {return m_colorf;}
	const NkColor& get_color() const {return m_color;}
};

#endif
