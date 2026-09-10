#include <stdint.h>
#include "../tui/wm/wm.h"

typedef void (*vga_puts_t)(const char *);
typedef void (*vga_putc_t)(char);

extern void vga_clear(void);
extern const char _pwm_info_start[];
extern const char _pwm_info_end[];

static int matches(const char *value, const char *expected) {
    int index = 0;
    while (value[index] && expected[index] && value[index] == expected[index]) index++;
    return value[index] == expected[index];
}

static void print_help(vga_puts_t vga_puts) {
    vga_puts("Usage: pwm [--help|--info|--reload|--start|--stop]\n");
    vga_puts("  --help     Show this help message\n");
    vga_puts("  --info     Show pWM status and controls\n");
    vga_puts("  --reload   Reset the active terminal view and redraw pWM\n");
    vga_puts("  --start    Start pWM from the minimal shell\n");
    vga_puts("  --stop     Stop pWM and return to the minimal shell\n");
}

static void print_info(vga_putc_t vga_putc) {
    const char *info = _pwm_info_start;
    while (info < _pwm_info_end) vga_putc(*info++);
}

void cmd_pwm(const char *param, vga_puts_t vga_puts, vga_putc_t vga_putc) {
    (void)vga_putc;
    if (param[0] == '\0' || matches(param, "--help")) {
        print_help(vga_puts);
        return;
    }
    if (matches(param, "--info")) {
        print_info(vga_putc);
        vga_puts(pwm_command_is_active() ? "Status: active\n" : "Status: inactive\n");
        return;
    }
    if (matches(param, "--reload")) {
        if (pwm_command_reload()) vga_puts("pWM reloaded\n");
        else vga_puts("pWM is not active\n");
        return;
    }
    if (matches(param, "--stop")) {
        if (pwm_command_stop()) vga_puts("Stopping pWM...\n");
        else vga_puts("pWM is not active\n");
        return;
    }
    if (matches(param, "--start")) {
        if (pwm_command_is_active()) {
            vga_puts("pWM is already active\n");
            return;
        }
        pwm_run(vga_clear, vga_puts, vga_putc);
        return;
    }
    vga_puts("Unknown pwm option: ");
    vga_puts(param);
    vga_puts("\n\n");
    print_help(vga_puts);
}
