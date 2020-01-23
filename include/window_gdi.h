#ifndef GDI2UTILS__
#define GDI2UTILS__

#include <window.h>

struct gdi_impl;
struct gdi_font;

class NkWindowGDI : public NkWindow {
public:
	virtual ~NkWindowGDI();
	void run();
	static NkWindow* create(int w, int h, bool aa = true);

	void delete_texture(unsigned int id);
	unsigned int create_texture(const NkTexture& texture);
	gdi_impl* get_gdi_impl(){
		return m_gdi_impl;
	}
	void push_user_event(UserEvent* ev);
	virtual void load_fonts();
	virtual void use_font(std::string font_name);
	virtual void add_font(std::string font_name, std::string filename, float size);
	virtual struct nk_font* get_user_font(std::string font_name);
private:
	NkWindowGDI(int w, int h, bool aa = true);

	gdi_font* gdi_font_create(const char* name, int size);
	bool handle_events();

	void render();
	void do_ui();

	bool init_GDI(int width, int height);
	void shutdown();

	gdi_impl* m_gdi_impl;
	bool m_needs_refresh;
};

#endif
