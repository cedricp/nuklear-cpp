#include <nuklear_lib.h>
#include <thread.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <window_glx.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xlocale.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xlocale.h>

#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glxext.h>

#include <window_gl_common.h>

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

#ifndef NK_X11_DOUBLE_CLICK_LO
#define NK_X11_DOUBLE_CLICK_LO 20
#endif
#ifndef NK_X11_DOUBLE_CLICK_HI
#define NK_X11_DOUBLE_CLICK_HI 100
#endif

extern NkWindow* global_ptr;

struct nk_x11_device {
    struct nk_buffer cmds;
    GLuint vbo, vao, ebo;
    GLuint prog;
    GLuint vert_shdr;
    GLuint frag_shdr;
    GLint attrib_pos;
    GLint attrib_uv;
    GLint attrib_col;
    GLint uniform_tex;
    GLint uniform_proj;
    GLuint font_tex;
    size_t max_vertex_memory;
    size_t max_element_memory;
};

struct nk_x11_vertex {
    GLfloat position[2];
    GLfloat uv[2];
    nk_byte col[4];
};

struct XWindow {
    Display *dpy;
    Window win;
    XVisualInfo *vis;
    Colormap cmap;
    XSetWindowAttributes swa;
    XWindowAttributes attr;
    GLXFBConfig fbc;
    Atom wm_delete_window;
    int width, height;
};

enum graphics_card_vendors {
    VENDOR_UNKNOWN,
    VENDOR_NVIDIA,
    VENDOR_AMD,
    VENDOR_INTEL
};

struct opengl_info {
    /* info */
    const char *vendor_str;
    const char *version_str;
    const char *extensions_str;
    const char *renderer_str;
    const char *glsl_version_str;
    enum graphics_card_vendors vendor;
    /* version */
    float version;
    int major_version;
    int minor_version;
    /* extensions */
    int glsl_available;
    int vertex_buffer_obj_available;
    int vertex_array_obj_available;
    int map_buffer_range_available;
    int fragment_program_available;
    int frame_buffer_object_available;
};

struct glx_impl {
	nk_x11_device ogl;
	struct opengl_info glinfo;
    struct XWindow xwin;
    GLXContext glContext;
    Cursor cursor;
    XEvent evt;
    long last_button_click;
    Thread_mutex event_mutex;
    std::vector<UserEvent> event_queue;
};

/* GL_ARB_vertex_buffer_object */
typedef void(*nkglGenBuffers)(GLsizei, GLuint*);
typedef void(*nkglBindBuffer)(GLenum, GLuint);
typedef void(*nkglBufferData)(GLenum, GLsizeiptr, const GLvoid*, GLenum);
typedef void(*nkglBufferSubData)(GLenum, GLintptr, GLsizeiptr, const GLvoid*);
typedef void*(*nkglMapBuffer)(GLenum, GLenum);
typedef GLboolean(*nkglUnmapBuffer)(GLenum);
typedef void(*nkglDeleteBuffers)(GLsizei, GLuint*);
/* GL_ARB_vertex_array_object */
typedef void (*nkglGenVertexArrays)(GLsizei, GLuint*);
typedef void (*nkglBindVertexArray)(GLuint);
typedef void (*nkglDeleteVertexArrays)(GLsizei, const GLuint*);
/* GL_ARB_vertex_program / GL_ARB_fragment_program */
typedef void(*nkglVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid*);
typedef void(*nkglEnableVertexAttribArray)(GLuint);
typedef void(*nkglDisableVertexAttribArray)(GLuint);
/* GL_ARB_framebuffer_object */
typedef void(*nkglGenerateMipmap)(GLenum target);
/* GLSL/OpenGL 2.0 core */
typedef GLuint(*nkglCreateShader)(GLenum);
typedef void(*nkglShaderSource)(GLuint, GLsizei, const GLchar**, const GLint*);
typedef void(*nkglCompileShader)(GLuint);
typedef void(*nkglGetShaderiv)(GLuint, GLenum, GLint*);
typedef void(*nkglGetShaderInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void(*nkglDeleteShader)(GLuint);
typedef GLuint(*nkglCreateProgram)(void);
typedef void(*nkglAttachShader)(GLuint, GLuint);
typedef void(*nkglDetachShader)(GLuint, GLuint);
typedef void(*nkglLinkProgram)(GLuint);
typedef void(*nkglUseProgram)(GLuint);
typedef void(*nkglGetProgramiv)(GLuint, GLenum, GLint*);
typedef void(*nkglGetProgramInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void(*nkglDeleteProgram)(GLuint);
typedef GLint(*nkglGetUniformLocation)(GLuint, const GLchar*);
typedef GLint(*nkglGetAttribLocation)(GLuint, const GLchar*);
typedef void(*nkglUniform1i)(GLint, GLint);
typedef void(*nkglUniform1f)(GLint, GLfloat);
typedef void(*nkglUniformMatrix3fv)(GLint, GLsizei, GLboolean, const GLfloat*);
typedef void(*nkglUniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat*);

static nkglGenBuffers glGenBuffers;
static nkglBindBuffer glBindBuffer;
static nkglBufferData glBufferData;
static nkglBufferSubData glBufferSubData;
static nkglMapBuffer glMapBuffer;
static nkglUnmapBuffer glUnmapBuffer;
static nkglDeleteBuffers glDeleteBuffers;
static nkglGenVertexArrays glGenVertexArrays;
static nkglBindVertexArray glBindVertexArray;
static nkglDeleteVertexArrays glDeleteVertexArrays;
static nkglVertexAttribPointer glVertexAttribPointer;
static nkglEnableVertexAttribArray glEnableVertexAttribArray;
static nkglDisableVertexAttribArray glDisableVertexAttribArray;
static nkglGenerateMipmap glGenerateMipmap;
static nkglCreateShader glCreateShader;
static nkglShaderSource glShaderSource;
static nkglCompileShader glCompileShader;
static nkglGetShaderiv glGetShaderiv;
static nkglGetShaderInfoLog glGetShaderInfoLog;
static nkglDeleteShader glDeleteShader;
static nkglCreateProgram glCreateProgram;
static nkglAttachShader glAttachShader;
static nkglDetachShader glDetachShader;
static nkglLinkProgram glLinkProgram;
static nkglUseProgram glUseProgram;
static nkglGetProgramiv glGetProgramiv;
static nkglGetProgramInfoLog glGetProgramInfoLog;
static nkglDeleteProgram glDeleteProgram;
static nkglGetUniformLocation glGetUniformLocation;
static nkglGetAttribLocation glGetAttribLocation;
static nkglUniform1i glUniform1i;
static nkglUniform1f glUniform1f;
static nkglUniformMatrix3fv glUniformMatrix3fv;
static nkglUniformMatrix4fv glUniformMatrix4fv;

#define NK_SHADER_VERSION "#version 300 es\n"

static int gl_err = nk_false;
static int gl_error_handler(Display *dpy, XErrorEvent *ev)
{NK_UNUSED(dpy); NK_UNUSED(ev); gl_err = nk_true;return 0;}

static int
has_extension(const char *string, const char *ext)
{
    const char *start, *where, *term;
    where = strchr(ext, ' ');
    if (where || *ext == '\0')
        return nk_false;

    for (start = string;;) {
        where = strstr((const char*)start, ext);
        if (!where) break;
        term = where + strlen(ext);
        if (where == start || *(where - 1) == ' ') {
            if (*term == ' ' || *term == '\0')
                return nk_true;
        }
        start = term;
    }
    return nk_false;
}

#define GL_EXT(name) (nk##name)nk_gl_ext(#name)
NK_INTERN __GLXextFuncPtr
nk_gl_ext(const char *name)
{
    __GLXextFuncPtr func;
    func = glXGetProcAddress((const GLubyte*)name);
    if (!func) {
        fprintf(stdout, "[GL]: failed to load extension: %s", name);
        return NULL;
    }
    return func;
}

NK_INTERN int
nk_x11_check_extension(struct opengl_info *GL, const char *ext)
{
    const char *start, *where, *term;
    where = strchr(ext, ' ');
    if (where || *ext == '\0')
        return nk_false;

    for (start = GL->extensions_str;;) {
        where = strstr((const char*)start, ext);
        if (!where) break;
        term = where + strlen(ext);
        if (where == start || *(where - 1) == ' ') {
            if (*term == ' ' || *term == '\0')
                return nk_true;
        }
        start = term;
    }
    return nk_false;
}

NK_INTERN int
nk_x11_stricmpn(const char *a, const char *b, int len)
{
    int i = 0;
    for (i = 0; i < len && a[i] && b[i]; ++i)
        if (a[i] != b[i]) return 1;
    if (i != len) return 1;
    return 0;
}

static const GLchar *vertex_shader =
	NK_SHADER_VERSION
	"uniform mat4 ProjMtx;\n"
	"in vec2 Position;\n"
	"in vec2 TexCoord;\n"
	"in vec4 Color;\n"
	"out vec2 Frag_UV;\n"
	"out vec4 Frag_Color;\n"
	"void main() {\n"
	"   Frag_UV = TexCoord;\n"
	"   Frag_Color = Color;\n"
	"   gl_Position = ProjMtx * vec4(Position.xy, 0, 1);\n"
	"}\n";

static const GLchar *fragment_shader =
	NK_SHADER_VERSION
	"precision mediump float;\n"
	"uniform sampler2D Texture;\n"
	"in vec2 Frag_UV;\n"
	"in vec4 Frag_Color;\n"
	"out vec4 Out_Color;\n"
	"void main(){\n"
	"   Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
	"}\n";

NkWindowGLX::NkWindowGLX(int width, int height, size_t max_vert_mem, size_t max_elem_mem, bool aa) : NkWindow(width, height, aa)
{
	m_glx_impl = new glx_impl;

	m_glx_impl->ogl.max_vertex_memory = max_vert_mem;
	m_glx_impl->ogl.max_element_memory = max_elem_mem;

	assert(init_GLX() == true);
	assert(init_GL_functions() == true);
	assert(init_GLSL(max_elem_mem, max_vert_mem) == true);
}

bool NkWindowGLX::init_GLX()
{
	// GLX init
	{

	    struct nk_context *ctx;
	    struct nk_colorf bg;

	    memset(&m_glx_impl->xwin.win, 0, sizeof(m_glx_impl->xwin.win));
	    m_glx_impl->xwin.dpy = XOpenDisplay(NULL);
	    if (!m_glx_impl->xwin.dpy)
	    	return false;
	    {
	        /* check glx version */
	        int glx_major, glx_minor;
	        if (!glXQueryVersion(m_glx_impl->xwin.dpy, &glx_major, &glx_minor))
	        	return false;
	        if ((glx_major == 1 && glx_minor < 3) || (glx_major < 1))
	        	return false;
	    }
	    {
	        /* find and pick matching framebuffer visual */
	        int fb_count;
	        static GLint attr[] = {
	            GLX_X_RENDERABLE,   True,
	            GLX_DRAWABLE_TYPE,  GLX_WINDOW_BIT,
	            GLX_RENDER_TYPE,    GLX_RGBA_BIT,
	            GLX_X_VISUAL_TYPE,  GLX_TRUE_COLOR,
	            GLX_RED_SIZE,       8,
	            GLX_GREEN_SIZE,     8,
	            GLX_BLUE_SIZE,      8,
	            GLX_ALPHA_SIZE,     8,
	            GLX_DEPTH_SIZE,     24,
	            GLX_STENCIL_SIZE,   8,
	            GLX_DOUBLEBUFFER,   True,
	            None
	        };
	        GLXFBConfig *fbc;
	        fbc = glXChooseFBConfig(m_glx_impl->xwin.dpy, DefaultScreen(m_glx_impl->xwin.dpy), attr, &fb_count);
	        if (!fbc)
	        	return false;
	        {
	            /* pick framebuffer with most samples per pixel */
	            int i;
	            int fb_best = -1, best_num_samples = -1;
	            for (i = 0; i < fb_count; ++i) {
	                XVisualInfo *vi = glXGetVisualFromFBConfig(m_glx_impl->xwin.dpy, fbc[i]);
	                if (vi) {
	                    int sample_buffer, samples;
	                    glXGetFBConfigAttrib(m_glx_impl->xwin.dpy, fbc[i], GLX_SAMPLE_BUFFERS, &sample_buffer);
	                    glXGetFBConfigAttrib(m_glx_impl->xwin.dpy, fbc[i], GLX_SAMPLES, &samples);
	                    if ((fb_best < 0) || (sample_buffer && samples > best_num_samples))
	                        fb_best = i, best_num_samples = samples;
	                }
	            }
	            m_glx_impl->xwin.fbc = fbc[fb_best];
	            XFree(fbc);
	            m_glx_impl->xwin.vis = glXGetVisualFromFBConfig(m_glx_impl->xwin.dpy, m_glx_impl->xwin.fbc);
	        }
	    }
	    {
	        /* create window */
	    	m_glx_impl->xwin.cmap = XCreateColormap(m_glx_impl->xwin.dpy,
	    			RootWindow(m_glx_impl->xwin.dpy, m_glx_impl->xwin.vis->screen),
					m_glx_impl->xwin.vis->visual, AllocNone);
	    	m_glx_impl->xwin.swa.colormap = m_glx_impl->xwin.cmap;
	    	m_glx_impl->xwin.swa.background_pixmap = None;
	    	m_glx_impl->xwin.swa.border_pixel = 0;
	    	m_glx_impl->xwin.swa.event_mask =
	            ExposureMask | KeyPressMask | KeyReleaseMask |
	            ButtonPress | ButtonReleaseMask| ButtonMotionMask |
	            Button1MotionMask | Button3MotionMask | Button4MotionMask | Button5MotionMask|
	            PointerMotionMask| StructureNotifyMask;
	    	m_glx_impl->xwin.win = XCreateWindow(m_glx_impl->xwin.dpy, RootWindow(m_glx_impl->xwin.dpy, m_glx_impl->xwin.vis->screen), 0, 0,
	            get_width(), get_height(), 0, m_glx_impl->xwin.vis->depth, InputOutput,
				m_glx_impl->xwin.vis->visual, CWBorderPixel|CWColormap|CWEventMask, &m_glx_impl->xwin.swa);
	        if (!m_glx_impl->xwin.win)
	        	return false;
	        XFree(m_glx_impl->xwin.vis);
	        XStoreName(m_glx_impl->xwin.dpy, m_glx_impl->xwin.win, "Demo");
	        XMapWindow(m_glx_impl->xwin.dpy, m_glx_impl->xwin.win);
	        m_glx_impl->xwin.wm_delete_window = XInternAtom(m_glx_impl->xwin.dpy, "WM_DELETE_WINDOW", False);
	        XSetWMProtocols(m_glx_impl->xwin.dpy, m_glx_impl->xwin.win, &m_glx_impl->xwin.wm_delete_window, 1);
	    }
	    {
	        /* create opengl context */
	        typedef GLXContext(*glxCreateContext)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
	        int(*old_handler)(Display*, XErrorEvent*) = XSetErrorHandler(gl_error_handler);
	        const char *extensions_str = glXQueryExtensionsString(m_glx_impl->xwin.dpy, DefaultScreen(m_glx_impl->xwin.dpy));
	        glxCreateContext create_context = (glxCreateContext)
	            glXGetProcAddressARB((const GLubyte*)"glXCreateContextAttribsARB");

	        gl_err = nk_false;
	        if (!has_extension(extensions_str, "GLX_ARB_create_context") || !create_context) {
	            fprintf(stdout, "[X11]: glXCreateContextAttribARB() not found...\n");
	            fprintf(stdout, "[X11]: ... using old-style GLX context\n");
	            m_glx_impl->glContext = glXCreateNewContext(m_glx_impl->xwin.dpy, m_glx_impl->xwin.fbc, GLX_RGBA_TYPE, 0, True);
	        } else {
	            GLint attr[] = {
	                GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
	                GLX_CONTEXT_MINOR_VERSION_ARB, 0,
	                None
	            };
	            m_glx_impl->glContext = create_context(m_glx_impl->xwin.dpy, m_glx_impl->xwin.fbc, 0, True, attr);
	            XSync(m_glx_impl->xwin.dpy, False);
	            if (gl_err || !m_glx_impl->glContext) {
	                /* Could not create GL 3.0 context. Fallback to old 2.x context.
	                 * If a version below 3.0 is requested, implementations will
	                 * return the newest context version compatible with OpenGL
	                 * version less than version 3.0.*/
	                attr[1] = 1; attr[3] = 0;
	                gl_err = nk_false;
	                fprintf(stdout, "[X11] Failed to create OpenGL 3.0 context\n");
	                fprintf(stdout, "[X11] ... using old-style GLX context!\n");
	                m_glx_impl->glContext = create_context(m_glx_impl->xwin.dpy, m_glx_impl->xwin.fbc, 0, True, attr);
	            }
	        }
	        XSync(m_glx_impl->xwin.dpy, False);
	        XSetErrorHandler(old_handler);
	        if (gl_err || !m_glx_impl->glContext)
	            return false;
	        glXMakeCurrent(m_glx_impl->xwin.dpy, m_glx_impl->xwin.win, m_glx_impl->glContext);
	    }
	}


    if (!setlocale(LC_ALL,"")) return false;
    if (!XSupportsLocale()) return false;
    if (!XSetLocaleModifiers("@im=none")) return false;

    /* create invisible cursor */
    {
    	static XColor dummy; char data[1] = {0};
		Pixmap blank = XCreateBitmapFromData(m_glx_impl->xwin.dpy, m_glx_impl->xwin.win, data, 1, 1);
		if (blank == None) return false;
		m_glx_impl->cursor = XCreatePixmapCursor(m_glx_impl->xwin.dpy, blank, blank, &dummy, &dummy, 0, 0);
		XFreePixmap(m_glx_impl->xwin.dpy, blank);
    }

    return true;
}

NkWindowGLX::~NkWindowGLX()
{
	cleanup();
	shutdown();
}

NkWindow*
NkWindowGLX::create(int w, int h, bool aa)
{
	if (global_ptr != NULL)
		return global_ptr;

	global_ptr = (NkWindow*)new NkWindowGLX(w, h, DFT_MAX_VERTEX_MEMORY, DFT_MAX_ELEMENT_MEMORY, aa);
	return global_ptr;
}

void
NkWindowGLX::render()
{
	XWindowAttributes attr;
    struct nk_x11_device *dev = &m_glx_impl->ogl;
    int width, height;
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
    XGetWindowAttributes(m_glx_impl->xwin.dpy,  m_glx_impl->xwin.win, &attr);
	width = attr.width;
	height = attr.height;

	ortho[0][0] /= (GLfloat)width;
	ortho[1][1] /= (GLfloat)height;

	glXMakeCurrent(m_glx_impl->xwin.dpy, m_glx_impl->xwin.win, m_glx_impl->glContext);

	/* setup global state */
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
	glViewport(0,0,(GLsizei)width,(GLsizei)height);
    glClearColor(bg[0], bg[1], bg[2], bg[3]);
	glClear(GL_COLOR_BUFFER_BIT);

	/* convert from command queue into draw list and draw to screen */
	const struct nk_draw_command *cmd;
	void *vertices, *elements;
	const nk_draw_index *offset = NULL;

	/* allocate vertex and element buffer */
	glBindVertexArray(dev->vao);
	glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

	glBufferData(GL_ARRAY_BUFFER, m_glx_impl->ogl.max_vertex_memory, NULL, GL_STREAM_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_glx_impl->ogl.max_element_memory, NULL, GL_STREAM_DRAW);

	/* load draw vertices & elements directly into vertex + element buffer */
	vertices = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	elements = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);

	/* fill convert configuration */
	static const struct nk_draw_vertex_layout_element vertex_layout[] = {
		{NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_x11_vertex, position)},
		{NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_x11_vertex, uv)},
		{NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct nk_x11_vertex, col)},
		{NK_VERTEX_LAYOUT_END}
	};
	 struct nk_convert_config config;
	NK_MEMSET(&config, 0, sizeof(config));
	config.vertex_layout = vertex_layout;
	config.vertex_size = sizeof(struct nk_x11_vertex);
	config.vertex_alignment = NK_ALIGNOF(struct nk_x11_vertex);
	config.null = *get_draw_null_texture();
	config.circle_segment_count = 22;
	config.curve_segment_count = 22;
	config.arc_segment_count = 22;
	config.global_alpha = 1.0f;
	config.shape_AA = m_anti_aliasing ? NK_ANTI_ALIASING_ON : NK_ANTI_ALIASING_OFF;
	config.line_AA = m_anti_aliasing ? NK_ANTI_ALIASING_ON : NK_ANTI_ALIASING_OFF;

	/* setup buffers to load vertices and elements */
	struct nk_buffer vbuf, ebuf;
	nk_buffer_init_fixed(&vbuf, vertices, (nk_size)m_glx_impl->ogl.max_vertex_memory);
	nk_buffer_init_fixed(&ebuf, elements, (nk_size)m_glx_impl->ogl.max_element_memory);
	nk_convert(get_ctx(), &dev->cmds, &vbuf, &ebuf, &config);
	glUnmapBuffer(GL_ARRAY_BUFFER);
	glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

	/* iterate over and execute each draw command */
	nk_draw_foreach(cmd, get_ctx(), &dev->cmds){
		if (!cmd->elem_count) continue;
		glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
		glScissor(
			(GLint)(cmd->clip_rect.x),
			(GLint)((height - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h))),
			(GLint)(cmd->clip_rect.w),
			(GLint)(cmd->clip_rect.h));
		glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
		offset += cmd->elem_count;
	}

	/* default OpenGL state */
	glUseProgram(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glDisable(GL_BLEND);
	glDisable(GL_SCISSOR_TEST);

    glXSwapBuffers(m_glx_impl->xwin.dpy, m_glx_impl->xwin.win);
}

void
NkWindowGLX::draw_ui()
{
	/* GUI */
	for (NkWidget* w : get_widgets()){
		w->draw(get_ctx());
	}

    render();
    nk_clear(get_ctx());
}

void
NkWindowGLX::run()
{
	draw_ui();
	while (is_running()){
		bool evt = handle_events();
		if (!evt){
			usleep(25000);
			continue;
		}
		draw_ui();
 	}
}

bool NkWindowGLX::init_GL_functions() {
	int failed = nk_false;
	struct opengl_info *gl = &m_glx_impl->glinfo;
	gl->version_str = (const char*)glGetString(GL_VERSION);
	glGetIntegerv(GL_MAJOR_VERSION, &gl->major_version);
	glGetIntegerv(GL_MINOR_VERSION, &gl->minor_version);
	if (gl->major_version < 2) {
		fprintf(stderr, "[GL]: Graphics card does not fullfill minimum OpenGL 2.0 support [%s] [%i] [%i]\n", gl->version_str, gl->major_version, gl->minor_version);
		return false;
	} else {
		fprintf(stderr, "[GL]: OpenGL OK :  [%s] [%i] [%i]\n", gl->version_str, gl->major_version, gl->minor_version);
	}
	gl->version = (float)gl->major_version + (float)gl->minor_version * 0.1f;
	gl->renderer_str = (const char*)glGetString(GL_RENDERER);
	gl->extensions_str = (const char*)glGetString(GL_EXTENSIONS);
	gl->glsl_version_str = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
	gl->vendor_str = (const char*)glGetString(GL_VENDOR);
	if (!nk_x11_stricmpn(gl->vendor_str, "ATI", 4) ||
		!nk_x11_stricmpn(gl->vendor_str, "AMD", 4))
		gl->vendor = VENDOR_AMD;
	else if (!nk_x11_stricmpn(gl->vendor_str, "NVIDIA", 6))
		gl->vendor = VENDOR_NVIDIA;
	else if (!nk_x11_stricmpn(gl->vendor_str, "Intel", 5))
		gl->vendor = VENDOR_INTEL;
	else gl->vendor = VENDOR_UNKNOWN;

	/* Extensions */
	gl->glsl_available = (gl->version >= 2.0f);
	if (gl->glsl_available) {
		/* GLSL core in OpenGL > 2 */
		glCreateShader = GL_EXT(glCreateShader);
		glShaderSource = GL_EXT(glShaderSource);
		glCompileShader = GL_EXT(glCompileShader);
		glGetShaderiv = GL_EXT(glGetShaderiv);
		glGetShaderInfoLog = GL_EXT(glGetShaderInfoLog);
		glDeleteShader = GL_EXT(glDeleteShader);
		glCreateProgram = GL_EXT(glCreateProgram);
		glAttachShader = GL_EXT(glAttachShader);
		glDetachShader = GL_EXT(glDetachShader);
		glLinkProgram = GL_EXT(glLinkProgram);
		glUseProgram = GL_EXT(glUseProgram);
		glGetProgramiv = GL_EXT(glGetProgramiv);
		glGetProgramInfoLog = GL_EXT(glGetProgramInfoLog);
		glDeleteProgram = GL_EXT(glDeleteProgram);
		glGetUniformLocation = GL_EXT(glGetUniformLocation);
		glGetAttribLocation = GL_EXT(glGetAttribLocation);
		glUniform1i = GL_EXT(glUniform1i);
		glUniform1f = GL_EXT(glUniform1f);
		glUniformMatrix3fv = GL_EXT(glUniformMatrix3fv);
		glUniformMatrix4fv = GL_EXT(glUniformMatrix4fv);
	}
	gl->vertex_buffer_obj_available = nk_x11_check_extension(gl, "GL_ARB_vertex_buffer_object");
	if (gl->vertex_buffer_obj_available) {
		/* GL_ARB_vertex_buffer_object */
		glGenBuffers = GL_EXT(glGenBuffers);
		glBindBuffer = GL_EXT(glBindBuffer);
		glBufferData = GL_EXT(glBufferData);
		glBufferSubData = GL_EXT(glBufferSubData);
		glMapBuffer = GL_EXT(glMapBuffer);
		glUnmapBuffer = GL_EXT(glUnmapBuffer);
		glDeleteBuffers = GL_EXT(glDeleteBuffers);
	}
	gl->fragment_program_available = nk_x11_check_extension(gl, "GL_ARB_fragment_program");
	if (gl->fragment_program_available) {
		/* GL_ARB_vertex_program / GL_ARB_fragment_program  */
		glVertexAttribPointer = GL_EXT(glVertexAttribPointer);
		glEnableVertexAttribArray = GL_EXT(glEnableVertexAttribArray);
		glDisableVertexAttribArray = GL_EXT(glDisableVertexAttribArray);
	}
	gl->vertex_array_obj_available = nk_x11_check_extension(gl, "GL_ARB_vertex_array_object");
	if (gl->vertex_array_obj_available) {
		/* GL_ARB_vertex_array_object */
		glGenVertexArrays = GL_EXT(glGenVertexArrays);
		glBindVertexArray = GL_EXT(glBindVertexArray);
		glDeleteVertexArrays = GL_EXT(glDeleteVertexArrays);
	}
	gl->frame_buffer_object_available = nk_x11_check_extension(gl, "GL_ARB_framebuffer_object");
	if (gl->frame_buffer_object_available) {
		/* GL_ARB_framebuffer_object */
		glGenerateMipmap = GL_EXT(glGenerateMipmap);
	}
	if (!gl->vertex_buffer_obj_available) {
		fprintf(stdout, "[GL] Error: GL_ARB_vertex_buffer_object is not available!\n");
		failed = nk_true;
	}
	if (!gl->fragment_program_available) {
		fprintf(stdout, "[GL] Error: GL_ARB_fragment_program is not available!\n");
		failed = nk_true;
	}
	if (!gl->vertex_array_obj_available) {
		fprintf(stdout, "[GL] Error: GL_ARB_vertex_array_object is not available!\n");
		failed = nk_true;
	}
	if (!gl->frame_buffer_object_available) {
		fprintf(stdout, "[GL] Error: GL_ARB_framebuffer_object is not available!\n");
		failed = nk_true;
	}
	return true;
}

/*
 * Thread safe event registration
 */
void
NkWindowGLX::push_user_event(UserEvent* ev)
{
	m_glx_impl->event_mutex.lock();
	m_glx_impl->event_queue.push_back(*ev);
	m_glx_impl->event_mutex.unlock();
}

void
NkWindowGLX::shutdown(void)
{
    nk_free(get_ctx());

    struct nk_x11_device *dev = &m_glx_impl->ogl;
    glDetachShader(dev->prog, dev->vert_shdr);
    glDetachShader(dev->prog, dev->frag_shdr);
    glDeleteShader(dev->vert_shdr);
    glDeleteShader(dev->frag_shdr);
    glDeleteProgram(dev->prog);
    glDeleteBuffers(1, &dev->vbo);
    glDeleteBuffers(1, &dev->ebo);
    nk_buffer_free(&dev->cmds);

    glXMakeCurrent(m_glx_impl->xwin.dpy, 0, 0);
    glXDestroyContext(m_glx_impl->xwin.dpy, m_glx_impl->glContext);
    XUnmapWindow(m_glx_impl->xwin.dpy, m_glx_impl->xwin.win);
    XFreeColormap(m_glx_impl->xwin.dpy, m_glx_impl->xwin.cmap);
    XDestroyWindow(m_glx_impl->xwin.dpy, m_glx_impl->xwin.win);
    XCloseDisplay(m_glx_impl->xwin.dpy);
}

void
NkWindowGLX::delete_texture(GLuint id)
{
	glDeleteTextures(1, &id);
}

GLuint
NkWindowGLX::create_texture(const NkTexture& texture)
{
    return gl_create_texture(texture);
}

bool
NkWindowGLX::handle_event()
{
    nk_context *ctx = get_ctx();

    if (ctx->input.mouse.grab) {
        XDefineCursor(m_glx_impl->xwin.dpy, m_glx_impl->xwin.win, m_glx_impl->cursor);
        ctx->input.mouse.grab = 0;
    } else if (ctx->input.mouse.ungrab) {
        XWarpPointer(m_glx_impl->xwin.dpy, None, m_glx_impl->xwin.win, 0, 0, 0, 0,
            (int)ctx->input.mouse.pos.x, (int)ctx->input.mouse.pos.y);
        XUndefineCursor(m_glx_impl->xwin.dpy, m_glx_impl->xwin.win);
        ctx->input.mouse.ungrab = 0;
    }

    XEvent& evt = m_glx_impl->evt;
	if (evt.type == KeyPress || evt.type == KeyRelease)
	{
		/* Key handler */
		int ret, down = (evt.type == KeyPress);
		KeySym *code = XGetKeyboardMapping(m_glx_impl->xwin.dpy, (KeyCode)evt.xkey.keycode, 1, &ret);
		if (*code == XK_Shift_L || *code == XK_Shift_R) nk_input_key(ctx, NK_KEY_SHIFT, down);
		else if (*code == XK_Delete)    nk_input_key(ctx, NK_KEY_DEL, down);
		else if (*code == XK_Return || *code == XK_KP_Enter)    nk_input_key(ctx, NK_KEY_ENTER, down);
		else if (*code == XK_Tab)       nk_input_key(ctx, NK_KEY_TAB, down);
		else if (*code == XK_Left)      nk_input_key(ctx, NK_KEY_LEFT, down);
		else if (*code == XK_Right)     nk_input_key(ctx, NK_KEY_RIGHT, down);
		else if (*code == XK_Up)        nk_input_key(ctx, NK_KEY_UP, down);
		else if (*code == XK_Down)      nk_input_key(ctx, NK_KEY_DOWN, down);
		else if (*code == XK_BackSpace) nk_input_key(ctx, NK_KEY_BACKSPACE, down);
		else if (*code == XK_space && !down) nk_input_char(ctx, ' ');
		else if (*code == XK_Page_Up)   nk_input_key(ctx, NK_KEY_SCROLL_UP, down);
		else if (*code == XK_Page_Down) nk_input_key(ctx, NK_KEY_SCROLL_DOWN, down);
		else if (*code == XK_Home) {
			nk_input_key(ctx, NK_KEY_TEXT_START, down);
			nk_input_key(ctx, NK_KEY_SCROLL_START, down);
		} else if (*code == XK_End) {
			nk_input_key(ctx, NK_KEY_TEXT_END, down);
			nk_input_key(ctx, NK_KEY_SCROLL_END, down);
		} else {
			if (*code == 'c' && (evt.xkey.state & ControlMask))
				nk_input_key(ctx, NK_KEY_COPY, down);
			else if (*code == 'v' && (evt.xkey.state & ControlMask))
				nk_input_key(ctx, NK_KEY_PASTE, down);
			else if (*code == 'x' && (evt.xkey.state & ControlMask))
				nk_input_key(ctx, NK_KEY_CUT, down);
			else if (*code == 'z' && (evt.xkey.state & ControlMask))
				nk_input_key(ctx, NK_KEY_TEXT_UNDO, down);
			else if (*code == 'r' && (evt.xkey.state & ControlMask))
				nk_input_key(ctx, NK_KEY_TEXT_REDO, down);
			else if (*code == XK_Left && (evt.xkey.state & ControlMask))
				nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, down);
			else if (*code == XK_Right && (evt.xkey.state & ControlMask))
				nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, down);
			else if (*code == 'b' && (evt.xkey.state & ControlMask))
				nk_input_key(ctx, NK_KEY_TEXT_LINE_START, down);
			else if (*code == 'e' && (evt.xkey.state & ControlMask))
				nk_input_key(ctx, NK_KEY_TEXT_LINE_END, down);
			else {
				if (*code == 'i')
					nk_input_key(ctx, NK_KEY_TEXT_INSERT_MODE, down);
				else if (*code == 'r')
					nk_input_key(ctx, NK_KEY_TEXT_REPLACE_MODE, down);
				if (down) {
					char buf[32];
					KeySym keysym = 0;
					if (XLookupString((XKeyEvent*)&evt, buf, 32, &keysym, NULL) != NoSymbol)
						nk_input_glyph(ctx, buf);
				}
			}
		}
		XFree(code);
		return true;
	} else if (evt.type == ButtonPress || evt.type == ButtonRelease) {
        /* Button handler */
        int down = (evt.type == ButtonPress);
        const int x = evt.xbutton.x, y = evt.xbutton.y;
        if (evt.xbutton.button == Button1) {
            if (down) { /* Double-Click Button handler */
                long dt = timestamp() - m_glx_impl->last_button_click;
                if (dt > NK_X11_DOUBLE_CLICK_LO && dt < NK_X11_DOUBLE_CLICK_HI)
                    nk_input_button(ctx, NK_BUTTON_DOUBLE, x, y, nk_true);
                m_glx_impl->last_button_click = timestamp();
            } else nk_input_button(ctx, NK_BUTTON_DOUBLE, x, y, nk_false);
            nk_input_button(ctx, NK_BUTTON_LEFT, x, y, down);
        } else if (evt.xbutton.button == Button2)
            nk_input_button(ctx, NK_BUTTON_MIDDLE, x, y, down);
        else if (evt.xbutton.button == Button3)
            nk_input_button(ctx, NK_BUTTON_RIGHT, x, y, down);
        else if (evt.xbutton.button == Button4)
            nk_input_scroll(ctx, nk_vec2(0,1.0f));
        else if (evt.xbutton.button == Button5)
            nk_input_scroll(ctx, nk_vec2(0,-1.0f));
        else return false;
        return true;
	} else if (evt.type == MotionNotify) {
		/* Mouse motion handler */
		const int x = evt.xmotion.x, y = evt.xmotion.y;
		nk_input_motion(ctx, x, y);
		if (ctx->input.mouse.grabbed) {
			ctx->input.mouse.pos.x = ctx->input.mouse.prev.x;
			ctx->input.mouse.pos.y = ctx->input.mouse.prev.y;
			XWarpPointer(m_glx_impl->xwin.dpy, None, m_glx_impl->xwin.win, 0, 0, 0, 0, (int)ctx->input.mouse.pos.x, (int)ctx->input.mouse.pos.y);
		}
		return true;
	} else if (evt.type == KeymapNotify) {
		XRefreshKeyboardMapping(&evt.xmapping);
		return true;
	}

    return false;
}

bool
NkWindowGLX::handle_events()
{
	bool event_flag = false;

	handle_fd_events();
	handle_timer_events();

	while (!m_glx_impl->event_queue.empty()){
		m_glx_impl->event_mutex.lock();
		UserEvent& ev = m_glx_impl->event_queue.front();
		event_flag |= true;
		ev.on_callback();
		m_glx_impl->event_queue.erase(m_glx_impl->event_queue.begin());
		m_glx_impl->event_mutex.unlock();
	}


	nk_input_begin(get_ctx());

	while (XPending(m_glx_impl->xwin.dpy)) {
		XNextEvent(m_glx_impl->xwin.dpy, &m_glx_impl->evt);
		if (m_glx_impl->evt.type == ClientMessage)
			set_running(false);
		if (XFilterEvent(&m_glx_impl->evt, m_glx_impl->xwin.win))
			continue;

		event_flag |= handle_event();
	}

	nk_input_end(get_ctx());

	return event_flag;
}

void
UserEvent::on_callback(void* data1, void* data2)
{
	if (m_callback)
		m_callback(this, (void*)this, m_callback_data);
}

bool NkWindowGLX::init_GLSL(int max_elem_mem, int max_vert_mem) {
	struct opengl_info *gl = &m_glx_impl->glinfo;
	m_glx_impl->ogl.max_element_memory = max_elem_mem;
	m_glx_impl->ogl.max_element_memory = max_vert_mem;

    // GLSL init
	GLint status;

	struct nk_x11_device *dev = &m_glx_impl->ogl;
	nk_buffer_init_default(&dev->cmds);

	dev->prog = glCreateProgram();
	dev->vert_shdr = glCreateShader(GL_VERTEX_SHADER);
	dev->frag_shdr = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(dev->vert_shdr, 1, &vertex_shader, 0);
	glShaderSource(dev->frag_shdr, 1, &fragment_shader, 0);
	glCompileShader(dev->vert_shdr);
	glCompileShader(dev->frag_shdr);
	glGetShaderiv(dev->vert_shdr, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE){
		GLint maxLength = 0;
		glGetShaderiv(dev->vert_shdr, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		std::vector<GLchar> errorLog(maxLength);
		glGetShaderInfoLog(dev->vert_shdr, maxLength, &maxLength, &errorLog[0]);
		printf("Error : %s\n", errorLog.data());
		// Provide the infolog in whatever manor you deem best.
		// Exit with failure.
		glDeleteShader(dev->vert_shdr); // Don't leak the shader.
	}
	assert(status == GL_TRUE);
	glGetShaderiv(dev->frag_shdr, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE){
		GLint maxLength = 0;
		glGetShaderiv(dev->vert_shdr, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		std::vector<GLchar> errorLog(maxLength);
		glGetShaderInfoLog(dev->vert_shdr, maxLength, &maxLength, &errorLog[0]);
		printf("Error : %s\n", errorLog.data());
		// Provide the infolog in whatever manor you deem best.
		// Exit with failure.
		glDeleteShader(dev->vert_shdr); // Don't leak the shader.
	}
	assert(status == GL_TRUE);
	glAttachShader(dev->prog, dev->vert_shdr);
	glAttachShader(dev->prog, dev->frag_shdr);
	glLinkProgram(dev->prog);
	glGetProgramiv(dev->prog, GL_LINK_STATUS, &status);
	assert(status == GL_TRUE);

	dev->uniform_tex  = glGetUniformLocation(dev->prog, "Texture");
	dev->uniform_proj = glGetUniformLocation(dev->prog, "ProjMtx");
	dev->attrib_pos   = glGetAttribLocation(dev->prog, "Position");
	dev->attrib_uv    = glGetAttribLocation(dev->prog, "TexCoord");
	dev->attrib_col   = glGetAttribLocation(dev->prog, "Color");

	/* buffer setup */
	GLsizei vs = sizeof(struct nk_x11_vertex);
	size_t vp = offsetof(struct nk_x11_vertex, position);
	size_t vt = offsetof(struct nk_x11_vertex, uv);
	size_t vc = offsetof(struct nk_x11_vertex, col);

	glGenBuffers(1, &dev->vbo);
	glGenBuffers(1, &dev->ebo);
	glGenVertexArrays(1, &dev->vao);

	glBindVertexArray(dev->vao);
	glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

	glEnableVertexAttribArray((GLuint)dev->attrib_pos);
	glEnableVertexAttribArray((GLuint)dev->attrib_uv);
	glEnableVertexAttribArray((GLuint)dev->attrib_col);

	glVertexAttribPointer((GLuint)dev->attrib_pos, 2, GL_FLOAT, GL_FALSE, vs, (void*)vp);
	glVertexAttribPointer((GLuint)dev->attrib_uv, 2, GL_FLOAT, GL_FALSE, vs, (void*)vt);
	glVertexAttribPointer((GLuint)dev->attrib_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, vs, (void*)vc);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

    return true;
}

UserEvent::UserEvent()
{
	m_event_idx = 0;
	NkWindow::get()->register_user_event(this);
}

UserEvent::~UserEvent()
{
	if (m_event_idx != -1)
		NkWindow::get()->unregister_user_event(this);
}

void UserEvent::push(int code, void* data1, void* data2)
{
	NkWindowGLX* glxw = (NkWindowGLX*)NkWindow::get();
	m_event_idx = code;
	m_userdata1 = data1;
	m_userdata2 = data2;
	glxw->push_user_event(this);
}
