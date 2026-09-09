#include <stdint.h>

static int is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

int tokenize(const char *input, char tokens[][256], int max_tokens) {
    int count = 0;
    const char *p = input;

    while (*p && count < max_tokens) {
        while (is_space(*p)) p++;
        if (!*p) break;

        char quote = 0;
        if (*p == '"' || *p == '\'') {
            quote = *p;
            p++;
        }

        char *out = tokens[count];
        while (*p) {
            if (quote) {
                if (*p == quote) {
                    p++;
                    break;
                }
            } else if (is_space(*p)) {
                break;
            }
            if (*p == '\\' && quote && p[1]) {
                p++;
                if (*p == 'n') {
                    *out++ = '\n';
                } else {
                    *out++ = *p;
                }
                p++;
                continue;
            }
            *out++ = *p++;
        }
        *out = '\0';
        count++;
    }
    return count;
}

void cmd_echo(const char *args, void (*vga_puts)(const char*), void (*vga_putc)(char)) {
    char tokens[16][256];
    int n = tokenize(args, tokens, 16);

    for (int i = 0; i < n; i++) {
        vga_puts(tokens[i]);
        if (i + 1 < n) {
            vga_putc(' ');
        }
    }
    vga_putc('\n');
}