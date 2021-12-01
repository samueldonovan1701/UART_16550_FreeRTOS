#ifndef UART_H
#define UART_H

#include <stdlib.h>

// 16550D UART
#define UART_RBR     ((volatile uint32_t*)0x44A11000) // Receiver Buffer Register
#define UART_THR     ((volatile uint32_t*)0x44A11000) // Transmitter Holding Register
#define UART_IER     ((volatile uint32_t*)0x44A11004) // Interrupt Enable Register
#define UART_IIR     ((volatile uint32_t*)0x44A11008) // Interrupt Identification Register
#define UART_FCR     ((volatile uint32_t*)0x44A11008) // FIFO Control Register
#define UART_LCR     ((volatile uint32_t*)0x44A1100C) // Line Control Register
#define UART_MCR     ((volatile uint32_t*)0x44A11010) // Modem Control Register
#define UART_LSR     ((volatile uint32_t*)0x44A11014) // Line Status Register
#define UART_MSR     ((volatile uint32_t*)0x44A11018) // Modem Status Register
#define UART_SCR     ((volatile uint32_t*)0x44A1101C) // Scratch Register
#define UART_DLL     ((volatile uint32_t*)0x44A11000) // Divisor Latch (LSB) Register
#define UART_DLH     ((volatile uint32_t*)0x44A11004) // Divisor Latch (MSB) Register

//Constants
#define UART_CLK 50000000
#define UART_BUF_SIZE 512
#define UART_TX_TRIGGER_LEVEL 1
#define UART_MAX_BLOCK pdMS_TO_TICKS(1000)

void UART_init(int baud_rate);
void UART_ISR();

size_t UART_putChar(const char c);
size_t UART_putStr(const char *s);
char uart_getChar();
void uart_getStr(char* s, int len);

void UART_lock();
void UART_unlock();

//Utils
void UART_clear();
void UART_setPos(unsigned short x, unsigned short y);
void UART_decorate(unsigned short option);
void UART_undecorate();                                            
void UART_setFG(unsigned char r, unsigned char g, unsigned char b);
void UART_setBG(unsigned char r, unsigned char g, unsigned char b);


#define UART_SGI_RESET 0

#define UART_SGI_BOLD 1
#define UART_SGI_DIM 2
#define UART_SGI_ITALIC 3
#define UART_SGI_UNDERLINE 4
#define UART_SGI_OVERLINE 53
#define UART_SGI_SLOW_BLINK 5
#define UART_SGI_FAST_BLINK 6
#define UART_SGI_STRIKE 9

#define UART_SGI_FG_BLACK 30
#define UART_SGI_FG_RED 31
#define UART_SGI_FG_GREEN 32
#define UART_SGI_FG_YELLOW 33
#define UART_SGI_FG_BLUE 34
#define UART_SGI_FG_MAGENTA 35
#define UART_SGI_FG_CYAN 36
#define UART_SGI_FG_WHITE 37
#define UART_SGI_FG_GREY 90

#define UART_SGI_BG_BLACK 40
#define UART_SGI_BG_RED 41
#define UART_SGI_BG_GREEN 42
#define UART_SGI_BG_YELLOW 43
#define UART_SGI_BG_BLUE 44
#define UART_SGI_BG_MAGENTA 45
#define UART_SGI_BG_CYAN 46
#define UART_SGI_BG_WHITE 47
#define UART_SGI_BG_GREY 100

#endif