#ifndef WINDOW_BASE_H
#define WINDOW_BASE_H

struct win_impl;
struct nk_context;
struct UserEvent;
struct NkWidget;
class  Socket_notifier;
class  Timer;
struct nk_font;

#include <string>
#include <vector>
#include <map>

typedef unsigned int GLuint;

struct NkTexture{
	enum TextureDataTye {
		TYPE_FLOAT,
		TYPE_UCHAR
	};

	enum InternalFormat {
		RGB_BYTE,
		RGBA_BYTE,
		RGB_FLOAT,
		RGBA_FLOAT,
		RGB_HALF,
		RGBA_HALF
	};

	void *data;
	TextureDataTye pixel_data_type;
	int width, height;
	InternalFormat internal_format;
	NkTexture(){
	}
	NkTexture(void* img_data, TextureDataTye data_type, int img_width, int img_height, InternalFormat img_internal_format){
		data = img_data;
		pixel_data_type = data_type;
		width = img_width;
		height = img_height;
		internal_format = img_internal_format;
	}
};

class NkWindow {
public:
	enum StyleTheme
	{
		THEME_BLACK,
		THEME_WHITE,
		THEME_RED,
		THEME_BLUE,
		THEME_DARK
	};
	NkWindow(int w, int h, bool aa);
	virtual ~NkWindow();
	virtual void shutdown() = 0;
	virtual void run() = 0;
	void add_widget(NkWidget* w);
	void set_stye(StyleTheme t);
	void cleanup();

	static NkWindow* get();

	void register_fd_event(Socket_notifier* sn);
	void unregister_fd_event(Socket_notifier* sn);
	void register_user_event(UserEvent* ev);
	void unregister_user_event(UserEvent* ev);
	void register_timer(Timer* t);
	void unregister_timer(Timer* t);

	int  get_width();
	int  get_height();
	unsigned long timestamp(void);

	void load_font(std::string font_file, std::string font_name, int size);
	void use_font(std::string font_name);
	void add_font(std::string font_name, std::string filename, float size);
	struct nk_font* get_user_font(std::string font_name);

	virtual void delete_texture(GLuint id) = 0;
	virtual GLuint create_texture(const NkTexture& texture) = 0;
	void load_fonts();
protected:
	struct nk_draw_null_texture* get_draw_null_texture();
	GLuint device_upload_atlas(void *image, int width, int height);
	void handle_fd_events();
	void handle_timer_events();
	nk_context*  get_ctx();
	std::vector<NkWidget*> get_widgets();
	std::vector<UserEvent*>& get_user_events();
	bool is_running();
	void set_running(bool);
	bool m_anti_aliasing;
private:
	win_impl* m_impl;
};

#endif
