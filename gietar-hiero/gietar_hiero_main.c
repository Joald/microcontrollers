#include <stdbool.h>
#include <stm32.h>

#include <gpio.h>
#include <fonts.h>
#include <delay.h>

#include "dma_uart.h"
#include "keyboard.h"

// quotation marks include to use the modified driver
#include "lcd.h"


void lcd_init() {
  LCDconfigure();
  LCDsetFont(&font8x16);
  LCDclear();
  LCDgoto(0, 0);
  // #define STR "Keyboard initialized!"
  // const char* buf = STR;
  // for (unsigned i = 0; i < sizeof(STR); ++i) {
  //   LCDputcharWrap(buf[i]);
  // }
}

void setTim3Params(int prescaler, int max_value) {
  TIM3->CR1 |= TIM_CR1_UDIS;
  TIM3->PSC = prescaler;
  TIM3->ARR = max_value;
  TIM3->CR1 &= ~TIM_CR1_UDIS;
}
int displayed_row = 0;
int displayed_col = 0;
int main() {
  initDmaUart();
  DMA_DBG("DMA INIT DONE\n");

  DMA_DBG("KB INIT START\n");
  kb_init();
  DMA_DBG("KB INIT DONE\n");
  
  DMA_DBG("LCD INIT\n");
  lcd_init();  
  DMA_DBG("LCD INIT DONE\n");
  Delay(100000);

  LCDclear();
  #define STRR "Row: "
  const char* buf = STRR;
  for (unsigned i = 0; i < sizeof(STRR) - 1; ++i) {
    LCDputchar(buf[i]);
  }
  LCDputcharWrap('0' + key_row);
  #define STRC ", Col: "
  buf = STRC;
  for (unsigned i = 0; i < sizeof(STRC) - 1; ++i) {
    LCDputcharWrap(buf[i]);
  }
  LCDputcharWrap('0' + key_col);

  while (true) {
    if (displayed_row != key_row || displayed_col != key_col) {
      LCDgoto(0, 5);
      LCDputchar('0' + key_row);
      LCDgoto(0, 13);
      LCDputchar('0' + key_col);
      displayed_col = key_col;
      displayed_row = key_row;
    }
    Delay(1000000);
  }
}