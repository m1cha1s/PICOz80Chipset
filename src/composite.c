#include "pico/stdlib.h"

#include "memory.h"

#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

#include "composite.h"
#include "composite.pio.h"

#include "bitmap.h"

void initComposite(void) {
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &composite_program);

    dma_channel = dma_claim_unused_channel(true);
    vline = 1;
    bline = 0;

    write_vsync_l(&vsync_ll[0], HDOTS>>1);
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
            memcpy(&pixel_buffer[bline & 1][PIXEL_START], &bitmap[bline], WIDTH);
            break;
        case 4 ... 5:
        case 310 ... 312:
            dma_channel_set_read_addr(dma_channel, vsync_ss, true);
            break;

        case 6 ... 68:
        case 260 ... 309:
            dma_channel_set_read_addr(dma_channel, border, true);

        default:
            dma_channel_set_read_addr(dma_channel, pixel_buffer[bline ++ & 1], true);
            memcpy(&pixel_buffer[bline & 1][PIXEL_START], &bitmap[bline], WIDTH);
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