#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "fifo.h"
#include "usart.h"

struct fifo usart1_recv_fifo = {0};
struct fifo usart2_recv_fifo = {0};
struct fifo usart2_tx_fifo = {0};

void __io_putchar(char ch);
char __io_getchar();
void USART1_IRQHandler();
void USART2_IRQHandler();

// initialize UART peripherals
void
usart_init() {
    setbuf(stdin,0);
    setbuf(stdout,0);
    setbuf(stderr,0);

    // GPIO configuration for USART1 (PA9,10)
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN;

    GPIOA->MODER &= ~(GPIO_MODER_MODE9 | GPIO_MODER_MODE10);
    GPIOA->MODER |= GPIO_MODER_MODE9_1 | GPIO_MODER_MODE10_1; // Alternate function mode

    GPIOA->AFR[1] &= ~(GPIO_AFRH_AFSEL9 | GPIO_AFRH_AFSEL10);
    GPIOA->AFR[1] |= 0x4 << GPIO_AFRH_AFSEL9_Pos
                   | 0x4 << GPIO_AFRH_AFSEL10_Pos;  // AF4 for pa9,10

    GPIOA->OSPEEDR &= ~(GPIO_OSPEEDER_OSPEED9 | GPIO_OSPEEDER_OSPEED10);
    GPIOA->OSPEEDR |= 3 << GPIO_OSPEEDER_OSPEED9_Pos
                    | 3 << GPIO_OSPEEDER_OSPEED10_Pos; // output speed = 0b11 fast

    // GPIO config for USART2 (PA2,3)
    MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODE2 | GPIO_MODER_MODE3,
               GPIO_MODER_MODE2_1 | GPIO_MODER_MODE3_1); // alternate function modes

    MODIFY_REG(GPIOA->AFR[0], GPIO_AFRL_AFSEL2 | GPIO_AFRL_AFSEL3,
               0x4 << GPIO_AFRL_AFSEL2_Pos | 0x4 << GPIO_AFRL_AFSEL3_Pos); // AF4 for pa2,3

    MODIFY_REG(GPIOA->OSPEEDR, GPIO_OSPEEDER_OSPEED2 | GPIO_OSPEEDER_OSPEED3,
               0b11 << GPIO_OSPEEDER_OSPEED2_Pos | 0b11 << GPIO_OSPEEDER_OSPEED3_Pos); // output speed fast

    // Set Up UART1
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    USART1->CR1 &= ~USART_CR1_UE;    // Disable UE to make changes
    USART1->CR1 &= ~USART_CR1_M;     // Set to 8 bit data length
    USART1->BRR = 0x12;              // Set Baud Rate to 115.3Kb/s (value obtained with baud-rate detection)
    USART1->CR2 &= ~USART_CR2_STOP;  // Set for a single stop bit
    USART1->CR1 |= USART_CR1_TE
                 | USART_CR1_RE
                 | USART_CR1_RXNEIE; // Enable Transmission and Reception
    USART1->CR2 |= USART_CR2_ABREN;
    USART1->CR1 |= USART_CR1_UE;     // Enable UE

    while ((USART1->ISR & USART_ISR_TEACK) == 0) {}
    while ((USART1->ISR & USART_ISR_REACK) == 0) {}

    // enable USART1 interrupt
	NVIC_SetPriority(USART1_IRQn, 0);
	NVIC_EnableIRQ(USART1_IRQn);

    // Set Up UART2
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    USART2->CR1 &= ~USART_CR1_UE;    // Disable UE to make changes
    USART2->CR1 &= ~USART_CR1_M;     // Set to 8 bit data length
    USART2->BRR = 0x12;              // Set Baud Rate to 115.3Kb/s (value obtained with baud-rate detection)
    USART2->CR2 &= ~USART_CR2_STOP;  // Set for a single stop bit
    USART2->CR1 |= USART_CR1_TE
                 | USART_CR1_RE
                 | USART_CR1_RXNEIE; // Enable Transmission and Reception
    USART2->CR2 |= USART_CR2_ABREN;
    USART2->CR1 |= USART_CR1_UE;     // Enable UE

    while ((USART2->ISR & USART_ISR_TEACK) == 0) {}
    while ((USART2->ISR & USART_ISR_REACK) == 0) {}

    // enable USART2 interrupt
	NVIC_SetPriority(USART2_IRQn, 0);
	NVIC_EnableIRQ(USART2_IRQn);
}

char
usart_read_char(USART_TypeDef *usart) {
    struct fifo *f;
    if (usart == USART1) {
        f = &usart1_recv_fifo;
    } else if (usart == USART2) {
        f = &usart2_recv_fifo;
    } else {
        return 0; // no such usart
    }

    return fifo_remove(f);
}

void
usart_write_char(USART_TypeDef *usart, char c) {
    while ((usart->ISR & USART_ISR_TXE) == 0) {};
    usart->TDR = c;
}

void
usart_write(USART_TypeDef *usart, char *buf, int len) {
    for (int i = 0; i < len; i++) {
        usart_write_char(usart, buf[i]);
    }
}

void
usart_write_str(USART_TypeDef *usart, char *str) {
    usart_write(usart, str, strlen(str));
}

// Read out a line from the fifo into a buffer, or `maxsize` bytes, whichever is smaller.
// Returns the number of bytes read.
int
usart_readline(USART_TypeDef *usart, char *buf, int maxsize) {
    struct fifo *f;
    if (usart == USART1) {
        f = &usart1_recv_fifo;
    } else if (usart == USART2) {
        f = &usart2_recv_fifo;
    } else {
        return 0; // no such usart
    }

    while (fifo_empty(f) || !fifo_newline(f)) {}

    int i = 0;
    while (i < maxsize - 1 && (buf[i] = fifo_remove(f)) != '\n') {
        i++;
    }

    buf[i+1] = '\0';
    return i;
}

void
putstr(const char *s) {
    while (*s) putchar(*s++);
}

bool echo_mode = true;

void
insert_echo_char(char ch) {
    if (ch == '\r') ch = '\n';
    if (!echo_mode) {
        fifo_insert(&usart2_recv_fifo, ch);
        return;
    }
    if (ch == '\b' || ch == '\177') {
        if (!fifo_empty(&usart2_recv_fifo)) {
            char tmp = fifo_uninsert(&usart2_recv_fifo);
            if (tmp == '\n')
                fifo_insert(&usart2_recv_fifo, '\n');
            else if (tmp < 32)
                putstr("\b\b  \b\b");
            else
                putstr("\b \b");
        }
        return; // Don't put a backspace into buffer.
    } else if (ch == '\n') {
        __io_putchar('\n');
    } else if (ch == 0){
        putstr("^0");
    } else if (ch == 28) {
        putstr("^\\");
    } else if (ch < 32) {
        __io_putchar('^');
        __io_putchar('A'-1+ch);
    } else {
        __io_putchar(ch);
    }
    fifo_insert(&usart2_recv_fifo, ch);
}

void
__io_putchar(char ch) {
    while ( fifo_full(&usart2_tx_fifo) ) {
        asm("wfi");
    }

    if ( ch == '\n' ) {
        fifo_insert(&usart2_tx_fifo, '\r');
    } else {
        fifo_insert(&usart2_tx_fifo, ch);
    }

    if ( (USART2->CR1 & USART_CR1_TXEIE) == 0 ) {
        USART2->CR1 |= USART_CR1_TXEIE;
        USART2_IRQHandler();
    }

    if ( ch == '\n' ) {
        while ( fifo_full(&usart2_tx_fifo) ) {
            asm("wfi");
        }

        fifo_insert(&usart2_tx_fifo, '\n');
    }
}

char
__io_getchar(void) {
    while (!fifo_newline(&usart2_recv_fifo)) {
        asm("wfi");
    }
    return fifo_remove(&usart2_recv_fifo);
}

void
USART1_IRQHandler() {
    if (USART1->ISR & USART_ISR_RXNE) {
        usart_write_char(USART2, USART1->RDR);
        // __io_putchar(USART1->RDR);
        // fifo_insert(&usart1_recv_fifo, (char)USART1->RDR);
    }
}

void
USART2_IRQHandler() {
    if (USART2->ISR & USART_ISR_RXNE) {
        // usart_write_char(USART1, USART2->RDR);
        insert_echo_char(USART2->RDR);
    }

    if (USART2->ISR & USART_ISR_TXE) {
        if (fifo_empty(&usart2_tx_fifo)) {
            USART2->CR1 &= ~USART_CR1_TXEIE;
        } else {
            USART2->TDR = fifo_remove(&usart2_tx_fifo);
        }
    }
}
