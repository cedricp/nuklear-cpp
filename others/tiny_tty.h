#ifndef TINY_TTY_H
#define TINY_TTY_H

#include <stdint.h>
#include <sys/types.h>

#define TFT_BLACK   0x000000FF
#define TFT_BLUE    0x0000FFFF
#define TFT_RED     0xFF0000FF
#define TFT_GREEN   0x00FF00FF
#define TFT_CYAN    0x00FFFFFF
#define TFT_MAGENTA 0xFF00FFFF
#define TFT_YELLOW  0xFFFF00FF
#define TFT_WHITE   0xFFFFFFFF

#define TFT_BOLD_BLACK   0x000000FF
#define TFT_BOLD_BLUE    0x0000FFFF
#define TFT_BOLD_RED     0xFF0000FF
#define TFT_BOLD_GREEN   0x00FF00FF
#define TFT_BOLD_CYAN    0x00FFFFFF
#define TFT_BOLD_MAGENTA 0xFF00FFFF
#define TFT_BOLD_YELLOW  0xFFFF00FF
#define TFT_BOLD_WHITE   0xFFFFFFFF

const uint32_t ANSI_COLORS[] = {
    TFT_BLACK,
    TFT_RED,
    TFT_GREEN,
    TFT_YELLOW,
    TFT_BLUE,
    TFT_MAGENTA,
    TFT_CYAN,
    TFT_WHITE
};

const uint32_t ANSI_BOLD_COLORS[] = {
    TFT_BOLD_BLACK,
    TFT_BOLD_RED,
    TFT_BOLD_GREEN,
    TFT_BOLD_YELLOW,
    TFT_BOLD_BLUE,
    TFT_BOLD_MAGENTA,
    TFT_BOLD_CYAN,
    TFT_BOLD_WHITE
};

const int16_t IDLE_CYCLE_MAX = 25000;
const int16_t IDLE_CYCLE_ON = 12500;

const int16_t TAB_SIZE = 4;

// cursor and character position is in global buffer coordinate space (may exceed screen height)
struct tintty_state {
    // @todo consider storing cursor position as single int offset
    int16_t cursor_col, cursor_row;
    uint16_t bg_ansi_color, fg_ansi_color;
    bool bold;

    // cursor mode
    bool cursor_key_mode_application;

    // saved DEC cursor info (in screen coords)
    int16_t dec_saved_col, dec_saved_row, dec_saved_bg, dec_saved_fg;
    uint8_t dec_saved_g4bank;
    bool dec_saved_bold, dec_saved_no_wrap;

    // @todo deal with integer overflow
    int16_t top_row; // first displayed row in a logical scrollback buffer
    bool no_wrap;
    bool cursor_hidden;

    char out_char;
    int16_t out_char_col, out_char_row;
    uint8_t out_char_g4bank; // current set shift state, G0 to G3
    int16_t out_clear_before, out_clear_after;

    uint8_t g4bank_char_set[4];

    int16_t idle_cycle_count; // @todo track during blocking reads mid-command
};

struct tintty_rendered {
    int16_t cursor_col, cursor_row;
    int16_t top_row;
} ;

class Tinytty_display {
protected:
    int16_t m_screen_width, m_screen_height;
    int16_t m_screen_col_count, m_screen_row_count;
	int m_char_width, m_char_height;
public:
    Tinytty_display(int screen_width, int screen_height, int char_width, int char_height){
		m_screen_width = screen_width;
		m_screen_height = screen_height;
		m_screen_col_count = screen_width / char_width;
		m_screen_row_count = screen_height / char_height;
		m_char_width = char_width;
		m_char_height = char_height;
	}
    virtual ~Tinytty_display(){}

    virtual void fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color) = 0;
    virtual void draw_pixels(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t *pixels) = 0;
    virtual void set_vscroll(int16_t offset) = 0;
    virtual void draw_character(char c, unsigned char char_set, uint32_t bg_col, uint32_t fg_col, int x, int y) = 0;

    friend class Tiny_tty;
};

class Tiny_tty {
protected:
	tintty_rendered m_rendered;
	tintty_state m_state;
	Tinytty_display* m_display;
	bool m_cursor_key_mode_application;

private:
	void ensure_cursor_vscroll();
	void send_sequence(char* str);
	char read_decimal();
	void apply_graphic_rendition(uint16_t* arg_list, uint16_t arg_count);
	void apply_mode_setting(bool mode_on, uint16_t* arg_list, uint16_t arg_count);
	void exec_escape_question_command();
	void exec_escape_bracket_command_with_args(uint16_t* arg_list, uint16_t arg_count);
	void exec_escape_bracket_command();
	void exec_escape_code();
	void exec_character_set( uint8_t g4bank_index);
public:
	Tiny_tty(Tinytty_display* display){
		m_display = display;
		m_cursor_key_mode_application = true;

	}
	void set_cursor_key_mode(bool b){
		m_cursor_key_mode_application = b;
	}
	void render();
	bool process();
	void init();
	void tintty_idle();

	// Must return next char in the buffer
	virtual char peek_char();
	// Must return next char in the buffer and remove it from buffer
	virtual char read_char();
	// Sends a char
	virtual void send_char(char ch);
};

#endif
