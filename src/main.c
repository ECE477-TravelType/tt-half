// -*- mode: C; compile-command: "make -k run";  -*-

#include <string.h>
#include <stdio.h>

#include "common.h"
#include "usart.h"

int
main(void) {
    usart_init();

    usart_write_str(USART1, "$$$");
    // usart_write_str(USART1, "+\r");
    // usart_write_str(USART1, "SS,C0\r");
    // usart_write_str(USART1, "F\r");
    // usart_write_str(USART1, "R,1\r");
    // printf("\n~~welcome to oats' software emporium!~~\n");
    // printf("#SaveBenFrom437\n");

    while (1) {
        char input_buf[512] = {0};
        fgets(input_buf, sizeof(input_buf), stdin);
        usart_write_str(USART1, input_buf);
    }

    while (1) asm("wfi");
}
