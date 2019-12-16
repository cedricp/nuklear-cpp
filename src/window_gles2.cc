#include <nuklear_lib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <unistd.h>
#include "window_gles2.h"
#define GLES2
#include "window_gl_common.h"
#undef GLES2

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

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) < (b) ? (b) : (a))
#define LEN(a) (sizeof(a)/sizeof(a)[0])

extern NkWindow* global_ptr;

struct nk_sdl_device {
    struct nk_buffer cmds;
    GLuint vbo, ebo;
    GLuint prog;
    GLuint vert_shdr;
    GLuint frag_shdr;
    GLint attrib_pos;
    GLint attrib_uv;
    GLint attrib_col;
    GLint uniform_tex;
    GLint uniform_proj;
    GLsizei vs;
    size_t vp, vt, vc;
    size_t max_vertex_memory;
    size_t max_element_memory;
    void *vertices_mem, *elements_mem;
};

struct nk_sdl_vertex {
    GLfloat position[2];
    GLfloat uv[2];
    nk_byte col[4];
};

struct sdl_impl {
	nk_sdl_device ogl;
    SDL_Window *win;
    SDL_Event evt;
};

#define NK_SHADER_VERSION "#version 100\n"


static void
nk_sdl_clipboard_paste(nk_handle usr, struct nk_text_edit *edit)
{
    const char *text = SDL_GetClipboardText();
    if (text) nk_textedit_paste(edit, text, nk_strlen(text));
    (void)usr;
}

static void
nk_sdl_clipboard_copy(nk_handle usr, const char *text, int len)
{
    char *str = 0;
    (void)usr;
    if (!len) return;
    str = (char*)malloc((size_t)len+1);
    if (!str) return;
    memcpy(str, text, (size_t)len);
    str[len] = '\0';
    SDL_SetClipboardText(str);
    free(str);
}

static const GLchar *vertex_shader =
    NK_SHADER_VERSION
    "uniform mat4 ProjMtx;\n"
    "attribute vec2 Position;\n"
    "attribute vec2 TexCoord;\n"
    "attribute vec4 Color;\n"
    "varying vec2 Frag_UV;\n"
    "varying vec4 Frag_Color;\n"
    "void main() {\n"
    "   Frag_UV = TexCoord;\n"
    "   Frag_Color = Color;\n"
    "   gl_Position = ProjMtx * vec4(Position.xy, 0, 1);\n"
    "}\n";
static const GLchar *fragment_shader =
    NK_SHADER_VERSION
    "precision mediump float;\n"
    "uniform sampler2D Texture;\n"
    "varying vec2 Frag_UV;\n"
    "varying vec4 Frag_Color;\n"
    "void main(){\n"
    "   gl_FragColor = Frag_Color * texture2D(Texture, Frag_UV);\n"
    "}\n";

NkWindowGLES2::NkWindowGLES2(int width, int height, size_t max_vert_mem, size_t max_elem_mem, bool aa) : NkWindow(width, height, aa)
{
	m_sdl_impl = new sdl_impl;
	m_sdl_impl->ogl.max_vertex_memory  = max_vert_mem;
	m_sdl_impl->ogl.max_element_memory = max_elem_mem;

    m_sdl_impl->ogl.vertices_mem = malloc((size_t)m_sdl_impl->ogl.max_vertex_memory);
    m_sdl_impl->ogl.elements_mem = malloc((size_t)m_sdl_impl->ogl.max_element_memory);

    get_ctx()->clip.copy = nk_sdl_clipboard_copy;
    get_ctx()->clip.paste = nk_sdl_clipboard_paste;
    get_ctx()->clip.userdata = nk_handle_ptr(0);

    init_SDL();
    init_GL();
}

NkWindowGLES2::~NkWindowGLES2()
{
	cleanup();
	shutdown();

	free(m_sdl_impl->ogl.vertices_mem);
	free(m_sdl_impl->ogl.elements_mem);
}

bool NkWindowGLES2::init_GL() {
    /* OpenGL setup */
    GLint status;
    struct nk_sdl_device *dev = &m_sdl_impl->ogl;

    nk_buffer_init_default(&dev->cmds);
    dev->prog = glCreateProgram();
    dev->vert_shdr = glCreateShader(GL_VERTEX_SHADER);
    dev->frag_shdr = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(dev->vert_shdr, 1, &vertex_shader, 0);
    glShaderSource(dev->frag_shdr, 1, &fragment_shader, 0);
    glCompileShader(dev->vert_shdr);
    glCompileShader(dev->frag_shdr);
    glGetShaderiv(dev->vert_shdr, GL_COMPILE_STATUS, &status);
    assert(status == GL_TRUE);
    glGetShaderiv(dev->frag_shdr, GL_COMPILE_STATUS, &status);
    assert(status == GL_TRUE);
    glAttachShader(dev->prog, dev->vert_shdr);
    glAttachShader(dev->prog, dev->frag_shdr);
    glLinkProgram(dev->prog);
    glGetProgramiv(dev->prog, GL_LINK_STATUS, &status);
    assert(status == GL_TRUE);


    dev->uniform_tex = glGetUniformLocation(dev->prog, "Texture");
    dev->uniform_proj = glGetUniformLocation(dev->prog, "ProjMtx");
    dev->attrib_pos = glGetAttribLocation(dev->prog, "Position");
    dev->attrib_uv = glGetAttribLocation(dev->prog, "TexCoord");
    dev->attrib_col = glGetAttribLocation(dev->prog, "Color");
    {
        dev->vs = sizeof(struct nk_sdl_vertex);
        dev->vp = offsetof(struct nk_sdl_vertex, position);
        dev->vt = offsetof(struct nk_sdl_vertex, uv);
        dev->vc = offsetof(struct nk_sdl_vertex, col);

        /* Allocate buffers */
        glGenBuffers(1, &dev->vbo);
        glGenBuffers(1, &dev->ebo);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    return true;
}

bool NkWindowGLES2::init_SDL() {
    SDL_GLContext glContext;
    /* SDL setup */
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    SDL_Init(SDL_INIT_EVENTS);
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    m_sdl_impl->win = SDL_CreateWindow("Demo",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		get_width(), get_height(), SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN|SDL_WINDOW_ALLOW_HIGHDPI);
    glContext = SDL_GL_CreateContext(m_sdl_impl->win);
    return true;
}

NkWindow*
NkWindowGLES2::create(int w, int h, bool aa)
{
	if (global_ptr != NULL)
		return global_ptr;

	global_ptr = (NkWindow*)new NkWindowGLES2(w, h, DFT_MAX_VERTEX_MEMORY, DFT_MAX_ELEMENT_MEMORY, aa);
	return global_ptr;
}

void
NkWindowGLES2::render()
{
    struct nk_sdl_device *dev = &m_sdl_impl->ogl;
    int width, height;
    int display_width, display_height;
    struct nk_vec2 scale;
    GLfloat ortho[4][4] = {
        {2.0f, 0.0f, 0.0f, 0.0f},
        {0.0f,-2.0f, 0.0f, 0.0f},
        {0.0f, 0.0f,-1.0f, 0.0f},
        {-1.0f,1.0f, 0.0f, 1.0f},
    };

    float bg[4];
    int win_width, win_height;
    nk_color_fv(bg, nk_rgb(28,48,62));
    SDL_GetWindowSize(m_sdl_impl->win, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(bg[0], bg[1], bg[2], bg[3]);
    glClear(GL_COLOR_BUFFER_BIT);

    SDL_GL_GetDrawableSize(m_sdl_impl->win, &display_width, &display_height);
    ortho[0][0] /= (GLfloat)width;
    ortho[1][1] /= (GLfloat)height;

    scale.x = (float)display_width/(float)width;
    scale.y = (float)display_height/(float)height;

    /* setup global state */
    glViewport(0,0,display_width,display_height);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);

    /* setup program */
    glUseProgram(dev->prog);
    glUniform1i(dev->uniform_tex, 0);
    glUniformMatrix4fv(dev->uniform_proj, 1, GL_FALSE, &ortho[0][0]);
    {
        /* convert from command queue into draw list and draw to screen */
        const struct nk_draw_command *cmd;
        const nk_draw_index *offset = NULL;

        /* Bind buffers */
        glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

		/* buffer setup */
		glEnableVertexAttribArray((GLuint)dev->attrib_pos);
		glEnableVertexAttribArray((GLuint)dev->attrib_uv);
		glEnableVertexAttribArray((GLuint)dev->attrib_col);

		glVertexAttribPointer((GLuint)dev->attrib_pos, 2, GL_FLOAT, GL_FALSE, dev->vs, (void*)dev->vp);
		glVertexAttribPointer((GLuint)dev->attrib_uv, 2, GL_FLOAT, GL_FALSE, dev->vs, (void*)dev->vt);
		glVertexAttribPointer((GLuint)dev->attrib_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, dev->vs, (void*)dev->vc);

        glBufferData(GL_ARRAY_BUFFER, m_sdl_impl->ogl.max_vertex_memory, NULL, GL_STREAM_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_sdl_impl->ogl.max_element_memory, NULL, GL_STREAM_DRAW);

        /* fill convert configuration */
        struct nk_convert_config config;
        static const struct nk_draw_vertex_layout_element vertex_layout[] = {
            {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_sdl_vertex, position)},
            {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_sdl_vertex, uv)},
            {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct nk_sdl_vertex, col)},
            {NK_VERTEX_LAYOUT_END}
        };
        NK_MEMSET(&config, 0, sizeof(config));
        config.vertex_layout = vertex_layout;
        config.vertex_size = sizeof(struct nk_sdl_vertex);
        config.vertex_alignment = NK_ALIGNOF(struct nk_sdl_vertex);
        config.null = *get_draw_null_texture();
        config.circle_segment_count = 22;
        config.curve_segment_count = 22;
        config.arc_segment_count = 22;
        config.global_alpha = 1.0f;
        config.shape_AA = m_anti_aliasing ? NK_ANTI_ALIASING_ON : NK_ANTI_ALIASING_OFF;
        config.line_AA = m_anti_aliasing? NK_ANTI_ALIASING_ON : NK_ANTI_ALIASING_OFF;

		/* setup buffers to load vertices and elements */
		struct nk_buffer vbuf, ebuf;
		nk_buffer_init_fixed(&vbuf, m_sdl_impl->ogl.vertices_mem, (nk_size)m_sdl_impl->ogl.max_vertex_memory);
		nk_buffer_init_fixed(&ebuf, m_sdl_impl->ogl.elements_mem, (nk_size)m_sdl_impl->ogl.max_element_memory);
		nk_convert(get_ctx(), &dev->cmds, &vbuf, &ebuf, &config);

        glBufferSubData(GL_ARRAY_BUFFER, 0, (size_t)m_sdl_impl->ogl.max_vertex_memory, m_sdl_impl->ogl.vertices_mem);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, (size_t)m_sdl_impl->ogl.max_element_memory, m_sdl_impl->ogl.elements_mem);

        /* iterate over and execute each draw command */
        nk_draw_foreach(cmd, get_ctx(), &dev->cmds) {
            if (!cmd->elem_count) continue;
            glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
            glScissor((GLint)(cmd->clip_rect.x * scale.x),
                (GLint)((height - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h)) * scale.y),
                (GLint)(cmd->clip_rect.w * scale.x),
                (GLint)(cmd->clip_rect.h * scale.y));
            glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
            offset += cmd->elem_count;
        }
    }

    glUseProgram(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    SDL_GL_SwapWindow(m_sdl_impl->win);
}


void
NkWindowGLES2::do_ui()
{
	/* GUI */
	for (int i = 0; i < get_widgets().size(); ++i){
		get_widgets()[i]->draw(get_ctx());
	}
}

void
NkWindowGLES2::run()
{
	do_ui();
	render();
	nk_clear(get_ctx());

	while (is_running()){
		bool evt = handle_events();
		if (!evt){
			/*
			 * calm down the processor
			 */
			usleep(25000);
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

bool
NkWindowGLES2::handle_event()
{
    nk_context *ctx = get_ctx();
    SDL_Event* evt = &m_sdl_impl->evt;

    if (evt->type == SDL_KEYUP || evt->type == SDL_KEYDOWN) {
        /* key events */
        int down = evt->type == SDL_KEYDOWN;
        const Uint8* state = SDL_GetKeyboardState(0);
        SDL_Keycode sym = evt->key.keysym.sym;
        if (sym == SDLK_RSHIFT || sym == SDLK_LSHIFT)
            nk_input_key(ctx, NK_KEY_SHIFT, down);
        else if (sym == SDLK_DELETE)
            nk_input_key(ctx, NK_KEY_DEL, down);
        else if (sym == SDLK_RETURN)
            nk_input_key(ctx, NK_KEY_ENTER, down);
        else if (sym == SDLK_KP_ENTER)
            nk_input_key(ctx, NK_KEY_ENTER, down);
        else if (sym == SDLK_TAB)
            nk_input_key(ctx, NK_KEY_TAB, down);
        else if (sym == SDLK_BACKSPACE)
            nk_input_key(ctx, NK_KEY_BACKSPACE, down);
        else if (sym == SDLK_HOME) {
            nk_input_key(ctx, NK_KEY_TEXT_START, down);
            nk_input_key(ctx, NK_KEY_SCROLL_START, down);
        } else if (sym == SDLK_END) {
            nk_input_key(ctx, NK_KEY_TEXT_END, down);
            nk_input_key(ctx, NK_KEY_SCROLL_END, down);
        } else if (sym == SDLK_PAGEDOWN) {
            nk_input_key(ctx, NK_KEY_SCROLL_DOWN, down);
        } else if (sym == SDLK_PAGEUP) {
            nk_input_key(ctx, NK_KEY_SCROLL_UP, down);
        } else if (sym == SDLK_z)
            nk_input_key(ctx, NK_KEY_TEXT_UNDO, down && state[SDL_SCANCODE_LCTRL]);
        else if (sym == SDLK_r)
            nk_input_key(ctx, NK_KEY_TEXT_REDO, down && state[SDL_SCANCODE_LCTRL]);
        else if (sym == SDLK_c)
            nk_input_key(ctx, NK_KEY_COPY, down && state[SDL_SCANCODE_LCTRL]);
        else if (sym == SDLK_v)
            nk_input_key(ctx, NK_KEY_PASTE, down && state[SDL_SCANCODE_LCTRL]);
        else if (sym == SDLK_x)
            nk_input_key(ctx, NK_KEY_CUT, down && state[SDL_SCANCODE_LCTRL]);
        else if (sym == SDLK_b)
            nk_input_key(ctx, NK_KEY_TEXT_LINE_START, down && state[SDL_SCANCODE_LCTRL]);
        else if (sym == SDLK_e)
            nk_input_key(ctx, NK_KEY_TEXT_LINE_END, down && state[SDL_SCANCODE_LCTRL]);
        else if (sym == SDLK_UP)
            nk_input_key(ctx, NK_KEY_UP, down);
        else if (sym == SDLK_DOWN)
            nk_input_key(ctx, NK_KEY_DOWN, down);
        else if (sym == SDLK_LEFT) {
            if (state[SDL_SCANCODE_LCTRL])
                nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, down);
            else nk_input_key(ctx, NK_KEY_LEFT, down);
        } else if (sym == SDLK_RIGHT) {
            if (state[SDL_SCANCODE_LCTRL])
                nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, down);
            else nk_input_key(ctx, NK_KEY_RIGHT, down);
        } else return false;
        return true;
    } else if (evt->type == SDL_MOUSEBUTTONDOWN || evt->type == SDL_MOUSEBUTTONUP) {
        /* mouse button */
        int down = evt->type == SDL_MOUSEBUTTONDOWN;
        const int x = evt->button.x, y = evt->button.y;
        if (evt->button.button == SDL_BUTTON_LEFT) {
        	/*
        	 * button.clicks stores number of clicks
        	 */
            if (evt->button.clicks == 2)
                nk_input_button(ctx, NK_BUTTON_DOUBLE, x, y, down);
            nk_input_button(ctx, NK_BUTTON_LEFT, x, y, down);
        } else if (evt->button.button == SDL_BUTTON_MIDDLE)
            nk_input_button(ctx, NK_BUTTON_MIDDLE, x, y, down);
        else if (evt->button.button == SDL_BUTTON_RIGHT)
            nk_input_button(ctx, NK_BUTTON_RIGHT, x, y, down);
        //printf("Click %i %i\n", down, evt->button.clicks);
        return true;
    } else if (evt->type == SDL_MOUSEMOTION) {
        /* mouse motion */
        if (ctx->input.mouse.grabbed) {
            int x = (int)ctx->input.mouse.prev.x, y = (int)ctx->input.mouse.prev.y;
            nk_input_motion(ctx, x + evt->motion.xrel, y + evt->motion.yrel);
        } else {
        	nk_input_motion(ctx, evt->motion.x, evt->motion.y);
        }
        return true;
    } else if (evt->type == SDL_TEXTINPUT) {
        /* text input */
        nk_glyph glyph;
        memcpy(glyph, evt->text.text, NK_UTF_SIZE);
        nk_input_glyph(ctx, glyph);
        return true;
    } else if (evt->type == SDL_MOUSEWHEEL) {
        /* mouse wheel */
        nk_input_scroll(ctx,nk_vec2((float)evt->wheel.x,(float)evt->wheel.y));
        return true;
    }
    return false;
}

void
NkWindowGLES2::shutdown(void)
{
    nk_free(get_ctx());

    struct nk_sdl_device *dev = &m_sdl_impl->ogl;
    glDetachShader(dev->prog, dev->vert_shdr);
    glDetachShader(dev->prog, dev->frag_shdr);
    glDeleteShader(dev->vert_shdr);
    glDeleteShader(dev->frag_shdr);
    glDeleteProgram(dev->prog);
    glDeleteBuffers(1, &dev->vbo);
    glDeleteBuffers(1, &dev->ebo);
    nk_buffer_free(&dev->cmds);
}

UserEvent::UserEvent()
{
	m_event_idx = SDL_RegisterEvents(1);
	if (m_event_idx != -1)
		NkWindow::get()->register_user_event(this);
	else
		fprintf(stderr, "Cannot register user event\n");
}

UserEvent::~UserEvent()
{
	if (m_event_idx != -1)
		NkWindow::get()->unregister_user_event(this);
}

void UserEvent::push(int code, void* data1, void* data2)
{
	if (m_event_idx != -1){
		SDL_Event event;
		SDL_memset(&event, 0, sizeof(event));
		event.type = m_event_idx;
		event.user.code = code;
		event.user.data1 = data1;
		event.user.data2 = data2;
		SDL_PushEvent(&event);
	}
}

void
NkWindowGLES2::delete_texture(GLuint id)
{
	glDeleteTextures(1, &id);
}

GLuint
NkWindowGLES2::create_texture(const NkTexture& texture)
{
    return gl_create_texture(texture);
}

bool
NkWindowGLES2::handle_events()
{
	bool event_flag = false;

	handle_fd_events();
	handle_timer_events();

	nk_input_begin(get_ctx());
	while (SDL_PollEvent(&m_sdl_impl->evt)) {
		if (m_sdl_impl->evt.type == SDL_QUIT){
			set_running(false);
			break;
		}

		std::vector<UserEvent*>::iterator it = get_user_events().begin();
		for (; it < get_user_events().end(); ++it){
			if (m_sdl_impl->evt.type == (*it)->m_event_idx){
				event_flag |= true;
				(*it)->on_callback(m_sdl_impl->evt.user.data1, m_sdl_impl->evt.user.data2);
			}
		}

		/*
		 * SDL event handling
		 */
		event_flag |= handle_event();
	}

	nk_input_end(get_ctx());
	return event_flag;
}

void
UserEvent::on_callback(void* data1, void* data2)
{
	if (m_callback)
		m_userdata1 = data1;
		m_userdata2 = data2;
		m_callback(this, (void*)this, m_callback_data);
}

