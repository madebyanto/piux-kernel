#include <stdint.h>

typedef void (*vga_puts_t)(const char*);
typedef void (*vga_putc_t)(char);

extern void vga_clear(void);

void cmd_clean(const char *param, vga_puts_t vga_puts, vga_putc_t vga_putc) {
    vga_clear();
}
