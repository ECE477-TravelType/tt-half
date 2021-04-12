#include "fifo.h"

struct fifo recv_fifo = {0};

// initialize UART peripherals
void
init_uart() {
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
}

void
USART1_IRQHandler() {
    if (USART1->ISR & USART_ISR_RXNE) {
        recv_buf[recv_buf_i] = usart_read_char(USART1);
        recv_buf_i++;
        if (recv_buf_i == sizeof(recv_buf)) {
            recv_buf_i = 0;
        }
    }
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

char
usart_read_char(USART_TypeDef *usart) {
    // Clear any overrun errors
    if ((usart->ISR & USART_ISR_ORE) == USART_ISR_ORE) {
        usart->ICR &= ~USART_ICR_ORECF;
    }

    // Wait until there's something received
    while ((usart->ISR & USART_ISR_RXNE) == 0) {};
    return usart->RDR;
}

// Read out a line from the fifo into a buffer, or `maxsize` bytes, whichever is smaller.
// Returns number of bytes read.
int
usart_readline(struct fifo *f, char *buf, int maxsize) {
    if (fifo_empty(f)) return 0;
    if (!fifo_newline(f)) return 0;

    int i;
    while (i < maxsize - 1 && (buf[i] = fifo_remove(f)) != '\n') {
        i++;
    }

    buf[i+1] = '\0';
    return i;
}
