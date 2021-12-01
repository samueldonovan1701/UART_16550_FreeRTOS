#include <uart.h>

#include <ARMCM3.h>

#include "FreeRTOSConfig.h"
#include <FreeRTOS.h>
#include <stream_buffer.h>
#include <semphr.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//Signals
static bool UART_TX_empty = false;

//Async Comm Buffer
static uint8_t UART_TX_StreamBuf_Storage [UART_BUF_SIZE+1];
static StaticStreamBuffer_t UART_TX_StreamBuf_Struct;
static StreamBufferHandle_t UART_TX_StreamBuf_Handle;

//Lock
static StaticSemaphore_t UART_Lock_Storage;
static SemaphoreHandle_t UART_Lock_Semaphore;

void UART_init(int baud_rate)
{
//RTOS Setup
  UART_TX_StreamBuf_Handle = xStreamBufferCreateStatic(sizeof(UART_TX_StreamBuf_Storage), 
                                                    UART_TX_TRIGGER_LEVEL, 
                                                    UART_TX_StreamBuf_Storage, 
                                                    &UART_TX_StreamBuf_Struct);
  configASSERT(UART_TX_StreamBuf_Handle);

  UART_Lock_Semaphore = xSemaphoreCreateRecursiveMutexStatic(&UART_Lock_Storage);
  configASSERT(UART_Lock_Semaphore);

//Signal Setup
  UART_TX_empty = true;


//Device Setup
//https://media.digikey.com/pdf/Data%20Sheets/Texas%20Instruments%20PDFs/PC16550D.pdf
  
  //Baud Rate
  int divisor = UART_CLK / (baud_rate << 4);
  *UART_LCR = 0x80;  // Set Divisor Latch Access Bit (DLAB)
  *UART_DLL = divisor & 0xFF;  // write low byte
  *UART_DLH = divisor >> 8;    // write high byte
  
  //Data format
  *UART_LCR = 0x07; //8 bit data + 2 stop bits
  
  //FIFOs
  *UART_FCR = 0x0F; //Reset & Enable FIFOs + Set RX/TXRDY, Set RX FIFO trigger to 1

  //Interrupts
  *UART_IER = 0x02; //Enable TX Empty INT (0x03 for TX+RX INT)
  NVIC_EnableIRQ(2);
}


// -------------------------------------------------------------------

void UART_ISR()
{
  int interrupt_id = *UART_IIR & 0b1111; //Bits 0,1,2,3 (3 only in FIFO mode)
  switch(interrupt_id)
  {
    //case 0b0001: //No interrupt (shouldn't happen)
    case 0b0110: //Overrun Error, Parity Error, or Framing Error, or Break Interrupt
    {
      //Should not happen (block)
      for(;;);
      //Clear by reading LSR
      char temp = *UART_LSR;
      break;
    }
    case 0b0100: //RX Data Available
    {
      //Should not happen (block)
      for(;;);
      //Clear by reading RBR / FIFO below thresh
      char temp = *UART_RBR;
      break;
    }
    case 0b1100: //Character Timeout Indication
    {
      //Should not happen (block)
      for(;;);
      //Clear by reading Reciever
      char temp = *UART_RBR;
      break;
    }
    case 0b0010: //TX Empty
    {
      static char buf[16]; //NOTE: 1 to 16 characters may be written to XMIT FIFO during this ISR
      BaseType_t task_was_woken = pdFALSE;
      size_t n_chars = xStreamBufferReceiveFromISR(UART_TX_StreamBuf_Handle,
                                           buf,
                                           sizeof(buf),
                                           &task_was_woken);
      if(n_chars > 0)
      {
        for(int i = 0; i < n_chars; i++)
        {
	  *UART_THR = buf[i];
        }
      }
      else
      {
        UART_TX_empty = true;
      }

      portYIELD_FROM_ISR(task_was_woken);
      break;
    }
    case 0b0000: //MODEM Status
    {
      //Should not happen (block)
      for(;;);
    }
    default: //Unknown
    {
      //Should not happen (block)
      for(;;);
      break;
    }
  }

  NVIC_ClearPendingIRQ(2);
}

size_t UART_putChar(const char c)
{
  if(UART_TX_empty)
  { //UART TX is empty, write a null char to directly to TX
    *UART_THR = c;
    UART_TX_empty = false;
    return 1;
  }
  else
  { //Else write to buf
    return xStreamBufferSend(UART_TX_StreamBuf_Handle, &c, 1, UART_MAX_BLOCK);
  }
}

size_t UART_putStr(const char *s)
{
  size_t n_sent = 0;

  if(s != NULL)
  {
      size_t len = strlen(s);  // find out how many chararters we have

      if(len == 0)
        return n_sent;


      if(UART_TX_empty)
      {   //Send first char if TX is empty
	  *UART_THR = s[0];
          UART_TX_empty = false;
          len -= 1;
          s += 1;
          n_sent = 1;
      }

      // Send remaining chars
      if(len > 0)
	n_sent += xStreamBufferSend(UART_TX_StreamBuf_Handle, s, len, UART_MAX_BLOCK);
  }

  return n_sent;
}

void UART_lock()
{
  xSemaphoreTakeRecursive(UART_Lock_Semaphore, UART_MAX_BLOCK);
}

void UART_unlock()
{
  xSemaphoreGiveRecursive(UART_Lock_Semaphore);
}

char UART_getChar()
{
  while ((*UART_LSR & 0b1) != 1); //Wait for line status to say data ready

  return (*UART_RBR);
}

size_t UART_getStr(char* s, int len)
{
  char c;
  int i;

  for(i = 0; i < len-1; i++)
  {
    c = UART_getChar();
    if(c == '\n' || c == '\r')
      break;
    else
      s[i] = c;
  }
  s[i] = 0;
  return i;
}

//Utils
//https://en.wikipedia.org/wiki/ANSI_escape_code

static char UART_cmd_buf[32];

void UART_clear() 
{
  //CSI 2 J 
  UART_putStr("\e[2J");
}
void UART_setPos(unsigned short x, unsigned short y) 
{
  //CSI x ; i H
  sprintf(UART_cmd_buf, "\e[%u;%uH", x, y);
  UART_putStr(UART_cmd_buf);
}
void UART_decorate(unsigned short option) 
{
  //CSI option m
  sprintf(UART_cmd_buf, "\e[%um", option);
  UART_putStr(UART_cmd_buf);
}
void UART_undecorate()
{
  UART_decorate(UART_SGI_RESET);
}
void UART_setFG(unsigned char r, unsigned char g, unsigned char b)
{
  //CSI 38 2 r g b m
  sprintf(UART_cmd_buf, "\e[38;2;%u;%u;%u;m", r, g, b);
  UART_putStr(UART_cmd_buf);
}
void UART_setBG(unsigned char r, unsigned char g, unsigned char b)
{
  //CSI 48 2 r g b m
  sprintf(UART_cmd_buf, "\e[48;2;%u;%u;%u;m", r, g, b);
  UART_putStr(UART_cmd_buf);
}