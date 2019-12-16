#ifndef GLX_WINDOW_H
#define GLX_WINDOW_H

#define DFT_MAX_VERTEX_MEMORY 512 * 1024
#define DFT_MAX_ELEMENT_MEMORY 128 * 1024

#include <window.h>

typedef unsigned int GLuint;

struct glx_impl;
class UserEvent;

class NkWindowGLX : public NkWindow {
public:
	virtual ~NkWindowGLX();
	void run();
	static NkWindow* create(int w, int h, bool aa = true);

	virtual void delete_texture(GLuint id) override;
	virtual GLuint create_texture(const NkTexture& texture) override;

	void push_user_event(UserEvent* ev);
private:
	NkWindowGLX(int w, int h,
			unsigned long max_vert_mem = DFT_MAX_VERTEX_MEMORY,
			unsigned long max_elem_me = DFT_MAX_ELEMENT_MEMORY,
			bool aa = true);

	bool handle_events();
	bool handle_event();

	void render();
	void do_ui();

	bool init_GLX();
	bool init_GL_functions();
	bool init_GLSL(int , int);
	void shutdown();

	glx_impl* m_glx_impl;
};

#endif
