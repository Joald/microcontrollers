#ifndef DMA_UART_H 
#define DMA_UART_H

#include <stddef.h>

// Initialization
void initDmaUart();

// send/receive
void dmaSend(const char* buf, size_t len);
void dmaRecv(char* buf); // size must be 1

// helper send macro that works only for compile-time constants
#define DMA_SEND(MSG) dmaSend(MSG, sizeof(MSG) - 1)

// Handlers
typedef void(*DmaUartHandler)(char*);

#define HANDLER_TYPES 2

typedef enum {
  H_DMA_SEND_FINISH,
  H_DMA_RECEIVE_FINISH,
} HandlerPurpose;

void registerDmaUartHandler(HandlerPurpose type, DmaUartHandler handler);


#endif // DMA_UART_H