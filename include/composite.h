#ifndef COMPOSITE_H
#define COMPOSITE_H

#define STATE_MACHINE 0
#define WIDTH 256
#define HEIGHT 192
#define HSYNC_BP1 24
#define HSYNC_BP2 48
#define HDOTS 382
#define PIOFREQ 7.0f
#define BORDER_COLOUR 11

#define PIXEL_START HSYNC_BP1 + HSYNC_BP2 + 18

unsigned int dma_channel;
unsigned int vline;
unsigned int bline;

unsigned char vsync_ll[HDOTS+1];
unsigned char vsync_ls[HDOTS+1];
unsigned char vsync_ss[HDOTS+1];
unsigned char border[HDOTS+1];
unsigned char pixel_line_buffer[2][HDOTS+1];

void init_composite(void);

void write_vsync_s(unsigned char *p, int lenght);
void write_vsync_l(unsigned char *p, int lenght);

void composite_dma_handler(void);

#endif