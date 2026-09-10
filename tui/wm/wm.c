#include <stdint.h>
#include "wm.h"
#include "keyboard.h"
#include "mouse.h"
#include "auth.h"
#include "ext2.h"

typedef void (*pwm_command_handler_t)(const char *, void (*)(const char *), void (*)(char));
typedef struct { const char *name; pwm_command_handler_t handler; } command_t;

#define VGA_MEMORY 0xB8000
#define PWM_MAX_WINDOWS 4
#define PWM_MAX_LINES 32
#define PWM_MAX_LINE_LENGTH 80
#define PWM_MAX_INPUT 80
#define PWM_MAX_HISTORY 16
#define PWM_WIDTH 80
#define PWM_HEIGHT 24

static uint16_t *vga_buffer = (uint16_t *)VGA_MEMORY;
static int active_window;
static int window_count;
static int pwm_running;
static int pwm_stop_requested;

extern ext2_filesystem_t fs;
extern command_t *find_command(const char *name);
extern int strlen(const char *text);

static struct {
    char title[16];
    char lines[PWM_MAX_LINES][PWM_MAX_LINE_LENGTH];
    int line_count;
    char input[PWM_MAX_INPUT];
    int input_length;
    int scroll_offset;
    char history[PWM_MAX_HISTORY][PWM_MAX_INPUT];
    int history_count;
    int history_position;
} terminals[PWM_MAX_WINDOWS];

static void window_geometry(int terminal, int *left, int *top, int *width, int *height) {
    int half_width = PWM_WIDTH / 2;
    int half_height = PWM_HEIGHT / 2;
    if (window_count == 1) { *left = 0; *top = 0; *width = PWM_WIDTH; *height = PWM_HEIGHT; }
    else if (window_count == 2) { *left = terminal * half_width; *top = 0; *width = half_width; *height = PWM_HEIGHT; }
    else if (window_count == 3 && terminal == 0) { *left = 0; *top = 0; *width = half_width; *height = PWM_HEIGHT; }
    else if (window_count == 3) { *left = half_width; *top = (terminal - 1) * half_height; *width = PWM_WIDTH - half_width; *height = terminal == 1 ? half_height : PWM_HEIGHT - half_height; }
    else { *left = (terminal % 2) * half_width; *top = (terminal / 2) * half_height; *width = half_width; *height = terminal / 2 == 0 ? half_height : PWM_HEIGHT - half_height; }
}

static void copy_text(char *destination, const char *source, int limit) {
    int index = 0;
    while (source[index] && index < limit - 1) destination[index] = source[index], index++;
    destination[index] = '\0';
}

static void push_line(int terminal, const char *text) {
    if (terminals[terminal].line_count == PWM_MAX_LINES) {
        for (int i = 1; i < PWM_MAX_LINES; i++) copy_text(terminals[terminal].lines[i - 1], terminals[terminal].lines[i], PWM_MAX_LINE_LENGTH);
        terminals[terminal].line_count--;
    }
    copy_text(terminals[terminal].lines[terminals[terminal].line_count++], text, PWM_MAX_LINE_LENGTH);
}

static void append_line(int terminal, const char *text) {
    char line[PWM_MAX_LINE_LENGTH];
    int start = 0;
    int ended_with_newline = 0;
    int left;
    int top;
    int width;
    int height;
    window_geometry(terminal, &left, &top, &width, &height);
    (void)left;
    (void)top;
    (void)height;
    int line_width = width - 2;
    if (!text[0]) { push_line(terminal, ""); return; }
    while (text[start]) {
        int length = 0;
        while (text[start] && text[start] != '\n' && length < line_width && length < PWM_MAX_LINE_LENGTH - 1) line[length++] = text[start++];
        line[length] = '\0';
        push_line(terminal, line);
        if (text[start] == '\n') {
            start++;
            ended_with_newline = text[start] == '\0';
        } else {
            ended_with_newline = 0;
        }
    }
    if (ended_with_newline) push_line(terminal, "");
}

static void terminal_putc(char character) {
    int terminal = active_window;
    if (character == '\n') { append_line(terminal, ""); return; }
    if (character == '\r') return;
    if (character == '\b') {
        if (terminals[terminal].line_count > 0) {
            int line = terminals[terminal].line_count - 1;
            int length = strlen(terminals[terminal].lines[line]);
            if (length > 0) terminals[terminal].lines[line][length - 1] = '\0';
        }
        return;
    }
    if (terminals[terminal].line_count == 0) append_line(terminal, "");
    {
        int line = terminals[terminal].line_count - 1;
        int length = strlen(terminals[terminal].lines[line]);
        int left, top, width, height;
        window_geometry(terminal, &left, &top, &width, &height);
        int columns = width - 2;
        if (length < columns && length < PWM_MAX_LINE_LENGTH - 1) {
            terminals[terminal].lines[line][length] = character;
            terminals[terminal].lines[line][length + 1] = '\0';
            terminals[terminal].scroll_offset = 0;
        } else {
            char text[2] = { character, '\0' };
            append_line(terminal, text);
        }
    }
}

static void terminal_puts(const char *text) { for (int i = 0; text[i]; i++) terminal_putc(text[i]); }

static void replace_input(const char *text) {
    while (terminals[active_window].input_length > 0) {
        terminals[active_window].input_length--;
        terminal_putc('\b');
    }
    copy_text(terminals[active_window].input, text, PWM_MAX_INPUT);
    terminals[active_window].input_length = strlen(terminals[active_window].input);
    for (int i = 0; i < terminals[active_window].input_length; i++) terminal_putc(terminals[active_window].input[i]);
}

static void save_history(int terminal) {
    if (terminals[terminal].input_length == 0) return;
    if (terminals[terminal].history_count < PWM_MAX_HISTORY) {
        copy_text(terminals[terminal].history[terminals[terminal].history_count], terminals[terminal].input, PWM_MAX_INPUT);
        terminals[terminal].history_count++;
    } else {
        for (int i = 1; i < PWM_MAX_HISTORY; i++) copy_text(terminals[terminal].history[i - 1], terminals[terminal].history[i], PWM_MAX_INPUT);
        copy_text(terminals[terminal].history[PWM_MAX_HISTORY - 1], terminals[terminal].input, PWM_MAX_INPUT);
    }
    terminals[terminal].history_position = terminals[terminal].history_count;
}

static void fill_screen(void) {
    for (int row = 0; row < PWM_HEIGHT; row++) for (int column = 0; column < PWM_WIDTH; column++)
        vga_buffer[row * PWM_WIDTH + column] = (0x07 << 8) | ' ';
}

static void draw_window(int terminal, int left, int top, int width, int height) {
    int content_height = height - 2;
    int first_line = terminals[terminal].line_count - content_height - terminals[terminal].scroll_offset;
    uint8_t title_color = active_window == terminal ? 0x1F : 0x17;
    if (first_line < 0) first_line = 0;
    for (int row = 0; row < height; row++) for (int column = 0; column < width; column++) {
        char character = ' ';
        uint8_t color = active_window == terminal &&
                (row == 0 || row == height - 1 || column == 0 || column == width - 1) ? 0x1F : 0x07;
        if (row == 0 || row == height - 1) character = '-';
        if (column == 0 || column == width - 1) character = '|';
        if ((row == 0 || row == height - 1) && (column == 0 || column == width - 1)) character = '+';
        if (row == 1 && column > 0 && column < width - 1) color = title_color;
        vga_buffer[(top + row) * PWM_WIDTH + left + column] = (color << 8) | character;
    }
    for (int index = 0; index < width - 2 && index < 13; index++) {
        char character = index == 0 ? (active_window == terminal ? '*' : ' ') :
                         (index == 1 ? ' ' : terminals[terminal].title[index - 2]);
        vga_buffer[(top + 1) * PWM_WIDTH + left + index + 1] = (title_color << 8) | character;
    }
    for (int row = 0; row < content_height; row++) {
        int line_index = first_line + row;
        if (line_index < terminals[terminal].line_count) {
            int length = strlen(terminals[terminal].lines[line_index]);
            for (int column = 0; column < width - 2 && column < length; column++)
                vga_buffer[(top + row + 2) * PWM_WIDTH + left + column + 1] = (0x07 << 8) | terminals[terminal].lines[line_index][column];
        }
    }
}

static void draw_status(void) {
    const char *user = auth_current_username()[0] ? auth_current_username() : "piux";
    const char *text = " pWM | ";
    int column = 0;
    for (int i = 0; text[i] && column < PWM_WIDTH; i++) vga_buffer[PWM_HEIGHT * PWM_WIDTH + column++] = (0x70 << 8) | text[i];
    for (int i = 0; user[i] && column < PWM_WIDTH; i++) vga_buffer[PWM_HEIGHT * PWM_WIDTH + column++] = (0x70 << 8) | user[i];
    text = " | Ctrl+Q new | Ctrl+C close | Ctrl+Arrows focus";
    for (int i = 0; text[i] && column < PWM_WIDTH; i++) vga_buffer[PWM_HEIGHT * PWM_WIDTH + column++] = (0x70 << 8) | text[i];
    while (column < PWM_WIDTH) vga_buffer[PWM_HEIGHT * PWM_WIDTH + column++] = (0x70 << 8) | ' ';
}

static void draw_desktop(void) {
    int left = PWM_WIDTH / 2;
    int right = PWM_WIDTH - left;
    int half = PWM_HEIGHT / 2;
    fill_screen();
    if (window_count == 1) draw_window(0, 0, 0, PWM_WIDTH, PWM_HEIGHT);
    else if (window_count == 2) {
        draw_window(0, 0, 0, left, PWM_HEIGHT);
        draw_window(1, left, 0, right, PWM_HEIGHT);
    } else if (window_count == 3) {
        draw_window(0, 0, 0, left, PWM_HEIGHT);
        draw_window(1, left, 0, right, half);
        draw_window(2, left, half, right, PWM_HEIGHT - half);
    } else {
        draw_window(0, 0, 0, left, half);
        draw_window(1, left, 0, right, half);
        draw_window(2, 0, half, left, PWM_HEIGHT - half);
        draw_window(3, left, half, right, PWM_HEIGHT - half);
    }
    draw_status();
}

static int handle_mouse(const mouse_event_t *event) {
    if (event->wheel == 0) return 0;
    terminals[active_window].scroll_offset += event->wheel > 0 ? -2 : 2;
    if (terminals[active_window].scroll_offset < 0) terminals[active_window].scroll_offset = 0;
    if (terminals[active_window].scroll_offset > terminals[active_window].line_count) {
        terminals[active_window].scroll_offset = terminals[active_window].line_count;
    }
    return 1;
}

int pwm_command_read_char(void) {
    if (!pwm_running) return keyboard_read_char();
    while (1) {
        mouse_event_t event;
        while (mouse_poll(&event)) {
            if (handle_mouse(&event)) draw_desktop();
        }
        int key = keyboard_try_read_char();
        if (key >= 0) {
            draw_desktop();
            return key;
        }
    }
}

int pwm_command_clear(void) {
    if (!pwm_running) return 0;
    terminals[active_window].line_count = 0;
    terminals[active_window].input_length = 0;
    terminals[active_window].scroll_offset = 0;
    return 1;
}

int pwm_command_is_active(void) {
    return pwm_running;
}

int pwm_command_reload(void) {
    if (!pwm_running) return 0;
    terminals[active_window].scroll_offset = 0;
    return 1;
}

int pwm_command_stop(void) {
    if (!pwm_running) return 0;
    pwm_stop_requested = 1;
    return 1;
}

static void parse_and_run(int terminal) {
    char command_line[PWM_MAX_INPUT];
    char command_name[PWM_MAX_INPUT];
    char parameter[PWM_MAX_INPUT];
    int length = terminals[terminal].input_length;
    int space = -1;
    if (length <= 0) return;
    if (length >= PWM_MAX_INPUT) length = PWM_MAX_INPUT - 1;
    for (int i = 0; i < length; i++) command_line[i] = terminals[terminal].input[i];
    command_line[length] = '\0';
    if (length > 5 && command_line[0] == 's' && command_line[1] == 'u' && command_line[2] == 'd' && command_line[3] == 'o' && command_line[4] == ' ') {
        if (!auth_current_user_is_sudoer()) { append_line(terminal, "Permission denied: user is not a sudoer."); return; }
        for (int i = 0; i <= length - 5; i++) command_line[i] = command_line[i + 5];
        length -= 5;
    }
    for (int i = 0; i < length; i++) if (command_line[i] == ' ') { space = i; break; }
    if (space >= 0) {
        for (int i = 0; i < space; i++) command_name[i] = command_line[i];
        command_name[space] = '\0';
        int index = 0;
        for (int i = space + 1; i < length; i++) parameter[index++] = command_line[i];
        parameter[index] = '\0';
    } else { copy_text(command_name, command_line, PWM_MAX_INPUT); parameter[0] = '\0'; }
    {
        command_t *command = find_command(command_name);
        if (command) command->handler(parameter, terminal_puts, terminal_putc);
        else append_line(terminal, "Unknown command\n");
    }
}

static void prompt(void) {
    terminal_puts(auth_current_username()[0] ? auth_current_username() : "piux");
    terminal_puts(" ");
    terminal_puts(ext2_is_mounted() ? fs.current_path : "~");
    terminal_puts("> ");
}

static void create_terminal(void) {
    if (window_count >= PWM_MAX_WINDOWS) return;
    active_window = window_count++;
    copy_text(terminals[active_window].title, "terminal", sizeof(terminals[active_window].title));
    prompt();
}

int pwm_run(pwm_clear_t clear, pwm_puts_t puts, pwm_putc_t putc) {
    (void)clear; (void)puts; (void)putc;
    window_count = 0;
    active_window = 0;
    mouse_init();
    pwm_running = 1;
    pwm_stop_requested = 0;
    create_terminal();
    {
        int dirty = 1;
    while (!pwm_stop_requested) {
        int key;
        mouse_event_t mouse_event;
        if (dirty) {
            draw_desktop();
            dirty = 0;
        }
        while (mouse_poll(&mouse_event)) {
            if (handle_mouse(&mouse_event)) dirty = 1;
        }
        key = keyboard_try_read_char();
        if (key < 0) continue;
        if (key == 17 && keyboard_is_ctrl_pressed()) create_terminal();
        else if (key == 3 && keyboard_is_ctrl_pressed()) {
            if (window_count > 1) {
                for (int i = active_window + 1; i < window_count; i++) terminals[i - 1] = terminals[i];
                window_count--;
                if (active_window >= window_count) active_window = window_count - 1;
            }
        } else if (key == KEY_ARROW_LEFT && keyboard_is_ctrl_pressed() && active_window > 0) active_window--;
        else if (key == KEY_ARROW_RIGHT && keyboard_is_ctrl_pressed() && active_window + 1 < window_count) active_window++;
        else if (key == KEY_ARROW_UP && terminals[active_window].history_count > 0) {
            if (terminals[active_window].history_position > 0) terminals[active_window].history_position--;
            replace_input(terminals[active_window].history[terminals[active_window].history_position]);
        } else if (key == KEY_ARROW_DOWN && terminals[active_window].history_position < terminals[active_window].history_count) {
            terminals[active_window].history_position++;
            if (terminals[active_window].history_position == terminals[active_window].history_count) replace_input("");
            else replace_input(terminals[active_window].history[terminals[active_window].history_position]);
        }
        else if (key == '\n') {
            terminal_putc('\n');
            save_history(active_window);
            draw_desktop();
            parse_and_run(active_window);
            terminals[active_window].input_length = 0;
            terminals[active_window].history_position = terminals[active_window].history_count;
            if (!pwm_stop_requested) prompt();
        } else if (key == '\b' && terminals[active_window].input_length > 0) {
            terminals[active_window].input_length--;
            terminal_putc('\b');
        } else if (key >= 32 && key < 127 && terminals[active_window].input_length < PWM_MAX_INPUT - 1) {
            terminals[active_window].input[terminals[active_window].input_length++] = (char)key;
            terminal_putc((char)key);
        }
        dirty = 1;
    }
    }
    pwm_running = 0;
    if (clear) clear();
    return 1;
}
