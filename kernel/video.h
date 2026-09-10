#ifndef VIDEO_H
#define VIDEO_H

#include <stdint.h>

#define VIDEO_WIDTH 1280
#define VIDEO_HEIGHT 720
#define VIDEO_CELL_WIDTH 8
#define VIDEO_CELL_HEIGHT 16
#define VIDEO_COLUMNS (VIDEO_WIDTH / VIDEO_CELL_WIDTH)
#define VIDEO_ROWS (VIDEO_HEIGHT / VIDEO_CELL_HEIGHT)

extern int video_framebuffer_ready;
extern int video_columns;
extern int video_rows;

void video_init(uint32_t magic, uint32_t multiboot_address);
void video_clear(void);
void video_scroll(void);
void video_put_cell(int column, int row, char character, uint8_t color);
int video_begin_frame(void);
void video_present(void);

#endif
