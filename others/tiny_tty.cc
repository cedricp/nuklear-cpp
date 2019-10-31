#include "tiny_tty.h"
#include <string>

inline int max(int a, int b) {
	return ((a) > (b) ? (a) : (b));
}
inline int min(int a, int b) {
	return ((a) < (b) ? (a) : (b));
}

inline int isdigit(char c){
	if (c > 47 && c < 58)
		return true;
	return false;
}

void Tiny_tty::render() {
	// expose the cursor key mode state
	m_cursor_key_mode_application = m_state.cursor_key_mode_application;

	// if scrolling, prepare the "recycled" screen area
	if (m_state.top_row != m_rendered.top_row) {
		// clear the new piece of screen to be recycled as blank space
		// @todo handle scroll-up
		if (m_state.top_row > m_rendered.top_row) {
			// pre-clear the lines at the bottom
			// @todo always use black instead of current background colour?
			// @todo deal with overflow from multiplication by m_display->m_char_height
			int16_t old_bottom_y = m_rendered.top_row * m_display->m_char_height
					+ m_display->m_screen_row_count * m_display->m_char_height; // bottom of text may not align with screen height
			int16_t new_bottom_y = m_state.top_row * m_display->m_char_height
					+ m_display->m_screen_height; // extend to bottom edge of new displayed area
			int16_t clear_sbuf_bottom = new_bottom_y
					% m_display->m_screen_height;
			int16_t clear_height = min(m_display->m_screen_height,
					new_bottom_y - old_bottom_y);
			int16_t clear_sbuf_top = clear_sbuf_bottom - clear_height;

			// if rectangle straddles the screen buffer top edge, render that slice at bottom edge
			if (clear_sbuf_top < 0) {
				m_display->fill_rect(0,
						clear_sbuf_top + m_display->m_screen_height,
						m_display->m_screen_width, -clear_sbuf_top,
						ANSI_COLORS[m_state.bg_ansi_color]);
			}

			// if rectangle is not entirely above top edge, render the normal slice
			if (clear_sbuf_bottom > 0) {
				m_display->fill_rect(0, max(0, clear_sbuf_top),
						m_display->m_screen_width,
						clear_sbuf_bottom - max(0, clear_sbuf_top),
						ANSI_COLORS[m_state.bg_ansi_color]);
			}
		}

		// update displayed scroll
		m_display->set_vscroll(
				(m_state.top_row * m_display->m_char_height)
						% m_display->m_screen_height); // @todo deal with overflow from multiplication

		// save rendered state
		m_rendered.top_row = m_state.top_row;
	}

	// render character if needed
	if (m_state.out_char != 0) {
		const uint16_t x = m_state.out_char_col * m_display->m_char_width;
		const uint16_t y = (m_state.out_char_row * m_display->m_char_height)
				% m_display->m_screen_height; // @todo deal with overflow from multiplication
		const uint32_t fg_tft_color =
				m_state.bold ?
						ANSI_BOLD_COLORS[m_state.fg_ansi_color] :
						ANSI_COLORS[m_state.fg_ansi_color];
		const uint32_t bg_tft_color = ANSI_COLORS[m_state.bg_ansi_color];

		uint8_t char_set = m_state.g4bank_char_set[m_state.out_char_g4bank & 0x03]; // ensure 0-3 value

		if (1) {
			m_display->draw_character(m_state.out_char & 0x7f, char_set, bg_tft_color, fg_tft_color, x, y);
		}

		// line-before
		// @todo detect when straddling edge of buffer
		if (m_state.out_clear_before > 0) {
			const int16_t line_before_chars = min(m_state.out_char_col,
					m_state.out_clear_before);
			const int16_t lines_before = (m_state.out_clear_before
					- line_before_chars) / m_display->m_screen_col_count;

			m_display->fill_rect(
					(m_state.out_char_col - line_before_chars)
							* m_display->m_char_width,
					(m_state.out_char_row * m_display->m_char_height)
							% m_display->m_screen_height, // @todo deal with overflow from multiplication
					line_before_chars * m_display->m_char_width,
					m_display->m_char_height,
					ANSI_COLORS[m_state.bg_ansi_color]);

			for (int16_t i = 0; i < lines_before; i += 1) {
				m_display->fill_rect(0,
						((m_state.out_char_row - 1 - i)
								* m_display->m_char_height)
								% m_display->m_screen_height, // @todo deal with overflow from multiplication
						m_display->m_screen_width, m_display->m_char_height,
						ANSI_COLORS[m_state.bg_ansi_color]);
			}
		}

		// line-after
		// @todo detect when straddling edge of buffer
		if (m_state.out_clear_after > 0) {
			const int16_t line_after_chars = min(
					m_display->m_screen_col_count - 1 - m_state.out_char_col,
					m_state.out_clear_after);
			const int16_t lines_after = (m_state.out_clear_after
					- line_after_chars) / m_display->m_screen_col_count;

			m_display->fill_rect(
					(m_state.out_char_col + 1) * m_display->m_char_width,
					(m_state.out_char_row * m_display->m_char_height)
							% m_display->m_screen_height, // @todo deal with overflow from multiplication
					line_after_chars * m_display->m_char_width,
					m_display->m_char_height,
					ANSI_COLORS[m_state.bg_ansi_color]);

			for (int16_t i = 0; i < lines_after; i += 1) {
				m_display->fill_rect(0,
						((m_state.out_char_row + 1 + i)
								* m_display->m_char_height)
								% m_display->m_screen_height, // @todo deal with overflow from multiplication
						m_display->m_screen_width, m_display->m_char_height,
						ANSI_COLORS[m_state.bg_ansi_color]);
			}
		}

		// clear for next render
		m_state.out_char = 0;
		m_state.out_clear_before = 0;
		m_state.out_clear_after = 0;

		// the char draw may overpaint the cursor, in which case
		// mark it for repaint
		if (m_rendered.cursor_col == m_state.out_char_col
				&& m_rendered.cursor_row == m_state.out_char_row) {
			m_rendered.cursor_col = -1;
		}
	}

	// reflect new cursor bar render state
	const bool cursor_bar_shown = (!m_state.cursor_hidden
			&& m_state.idle_cycle_count < IDLE_CYCLE_ON);

	// clear existing rendered cursor bar if needed
	// @todo detect if it is already cleared during scroll
	if (m_rendered.cursor_col >= 0) {
		if (!cursor_bar_shown || m_rendered.cursor_col != m_state.cursor_col
				|| m_rendered.cursor_row != m_state.cursor_row) {
			m_display->fill_rect(
					m_rendered.cursor_col * m_display->m_char_width,
					(m_rendered.cursor_row * m_display->m_char_height
							+ m_display->m_char_height - 1)
							% m_display->m_screen_height, // @todo deal with overflow from multiplication
					m_display->m_char_width, 1,
					ANSI_COLORS[m_state.bg_ansi_color] // @todo save the original background colour or even pixel values
					);

			// record the fact that cursor bar is not on screen
			m_rendered.cursor_col = -1;
		}
	}

	// render new cursor bar if not already shown
	// (sometimes right after clearing existing bar)
	if (m_rendered.cursor_col < 0) {
		if (cursor_bar_shown) {
			m_display->fill_rect(m_state.cursor_col * m_display->m_char_width,
					(m_state.cursor_row * m_display->m_char_height
							+ m_display->m_char_height - 1)
							% m_display->m_screen_height, // @todo deal with overflow from multiplication
					m_display->m_char_width, 1,
					m_state.bold ?
							ANSI_BOLD_COLORS[m_state.fg_ansi_color] :
							ANSI_COLORS[m_state.fg_ansi_color]);

			// save new rendered state
			m_rendered.cursor_col = m_state.cursor_col;
			m_rendered.cursor_row = m_state.cursor_row;
		}
	}
}

void Tiny_tty::ensure_cursor_vscroll() {
	// move displayed window down to cover cursor
	// @todo support scrolling up as well
	if (m_state.cursor_row - m_state.top_row >= m_display->m_screen_row_count) {
		m_state.top_row = m_state.cursor_row - m_display->m_screen_row_count
				+ 1;
	}
}

void Tiny_tty::send_sequence(char* str) {
	// send zero-terminated sequence character by character
	while (*str) {
		send_char(*str);
		str += 1;
	}
}

char Tiny_tty::read_decimal() {
	uint16_t accumulator = 0;

	while (isdigit(peek_char())) {
		const char digit_character = read_char();
		const uint16_t digit = digit_character - '0';
		accumulator = accumulator * 10 + digit;
	}

	return accumulator;
}

void Tiny_tty::apply_graphic_rendition(uint16_t* arg_list, uint16_t arg_count) {
	if (arg_count == 0) {
		// special case for resetting to default style
		m_state.bg_ansi_color = 0;
		m_state.fg_ansi_color = 7;
		m_state.bold = false;

		return;
	}

	// process commands
	// @todo support bold/etc for better colour support
	// @todo 39/49?
	for (uint16_t arg_index = 0; arg_index < arg_count; arg_index += 1) {
		const uint16_t arg_value = arg_list[arg_index];

		if (arg_value == 0) {
			// reset to default style
			m_state.bg_ansi_color = 0;
			m_state.fg_ansi_color = 7;
			m_state.bold = false;
		} else if (arg_value == 1) {
			// bold
			m_state.bold = true;
		} else if (arg_value >= 30 && arg_value <= 37) {
			// foreground ANSI colour
			m_state.fg_ansi_color = arg_value - 30;
		} else if (arg_value >= 40 && arg_value <= 47) {
			// background ANSI colour
			m_state.bg_ansi_color = arg_value - 40;
		}
	}
}

void Tiny_tty::apply_mode_setting(bool mode_on, uint16_t* arg_list,
		uint16_t arg_count) {
	// process modes
	for (uint16_t arg_index = 0; arg_index < arg_count; arg_index += 1) {
		const uint16_t mode_id = arg_list[arg_index];

		switch (mode_id) {
		case 4:
			// insert/replace mode
			// @todo this should be off for most practical purposes anyway?
			// ... otherwise visually shifting line text is expensive
			break;

		case 20:
			// auto-LF
			// ignoring per http://vt100.net/docs/vt220-rm/chapter4.html section 4.6.6
			break;

		case 34:
			// cursor visibility
			m_state.cursor_hidden = !mode_on;
			break;
		}
	}
}

void Tiny_tty::exec_escape_question_command() {
	// @todo support multiple mode commands
	// per http://vt100.net/docs/vt220-rm/chapter4.html section 4.6.1,
	// ANSI and DEC modes cannot mix; that is, '[?25;20;?7l' is not a valid Esc-command
	// (noting this because https://www.gnu.org/software/screen/manual/html_node/Control-Sequences.html
	// makes it look like the question mark is a prefix)
	const uint16_t mode = read_decimal();
	const bool mode_on = (read_char() != 'l');

	switch (mode) {
	case 1:
		// cursor key mode (normal/application)
		m_state.cursor_key_mode_application = mode_on;
		break;

	case 7:
		// auto wrap mode
		m_state.no_wrap = !mode_on;
		break;

	case 25:
		// cursor visibility
		m_state.cursor_hidden = !mode_on;
		break;
	}
}

// @todo cursor position report
void Tiny_tty::exec_escape_bracket_command_with_args(uint16_t* arg_list,
		uint16_t arg_count) {
	// convenient arg getter
#define ARG(index, default_value) (arg_count > index ? arg_list[index] : default_value)

	// process next character after Escape-code, bracket and any numeric arguments
	const char command_character = read_char();

	switch (command_character) {
	case '?':
		// question-mark commands
		exec_escape_question_command();
		break;

	case 'A':
		// cursor up (no scroll)
		m_state.cursor_row = max(m_state.top_row,
				m_state.cursor_row - ARG(0, 1));
		break;

	case 'B':
		// cursor down (no scroll)
		m_state.cursor_row = min(
				m_state.top_row + m_display->m_screen_row_count - 1,
				m_state.cursor_row + ARG(0, 1));
		break;

	case 'C':
		// cursor right (no scroll)
		m_state.cursor_col = min(m_display->m_screen_col_count - 1,
				m_state.cursor_col + ARG(0, 1));
		break;

	case 'D':
		// cursor left (no scroll)
		m_state.cursor_col = max(0, m_state.cursor_col - ARG(0, 1));
		break;

	case 'H':
	case 'f':
		// Direct Cursor Addressing (row;col)
		m_state.cursor_col = max(0,
				min(m_display->m_screen_col_count - 1, ARG(1, 1) - 1));
		m_state.cursor_row = m_state.top_row
				+ max(0, min(m_display->m_screen_row_count - 1, ARG(0, 1) - 1));
		break;

	case 'J':
		// clear screen
		m_state.out_char = ' ';
		m_state.out_char_col = m_state.cursor_col;
		m_state.out_char_row = m_state.cursor_row;

		{
			const int16_t rel_row = m_state.cursor_row - m_state.top_row;

			m_state.out_clear_before =
					ARG(0, 0) != 0 ?
							rel_row * m_display->m_screen_col_count
									+ m_state.cursor_col :
							0;
			m_state.out_clear_after =
					ARG(0, 0) != 1 ?
							(m_display->m_screen_row_count - 1 - rel_row)
									* m_display->m_screen_col_count
									+ (m_display->m_screen_col_count - 1
											- m_state.cursor_col) :
							0;
		}

		break;

	case 'K':
		// clear line
		m_state.out_char = ' ';
		m_state.out_char_col = m_state.cursor_col;
		m_state.out_char_row = m_state.cursor_row;

		m_state.out_clear_before = ARG(0, 0) != 0 ? m_state.cursor_col : 0;
		m_state.out_clear_after =
				ARG(0, 0) != 1 ?
						m_display->m_screen_col_count - 1 - m_state.cursor_col :
						0;

		break;

	case 'm':
		// graphic rendition mode
		apply_graphic_rendition(arg_list, arg_count);
		break;

	case 'h':
		// set mode
		apply_mode_setting(true, arg_list, arg_count);
		break;

	case 'l':
		// unset mode
		apply_mode_setting(false, arg_list, arg_count);
		break;
	}
}

void Tiny_tty::exec_escape_bracket_command() {
	const uint16_t MAX_COMMAND_ARG_COUNT = 10;
	uint16_t arg_list[MAX_COMMAND_ARG_COUNT];
	uint16_t arg_count = 0;

	// start parsing arguments if any
	// (this means that '' is treated as no arguments, but '0;' is treated as two arguments, each being zero)
	// @todo ignore trailing semi-colon instead of treating it as marking an extra zero arg?
	if (isdigit(peek_char())) {
		// keep consuming arguments while we have space
		while (arg_count < MAX_COMMAND_ARG_COUNT) {
			// consume decimal number
			arg_list[arg_count] = read_decimal();
			arg_count += 1;

			// stop processing if next char is not separator
			if (peek_char() != ';') {
				break;
			}

			// consume separator before starting next argument
			read_char();
		}
	}

	exec_escape_bracket_command_with_args(arg_list, arg_count);
}

// set the characters displayed for given G0-G3 bank
void Tiny_tty::exec_character_set(uint8_t g4bank_index) {
	switch (read_char()) {
	case 'A':
	case 'B':
		// normal character set (UK/US)
		m_state.g4bank_char_set[g4bank_index] = 0;
		break;

	case '0':
		// line-drawing
		m_state.g4bank_char_set[g4bank_index] = 1;
		break;

	default:
		// alternate sets are unsupported
		m_state.g4bank_char_set[g4bank_index] = 0;
		break;
	}
}

// @todo terminal reset
// @todo parse modes with arguments even if they are no-op
void Tiny_tty::exec_escape_code() {
	// read next character after Escape-code
	// @todo time out?
	char esc_character = read_char();

	// @todo support for (, ), #, c, cursor save/restore
	switch (esc_character) {
	case '[':
		exec_escape_bracket_command();
		break;

	case 'D':
		// index (move down and possibly scroll)
		m_state.cursor_row += 1;
		ensure_cursor_vscroll();
		break;

	case 'M':
		// reverse index (move up and possibly scroll)
		m_state.cursor_row -= 1;
		ensure_cursor_vscroll();
		break;

	case 'E':
		// next line
		m_state.cursor_row += 1;
		m_state.cursor_col = 0;
		ensure_cursor_vscroll();
		break;

	case 'Z':
		// Identify Terminal (DEC Private)
		send_sequence("\e[?1;0c"); // DA response: no options
		break;

	case '7':
		// save cursor
		// @todo verify that the screen-relative coordinate approach is valid
		m_state.dec_saved_col = m_state.cursor_col;
		m_state.dec_saved_row = m_state.cursor_row - m_state.top_row; // relative to top
		m_state.dec_saved_bg = m_state.bg_ansi_color;
		m_state.dec_saved_fg = m_state.fg_ansi_color;
		m_state.dec_saved_g4bank = m_state.out_char_g4bank;
		m_state.dec_saved_bold = m_state.bold;
		m_state.dec_saved_no_wrap = m_state.no_wrap;
		break;

	case '8':
		// restore cursor
		m_state.cursor_col = m_state.dec_saved_col;
		m_state.cursor_row = m_state.dec_saved_row + m_state.top_row; // relative to top
		m_state.bg_ansi_color = m_state.dec_saved_bg;
		m_state.fg_ansi_color = m_state.dec_saved_fg;
		m_state.out_char_g4bank = m_state.dec_saved_g4bank;
		m_state.bold = m_state.dec_saved_bold;
		m_state.no_wrap = m_state.dec_saved_no_wrap;
		break;

	case '=':
	case '>':
		// keypad mode setting - ignoring
		break;

	case '(':
		// set G0
		exec_character_set(0);
		break;

	case ')':
		// set G1
		exec_character_set(1);
		break;

	case '*':
		// set G2
		exec_character_set(2);
		break;

	case '+':
		// set G3
		exec_character_set(3);
		break;

	default:
		// unrecognized character, silently ignore
		break;
	}
}

/*
 * Returns true while an input char has been processed
 */
bool Tiny_tty::process() {
	// start in default idle state
	char initial_character = read_char();

	if (initial_character == 0) {
		return false;
	}

	if (initial_character >= 0x20 && initial_character <= 0x7e) {
		// output displayable character
		m_state.out_char = initial_character;
		m_state.out_char_col = m_state.cursor_col;
		m_state.out_char_row = m_state.cursor_row;

		// update caret
		m_state.cursor_col += 1;

		if (m_state.cursor_col >= m_display->m_screen_col_count) {
			if (m_state.no_wrap) {
				m_state.cursor_col = m_display->m_screen_col_count - 1;
			} else {
				m_state.cursor_col = 0;
				m_state.cursor_row += 1;
				ensure_cursor_vscroll();
			}
		}

		// reset idle state
		m_state.idle_cycle_count = 0;
	} else {
		// @todo bell, answer-back (0x05), delete
		switch (initial_character) {
		case '\n':
			// line-feed
			m_state.cursor_row += 1;
			ensure_cursor_vscroll();
			break;

		case '\r':
			// carriage-return
			m_state.cursor_col = 0;
			break;

		case '\b':
			// backspace
			m_state.cursor_col -= 1;

			if (m_state.cursor_col < 0) {
				if (m_state.no_wrap) {
					m_state.cursor_col = 0;
				} else {
					m_state.cursor_col = m_display->m_screen_col_count - 1;
					m_state.cursor_row -= 1;
					ensure_cursor_vscroll();
				}
			}

			break;

		case '\t':
			// tab
		{
			// @todo blank out the existing characters? not sure if that is expected
			const int16_t tab_num = m_state.cursor_col / TAB_SIZE;
			m_state.cursor_col = min(m_display->m_screen_col_count - 1,
					(tab_num + 1) * TAB_SIZE);
		}
			break;

		case '\e':
			// Escape-command
			exec_escape_code();
			break;

		case '\x0f':
			// Shift-In (use G0)
			// see also the fun reason why these are called this way:
			// https://en.wikipedia.org/wiki/Shift_Out_and_Shift_In_characters
			m_state.out_char_g4bank = 0;
			break;

		case '\x0e':
			// Shift-Out (use G1)
			m_state.out_char_g4bank = 1;
			break;

		default:
			// nothing, just animate cursor
			m_state.idle_cycle_count = (m_state.idle_cycle_count + 1)
					% IDLE_CYCLE_MAX;
		}
	}

	render();
	return true;
}

void Tiny_tty::init() {
	// set up initial state
	m_state.cursor_col = 0;
	m_state.cursor_row = 0;
	m_state.top_row = 0;
	m_state.no_wrap = 0;
	m_state.cursor_hidden = 0;
	m_state.bg_ansi_color = 0;
	m_state.fg_ansi_color = 7;
	m_state.bold = false;

	m_state.cursor_key_mode_application = false;

	m_state.dec_saved_col = 0;
	m_state.dec_saved_row = 0;
	m_state.dec_saved_bg = m_state.bg_ansi_color;
	m_state.dec_saved_fg = m_state.fg_ansi_color;
	m_state.dec_saved_g4bank = 0;
	m_state.dec_saved_bold = m_state.bold;
	m_state.dec_saved_no_wrap = false;

	m_state.out_char = 0;
	m_state.out_char_g4bank = 0;
	m_state.g4bank_char_set[0] = 0;
	m_state.g4bank_char_set[1] = 0;
	m_state.g4bank_char_set[2] = 0;
	m_state.g4bank_char_set[3] = 0;

	m_rendered.cursor_col = -1;
	m_rendered.cursor_row = -1;

	// clear screen
	m_display->fill_rect(0, 0, m_display->m_screen_width,
			m_display->m_screen_height, TFT_BLACK);

	// reset TFT scroll to default
	m_display->set_vscroll(0);

	// initial render
	render();

	// send CR to indicate that the screen is ready
	// (this works with the agetty --wait-cr option to help wait until Arduino boots)
	send_char('\r');
}

static char test_buffer[] = "-- \e[1mTinyTTY\e[m --\r\nHello World\nTest machine\r\n>>";
static int current_buffer_pos = 0;

void Tiny_tty::tintty_idle() {
	// animate cursor
	m_state.idle_cycle_count = (m_state.idle_cycle_count + 1) % IDLE_CYCLE_MAX;
	// re-render
	render();
}

char Tiny_tty::peek_char() {
	return test_buffer[current_buffer_pos];
}

char Tiny_tty::read_char() {
	if (test_buffer[current_buffer_pos])
		return test_buffer[current_buffer_pos++];
	return 0;
}

void Tiny_tty::send_char(char ch) {
	printf("%c", ch);
}
