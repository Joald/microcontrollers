#ifndef UART_INIT
#define UART_INIT

#include <gpio.h>

////////////////////////// UART INITIALIZATION //////////////////////////

// CR1 register

#define USART_Mode_Rx_Tx (USART_CR1_RE | USART_CR1_TE)
#define USART_Enable USART_CR1_UE

#define USART_WordLength_8b 0x0000
#define USART_WordLength_9b USART_CR1_M

#define USART_Parity_No 0x0000
#define USART_Parity_Even USART_CR1_PCE
#define USART_Parity_Odd (USART_CR1_PCE | USART_CR1_PS)

// CR2 register

#define USART_StopBits_1 0x0000
#define USART_StopBits_0_5 0x1000
#define USART_StopBits_2 0x2000
#define USART_StopBits_1_5 0x3000

// CR3 register

#define USART_FlowControl_None 0x0000
#define USART_FlowControl_RTS USART_CR3_RTSE
#define USART_FlowControl_CTS USART_CR3_CTSE

// BRR register

/// RC HSI (High Speed Internal) clock frequency in Hz (16 MHz)
#define HSI_HZ 16000000U

/// UART2 uses PCLK1, which by default is the HSI clock
#define PCLK1_HZ HSI_HZ

// Sample config
#define BAUD_RATE 9600U

void initUart();

#endif // UART_INIT