#ifndef SDLGLES2UTILS__
#define SDLGLES2UTILS__

#define DFT_MAX_VERTEX_MEMORY 512 * 1024
#define DFT_MAX_ELEMENT_MEMORY 128 * 1024

#include <window.h>

typedef unsigned int GLuint;

struct sdl_impl;

class NkWindowGLES2 : public NkWindow {
public:
	virtual ~NkWindowGLES2();
	void run();
	static NkWindow* create(int w, int h, bool aa = true);

	void delete_texture(GLuint id);
	GLuint create_texture(const NkTexture& texture);
private:
	NkWindowGLES2(int w, int h,
			unsigned long max_vert_mem = DFT_MAX_VERTEX_MEMORY,
			unsigned long max_elem_me = DFT_MAX_ELEMENT_MEMORY,
			bool aa = true);


	bool handle_events();
	bool handle_event();

	void render();
	void do_ui();

	bool init_GL();
	bool init_SDL();
	void shutdown();

	sdl_impl* m_sdl_impl;
};

#endif
