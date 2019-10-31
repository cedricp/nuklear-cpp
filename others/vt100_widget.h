#ifndef VT100_EMU
#define VT100_EMU
#include <nuklear_lib.h>

class NkVt100FramebufferDisplay : public Tinytty_display
{
	NkImage m_framebuffer;
public:
	NkVt100FramebufferDisplay() : Tinytty_display(150,200, 4, 6), m_framebuffer(150,200){

	}

	void draw(nk_context* ctx){
		m_framebuffer.draw(ctx);
	}

    virtual void fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color);
    virtual void draw_pixels(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t *pixels);
    virtual void set_vscroll(int16_t offset){

    }
    virtual void draw_character(char c, unsigned char char_set, uint32_t bg_col, uint32_t fg_col, int x, int y);
};

class NkVT100 : public NkBase
{
private:
	Tiny_tty m_tty;
	NkVt100FramebufferDisplay m_tty_diplay;
protected:
	virtual void draw(nk_context* ctx){
		m_tty.process();
		m_tty_diplay.draw(ctx);
	}
public:
	NkVT100(std::string name) : m_tty(&m_tty_diplay){
		m_tty.init();
	}
};

#endif
