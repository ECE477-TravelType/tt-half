#ifndef __USART_H__
#define __USART_H__

#include <stdbool.h>

#include "common.h"
#include "fifo.h"

void usart_init      ();
char usart_read_char (USART_TypeDef *usart);
void usart_write_str (USART_TypeDef *usart, char *str);
int  usart_readline  (USART_TypeDef *usart, char *buf, int maxsize);

#endif
