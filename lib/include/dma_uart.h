#ifndef DMA_UART_H
#define DMA_UART_H

#include <stddef.h>

#ifndef NDEBUG
// debug, definition will be in .c file
#define DECL_BEGIN
#define DECL_END ;
#else 
// no debug, make it inline noop
#define DECL_BEGIN \
  _Pragma("GCC diagnostic push") \
  _Pragma("GCC diagnostic ignored \"-Wunused-parameter\"") \
  inline
#define DECL_END \
  {} \
  _Pragma("GCC diagnostic pop")
#endif


// Initialization
DECL_BEGIN void initDmaUart() DECL_END

// send/receive
DECL_BEGIN void dmaSend(const char* buf, size_t len) DECL_END
DECL_BEGIN void dmaSendWithCopy(const char* buf, size_t len) DECL_END
DECL_BEGIN void dmaRecv(char* buf) DECL_END // size must be 1

// helper send macro that works only for compile-time constants
#define DMA_DBG(MSG) dmaSend(MSG, sizeof(MSG) - 1)

// Handlers
typedef void(*DmaUartHandler)(const char*);

#define HANDLER_TYPES 2

typedef enum {
  H_DMA_SEND_FINISH,
  H_DMA_RECEIVE_FINISH,
} HandlerPurpose;

DECL_BEGIN void registerDmaUartHandler(HandlerPurpose type, DmaUartHandler handler) DECL_END


#endif // DMA_UART_H