#include <stdbool.h>
#include <stm32.h>
#include <gpio.h>

#include "dma_uart.h"

/// RC HSI (High Speed Internal) clock frequency in Hz (16 MHz)
#define HSI_HZ 16000000U

/// UART2 uses PCLK1, which by default is the HSI clock
#define PCLK1_HZ HSI_HZ

#define BAUD 9600U

void initDmaUart() {
  // enable clock timing
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN |
    RCC_AHB1ENR_DMA1EN;
  RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
  
  // configure usart
  GPIOafConfigure(GPIOA,
    2,
    GPIO_OType_PP,
    GPIO_Fast_Speed,
    GPIO_PuPd_NOPULL,
    GPIO_AF_USART2);
  
  GPIOafConfigure(GPIOA,
    3,
    GPIO_OType_PP,
    GPIO_Fast_Speed,
    GPIO_PuPd_UP,
    GPIO_AF_USART2);

  USART2->CR1 = USART_CR1_RE | USART_CR1_TE;
  USART2->CR2 = 0;
  USART2->BRR = (PCLK1_HZ + (BAUD / 2U)) / BAUD;
  USART2->CR3 = USART_CR3_DMAT | USART_CR3_DMAR;

  // configure sending stream

  DMA1_Stream6->CR = 
      4U << 25
    | DMA_SxCR_PL_1
    | DMA_SxCR_MINC
    | DMA_SxCR_DIR_0
    | DMA_SxCR_TCIE;

  DMA1_Stream6->PAR = (uint32_t)&USART2->DR;

  // configure receiving stream

  DMA1_Stream5->CR = 
      4U << 25
    | DMA_SxCR_PL_1
    | DMA_SxCR_MINC
    | DMA_SxCR_TCIE;

  DMA1_Stream5->PAR = (uint32_t)&USART2->DR;

  // clear interrupt marks
  DMA1->HIFCR = DMA_HIFCR_CTCIF6 | DMA_HIFCR_CTCIF5;

  // enable interrupts
  NVIC_EnableIRQ(DMA1_Stream6_IRQn);
  NVIC_EnableIRQ(DMA1_Stream5_IRQn);

  // enable usart
  USART2->CR1 |= USART_CR1_UE;
}

// send/receive

typedef struct {
  const char* buf;
  size_t len;
} SendQueueElem;

#define SEND_QUEUE_SIZE 64

struct SendQueue {
  SendQueueElem elems[SEND_QUEUE_SIZE];
  int size;
  int start;
} queue;

#define QUEUE_GET(n) (queue.elems[(queue.start + n) % SEND_QUEUE_SIZE])

#define QUEUE_POP() \
({ \
  SendQueueElem* ret = &QUEUE_GET(0); \
  queue.size--; \
  queue.start++; \
  ret; \
})

#define QUEUE_PUSH(buf, len) \
  do { \
    SendQueueElem* elem = &QUEUE_GET(queue.size); \
    elem->buf = buf; \
    elem->len = len; \
    queue.size++; \
  } while (false)

static void queueSend(const char* buf, size_t len) {
  QUEUE_PUSH(buf, len);
}

static void forceSend(const char* buf, size_t len) {
  DMA1_Stream6->M0AR = (uint32_t)buf;
  DMA1_Stream6->NDTR = len;
  DMA1_Stream6->CR |= DMA_SxCR_EN;
}

void dmaSend(const char* buf, size_t len) {
  if ((DMA1_Stream6->CR & DMA_SxCR_EN) == 0
      && (DMA1->HISR & DMA_HISR_TCIF6) == 0) {
    forceSend(buf, len);
  } else {
    queueSend(buf, len);
  }
}

void dmaRecv(char* buf) { // size must be 1
  DMA1_Stream5->M0AR = (uint32_t)buf;
  DMA1_Stream5->NDTR = 1;
  DMA1_Stream5->CR |= DMA_SxCR_EN;
}

// Handlers

static DmaUartHandler handlers[HANDLER_TYPES];

void registerDmaUartHandler(HandlerPurpose type, DmaUartHandler handler) {
  handlers[type] = handler;
}

#define CALL_HANDLER(type_) \
  do { \
    HandlerPurpose type = type_; \
    DmaUartHandler handler = handlers[type]; \
    if (handler) handler(); \
  } while (false)

void DMA1_Stream6_IRQHandler() {
  // read which interrupts we should handle
  uint32_t isr = DMA1->HISR;
  if (isr & DMA_HISR_TCIF6) {
    // clear interrupt flag
    DMA1->HIFCR = DMA_HIFCR_CTCIF6;

    if (queue.size > 0) {
      SendQueueElem* to_send = QUEUE_POP();
      forceSend(to_send->buf, to_send->len);
    }

    CALL_HANDLER(H_DMA_SEND_FINISH);
  }
}

void DMA1_Stream5_IRQHandler() {
  // read which interrupts we should handle
  uint32_t isr = DMA1->HISR;
  if (isr & DMA_HISR_TCIF5) {
    // clear interrupt flag
    DMA1->HIFCR = DMA_HIFCR_CTCIF5;
    
    CALL_HANDLER(H_DMA_RECEIVE_FINISH);
  }
}