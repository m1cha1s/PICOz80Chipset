#include <stdio.h>
#include "pico/stdlib.h"

int main(void) {  
    stdio_init_all(); // Initialize stdio for printf, scanf, etc.

    gpio_init(PICO_DEFAULT_LED_PIN); // Initialize Led pin
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT); // Set led pins direction

    while (true) {

        // Blink
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        sleep_ms(100);
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        sleep_ms(100);


        printf("\n\nHello World\n"); // Hello world!
    }
}
