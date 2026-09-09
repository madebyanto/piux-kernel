#include <stdint.h>

typedef void (*vga_puts_t)(const char*);

extern const char _os_infos_start[];
extern const char _os_infos_end[];

void cmd_about(const char *param, vga_puts_t vga_puts, void (*vga_putc)(char)) {
    const char *ptr = _os_infos_start;
    while (ptr < _os_infos_end) {
        char c = *ptr++;
        if (c == '\n') {
            vga_puts("\n");
        } else {
            vga_putc(c);
        }
    }
}
