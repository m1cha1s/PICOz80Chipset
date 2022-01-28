#include "pico/stdlib.h"

#include "memory.h"

#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

#include "composite.h"
#include "composite.pio.h"

#include "bitmap.h"

void init_composite(void) {
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &composite_program);

    dma_channel = dma_claim_unused_channel(true);
    vline = 1;
    bline = 0;

    // TODO finish the init process
    write_vsync_l(&vsync_ll[0],        HDOTS>>1);			// Pre-build a long/long vscan line...
    write_vsync_l(&vsync_ll[HDOTS>>1], HDOTS>>1);
    write_vsync_l(&vsync_ls[0],        HDOTS>>1);			// A long/short vscan line...
    write_vsync_s(&vsync_ls[HDOTS>>1], HDOTS>>1);
    write_vsync_s(&vsync_ss[0],        HDOTS>>1);			// A short/short vscan line
    write_vsync_s(&vsync_ss[HDOTS>>1], HDOTS>>1);

    memset(&border[0], BORDER_COLOUR, HDOTS);				// Fill the border with the border colour
    memset(&border[0], 1, HSYNC_BP1);				        // Add the hsync pulse
    memset(&border[HSYNC_BP1], 9, HSYNC_BP2);

    for(int i = 0; i < 2; i++) {
        memset(&pixel_line_buffer[i][0], BORDER_COLOUR, HDOTS);
        memset(&pixel_line_buffer[i][0], 1, HSYNC_BP1);
        memset(&pixel_line_buffer[i][HSYNC_BP1], 9, HSYNC_BP2);
        memset(&pixel_line_buffer[i][PIXEL_START], 31, WIDTH);
    }

    // PIO init
    pio_sm_set_enabled(pio, STATE_MACHINE, false);
    pio_sm_clear_fifos(pio, STATE_MACHINE);
    composite_initialise_pio(pio, STATE_MACHINE, offset, 0, 5, PIOFREQ);
    composite_configure_pio_dma(pio, STATE_MACHINE, dma_channel, HDOTS+1);
    pio_sm_set_enabled(pio, STATE_MACHINE, true);

    composite_dma_handler();
}

void write_vsync_s(unsigned char *p, int lenght) {
    int pulse_width = lenght / 16;
    for(int i = 0; i < lenght; i++) {
        p[i] = i <= pulse_width ? 1 : 13;
    }
}

void write_vsync_l(unsigned char *p, int lenght) {
    int pulse_width = lenght - (lenght / 16) -1;
    for(int i = 0; i < lenght; i++) {
        p[i] = i >= pulse_width ? 13 : 1;
    }
}

void composite_dma_handler(void) {
    switch(vline) {
        case 1 ... 2:
            dma_channel_set_read_addr(dma_channel, vsync_ll, true);
            break;
        case 3:
            dma_channel_set_read_addr(dma_channel, vsync_ls, true);
            memcpy(&pixel_line_buffer[bline & 1][PIXEL_START], &bitmap[bline], WIDTH); // Copying data from bitmap to pixel_line_buffer
            break;
        case 4 ... 5:
        case 310 ... 312:
            dma_channel_set_read_addr(dma_channel, vsync_ss, true);
            break;

        case 6 ... 68:
        case 260 ... 309:
            dma_channel_set_read_addr(dma_channel, border, true);

        default:
            dma_channel_set_read_addr(dma_channel, pixel_line_buffer[bline ++ & 1], true); // Switch pixel buffer
            memcpy(&pixel_line_buffer[bline & 1][PIXEL_START], &bitmap[bline], WIDTH); // Copying data from bitmap to pixel_line_buffer
            break;
    }

    if(vline++ >= 312) {
        vline = 1;
        bline = 0;
    }

    dma_hw->ints0 = 1u << dma_channel;
}

void composite_configure_pio_dma(PIO pio, uint sm, uint dma_channel, size_t buffer_word_size) {
    pio_sm_clear_fifos(pio, sm);
    dma_channel_config c = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_read_increment(&c, true);
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, true));
    dma_channel_configure(dma_channel, &c,
        &pio->txf[sm],
        NULL,
        buffer_word_size,
        true
    );
    dma_channel_set_irq0_enabled(dma_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_0, composite_dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);
}