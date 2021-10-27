#include "uart_init.h"

void initUart() {
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
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

  USART2->CR1 = 
      USART_Mode_Rx_Tx 
    | USART_WordLength_8b 
    | USART_Parity_No;

  USART2->CR2 = USART_StopBits_1;

  USART2->CR3 = USART_FlowControl_None;

  USART2->BRR = (PCLK1_HZ + (BAUD_RATE / 2U)) / BAUD_RATE;

  USART2->CR1 |= USART_Enable;
}