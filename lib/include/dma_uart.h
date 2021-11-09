#ifndef DMA_UART_H 
#define DMA_UART_H

#include <stddef.h>

// Initialization
void initDmaUart();

// send/receive
void dmaSend(const char* buf, size_t len);
void dmaRecv(char* buf); // size must be 1

// Handlers
typedef void(*DmaUartHandler)();

#define HANDLER_TYPES 2

typedef enum {
  H_DMA_SEND_FINISH,
  H_DMA_RECEIVE_FINISH,
} HandlerPurpose;


#endif // DMA_UART_H