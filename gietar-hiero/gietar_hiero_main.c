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
  LCDputcharWrap('-');
  #define STRC ", Col: "
  buf = STRC;
  for (unsigned i = 0; i < sizeof(STRC) - 1; ++i) {
    LCDputcharWrap(buf[i]);
  }
  LCDputcharWrap('-');

  LCDgoto(1, 0);
  #define STRP "B===D"
  buf = STRP;
  for (unsigned i = 0; i < sizeof(STRP) - 1; ++i) {
    LCDputcharWrap(buf[i]);
  }

  bool hash_displayed = false;  

  while (true) {
    KbKey key;
    while ((key = getNext()) != KB_NOKEY) {
      if (key == KB_1) {
        DMA_DBG("Found key 1!\n");
      }
      if (GET_ROW_NUM(key) == 1) {
        DMA_DBG("ROW_NUM = 1!\n");
      }
      if (GET_COL_NUM(key) == 1) {
        DMA_DBG("COL_NUM = 1!\n");
      }
      LCDgoto(0, 5);
      LCDputchar('0' + GET_ROW_NUM(key));
      LCDgoto(0, 13);
      LCDputchar('0' + GET_COL_NUM(key));
    } 
    if (isKeyHeld(KB_POUND)) {
      if (!hash_displayed) {
        LCDgoto(3, 7);
        LCDputchar('#');
        hash_displayed = true;
      }
    } else if (hash_displayed) {
      LCDgoto(3, 7);
      LCDputchar(' ');        
      hash_displayed = false;
    }

    Delay(1000000);
  }
}